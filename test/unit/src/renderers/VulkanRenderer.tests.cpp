#include "renderers/VulkanRenderer.h"
#include "cameras/OrthographicCamera.h"
#include "cameras/PerspectiveCamera.h"
#include "geometries/PlaneGeometry.h"
#include "extras/DataUtils.h"
#include "core/NativeObject.h"
#include "core/JSJobs.h"
#include "materials/MeshBasicMaterial.h"
#include "objects/Group.h"
#include "objects/Mesh.h"
#include "objects/InstancedMesh.h"
#include "objects/SkinnedMesh.h"
#include "objects/LOD.h"
#include "objects/Line.h"
#include "objects/Points.h"
#include "objects/Sprite.h"
#include "scenes/FogExp2.h"
#include "scenes/Scene.h"
#include "textures/DataTexture.h"
#include <array>
#include <iostream>
#include <limits>
#include <stdexcept>

namespace {
int assertions = 0;
void require(bool condition, const char* description) {
    ++assertions;
    if (!condition)
        throw std::runtime_error(description);
}
void finishHostTurn() {
    endJSJob();
    JSValue::object().collectGarbage();
}
void renderHostTurn(VulkanRenderer& renderer, Scene& scene, Camera& camera) {
    // These synchronous native test calls are separate host turns. Production
    // render/prune must never end WeakRef keepalive or run a checkpoint itself.
    static bool active = false;
    if (!active) finishHostTurn();
    struct Guard { bool& active; bool previous; ~Guard() { active = previous; } } guard{active, active};
    active = true;
    renderer.render(scene, camera);
}
std::array<int, 4> pixel(const VulkanRenderer& renderer, unsigned x = 32, unsigned y = 32) {
    const auto pixels = renderer.readPixels();
    const auto index = (y * renderer.getWidth() + x) * 4;
    return {pixels.at(index), pixels.at(index + 1), pixels.at(index + 2), pixels.at(index + 3)};
}
void expectPixel(const VulkanRenderer& renderer, std::array<int, 4> expected,
                 const char* description, unsigned x = 32, unsigned y = 32, int tolerance = 1) {
    const auto actual = pixel(renderer, x, y);
    for (int i = 0; i < 4; ++i)
        if (std::abs(actual[i] - expected[i]) > tolerance) {
            std::cerr << description << ": expected " << expected[0] << ',' << expected[1] << ','
                      << expected[2] << ',' << expected[3] << ", received " << actual[0] << ','
                      << actual[1] << ',' << actual[2] << ',' << actual[3] << '\n';
            require(false, description);
        }
    require(true, description);
}
std::shared_ptr<Mesh> plane(const Color& color) {
    return std::make_shared<Mesh>(std::make_shared<PlaneGeometry>(1.6, 1.6),
                                  std::make_shared<MeshBasicMaterial>(color));
}
std::shared_ptr<MeshBasicMaterial> basic(const std::shared_ptr<Mesh>& mesh) {
    return std::dynamic_pointer_cast<MeshBasicMaterial>(mesh->material);
}

void unsupportedMeshTests(bool validation) {
    VulkanRenderer renderer({64, 64, validation});
    renderer.srgbOutput = false;
    renderer.setClearColor(Color(0, 0, 0), 1);
    OrthographicCamera camera(-1, 1, 1, -1, .1, 20);
    camera.position.set(0, 0, 5);
    Scene scene;
    auto supported = plane(Color(0, 1, 0));
    scene.add(supported);
    for (bool instanced : {false}) {
        auto geometry = std::make_shared<PlaneGeometry>(1.6, 1.6);
        auto material = std::make_shared<MeshBasicMaterial>(Color(1, 0, 0));
        std::shared_ptr<Mesh> mesh;
        if (instanced) mesh = std::make_shared<InstancedMesh>(geometry, material, 1);
        else {
            auto skinned=std::make_shared<SkinnedMesh>(geometry, material);
            // Objects updates a skeleton before material visibility/GPU capability.
            skinned->skeleton = std::make_shared<Skeleton>();
            skinned->boundingSphere=std::make_shared<Sphere>(Vector3(),1.2);
            mesh=skinned;
        }
        auto parent = std::make_shared<Group>();
        parent->add(mesh); scene.add(parent);
        const auto expectSkipped = [&](const char* message) {
            renderHostTurn(renderer, scene, camera);
            require(renderer.getInfo().calls == 1, message);
            expectPixel(renderer, {0, 255, 0, 255}, message);
        };
        mesh->visible = false;
        expectSkipped("Invisible unsupported mesh does not reject a supported frame");
        mesh->visible = true; parent->visible = false;
        expectSkipped("Invisible ancestor excludes an unsupported mesh");
        parent->visible = true; mesh->layers.set(1);
        expectSkipped("Camera layers exclude an unsupported mesh");
        mesh->layers.set(0); material->visible = false;
        expectSkipped("Invisible material excludes an unsupported mesh");
        material->visible = true; mesh->position.set(100, 0, 0);
        expectSkipped("Frustum culling excludes an unsupported mesh");
        mesh->position.set(0, 0, 0);
        supported->onBeforeRender = [&](auto&, auto&, auto&, auto*, auto*, auto*) {
            mesh->layers.set(1);
        };
        expectSkipped("Late layer changes skip queued unsupported mesh before capability checks");
        supported->onBeforeRender = {};
        mesh->layers.set(0);
        bool rejected = false;
        try { renderHostTurn(renderer, scene, camera); }
        catch (const std::invalid_argument& error) {
            rejected = std::string(error.what()) == (instanced
                ? "Vulkan InstancedMesh rendering is not implemented yet"
                : "Vulkan SkinnedMesh rendering is not implemented yet");
        }
        require(rejected, "Visible instanced and skinned meshes explicitly reject unsupported GPU paths");
        mesh->visible = false;
        expectSkipped("Renderer records and submits a valid frame after unsupported mesh rejection");
        scene.remove(parent);
    }
    require(renderer.getValidationMessages().empty(), "Unsupported mesh rejection does not leave invalid Vulkan state");
}

void unsupportedPrimitiveTests(bool validation) {
    VulkanRenderer renderer({64,64,validation});
    renderer.srgbOutput=false;renderer.setClearColor(Color(0,0,0),1);
    OrthographicCamera camera(-1,1,1,-1,.1,20);camera.position.z=5;
    Scene scene;auto supported=plane(Color(0,1,0));scene.add(supported);
    auto check=[&](const auto& primitive,const char* rejection) {
        primitive->renderOrder=1;
        auto parent=std::make_shared<Group>();parent->add(primitive);scene.add(parent);
        const auto material=primitive->material;
        auto expectSkipped=[&](const char* description) {
            renderHostTurn(renderer, scene,camera);
            require(renderer.getInfo().calls==1,description);
            expectPixel(renderer,{0,255,0,255},description);
        };
        primitive->visible=false;expectSkipped("Invisible primitive skips unsupported GPU path");
        primitive->visible=true;parent->visible=false;
        expectSkipped("Invisible ancestor excludes unsupported primitive");
        parent->visible=true;primitive->layers.set(1);
        expectSkipped("Camera layers exclude unsupported primitive");
        primitive->layers.set(0);material->visible=false;
        expectSkipped("Invisible primitive material skips unsupported GPU path");
        material->visible=true;primitive->position.x=100;
        expectSkipped("Frustum culling excludes unsupported primitive");
        primitive->position.x=0;
        if constexpr(requires{primitive->materials;primitive->materialIsArray;}) {
            primitive->materialIsArray=true;
            primitive->geometry->clearGroups();
            expectSkipped("Empty material array overrides the visible single material");
            primitive->materials.push_back(material);
            expectSkipped("Material array without geometry groups submits no primitive");
            primitive->geometry->addGroup(0,6,0);material->visible=false;
            expectSkipped("Invisible group material submits no primitive");
            material->visible=true;primitive->geometry->clearGroups();
            primitive->geometry->addGroup(0,6,1);
            expectSkipped("Missing group material submits no primitive");
            primitive->geometry->clearGroups();primitive->geometry->addGroup(0,6,0);
        }
        int before=0,materialBefore=0,after=0;
        primitive->onBeforeRender=[&](auto&,auto&,auto&,auto*,auto*,auto*){++before;};
        primitive->onAfterRender=[&](auto&,auto&,auto&,auto*,auto*,auto*){++after;};
        material->onBeforeRender=[&](auto&,auto&,auto&,auto*,auto*,auto*){++materialBefore;};
        supported->onBeforeRender=[&](auto&,auto&,auto&,auto*,auto*,auto*){primitive->layers.set(1);};
        expectSkipped("Late layer filtering excludes a queued unsupported primitive");
        require(before==0 && materialBefore==0,"Filtered primitive does not invoke render callbacks");
        supported->onBeforeRender={};primitive->layers.set(0);
        bool rejected=false;
        try { renderHostTurn(renderer, scene,camera); }
        catch(const std::invalid_argument& error){rejected=std::string(error.what())==rejection;}
        require(rejected,"Submitted primitive explicitly rejects its unsupported GPU path");
        require(before==1 && materialBefore==1 && after==0,
                "Primitive callbacks precede the submitted capability check");
        primitive->visible=false;expectSkipped("Renderer recovers after unsupported primitive rejection");
        primitive->onBeforeRender={};primitive->onAfterRender={};material->onBeforeRender={};
        scene.remove(parent);
    };
    check(std::make_shared<Line>(std::make_shared<PlaneGeometry>(1.6,1.6)),
          "Vulkan Line rendering is not implemented yet");
    check(std::make_shared<Points>(std::make_shared<PlaneGeometry>(1.6,1.6)),
          "Vulkan Points rendering is not implemented yet");
    check(std::make_shared<Sprite>(),"Vulkan Sprite rendering is not implemented yet");
    require(renderer.getValidationMessages().empty(),"Primitive guards leave valid Vulkan state");
}

void instancingTests(bool validation) {
    VulkanRenderer renderer({64,64,validation});
    renderer.srgbOutput = false;
    renderer.setClearColor(Color(0,0,0),1);
    OrthographicCamera camera(-1,1,1,-1,.1,20); camera.position.z=5;
    Scene scene;
    auto geometry=std::make_shared<PlaneGeometry>(.6,.6);
    auto material=std::make_shared<MeshBasicMaterial>(Color(1,1,1));
    auto mesh=std::make_shared<InstancedMesh>(geometry,material,2);
    Matrix4 left,right; left.makeTranslation(-.5,0,0); right.makeTranslation(.5,0,0);
    mesh->setMatrixAt(0,left).setMatrixAt(1,right);
    mesh->setColorAt(0,Color(1,0,0)).setColorAt(1,Color(0,0,1));
    std::vector<std::string> order;
    const auto position=std::dynamic_pointer_cast<BufferAttribute>(geometry->getAttribute("position"));
    position->onUpload([&]{order.push_back("position");});
    mesh->instanceMatrix->onUpload([&]{order.push_back("matrix");});
    mesh->instanceColor->onUpload([&]{order.push_back("color");});
    mesh->onBeforeRender=[&](auto&,auto&,auto&,auto*,auto*,auto*){order.push_back("before");};
    geometry->index->onUpload([&]{order.push_back("index");});
    mesh->onAfterRender=[&](auto&,auto&,auto&,auto*,auto*,auto*){order.push_back("after");};
    scene.add(mesh);
    renderHostTurn(renderer, scene,camera);
    require(order==std::vector<std::string>{"position","matrix","color","before","index","after"},
        "Instance attributes upload after geometry and before object hooks");
    expectPixel(renderer,{255,0,0,255},"First instance uses its matrix and color",16,32);
    expectPixel(renderer,{0,0,255,255},"Second instance uses its matrix and color",48,32);
    require(renderer.getInfo().calls==1 && renderer.getInfo().triangles==4,"Instance draw uses one call and multiplied triangle statistics");
    const auto buffers=renderer.getInfo().attributeBuffers;
    order.clear(); mesh->setColorAt(0,Color(0,1,0));
    renderHostTurn(renderer, scene,camera);
    expectPixel(renderer,{255,0,0,255},"Instance color writes require needsUpdate",16,32);
    require(order==std::vector<std::string>{"before","after"},"Unchanged instance versions do not upload");
    mesh->instanceColor->addUpdateRange(0,3);mesh->instanceColor->needsUpdate=true;
    renderHostTurn(renderer, scene,camera);
    expectPixel(renderer,{0,255,0,255},"Instance color partial upload reaches GPU",16,32);
    expectPixel(renderer,{0,0,255,255},"Instance color partial upload preserves other rows",48,32);
    require(mesh->instanceColor->updateRanges.get("length").strictEquals(0),"Instance update ranges clear after upload");
    mesh->onBeforeRender=[&](auto&,auto&,auto&,auto*,auto*,auto*){
        mesh->setMatrixAt(0,right);mesh->instanceMatrix->needsUpdate=true;
    };
    renderHostTurn(renderer, scene,camera);
    expectPixel(renderer,{0,255,0,255},"Object callback cannot retroactively upload an instance matrix",16,32);
    mesh->onBeforeRender={};renderHostTurn(renderer, scene,camera);
    expectPixel(renderer,{0,0,0,255},"Instance matrix callback changes upload next frame",16,32);
    mesh->setMatrixAt(0,left);mesh->instanceMatrix->needsUpdate=true;
    mesh->instanceMatrix->meshPerAttribute=2;
    renderHostTurn(renderer, scene,camera);
    expectPixel(renderer,{0,0,255,255},"Same-identity divisor changes retain existing vertex binding",48,32);
    geometry->setAttribute("position",position->clone());
    position->dispose();
    renderHostTurn(renderer, scene,camera);
    expectPixel(renderer,{0,0,0,255},"Attribute identity changes rebind the instance divisor",48,32);
    mesh->instanceMatrix->meshPerAttribute=1;
    mesh->dispose();renderHostTurn(renderer, scene,camera);
    expectPixel(renderer,{0,0,255,255},"Disposal releases instance bindings and recreates them",48,32);
    require(renderer.getInfo().attributeBuffers==buffers,"Instance resources are recreated without accumulating buffers");
    int hooks=0; mesh->onBeforeRender=[&](auto&,auto&,auto&,auto*,auto*,auto*){++hooks;};
    mesh->onAfterRender=[&](auto&,auto&,auto&,auto*,auto*,auto*){++hooks;};
    mesh->count=0;renderHostTurn(renderer, scene,camera);
    require(renderer.getInfo().calls==0 && hooks==2,"Numeric zero skips instance draw but keeps object hooks");
    mesh->count=JSValue("0");renderHostTurn(renderer, scene,camera);
    require(renderer.getInfo().calls==1 && renderer.getInfo().triangles==0,"String zero retains a zero-instance draw and statistics");
    mesh->count=1.5;renderHostTurn(renderer, scene,camera);
    require(renderer.getInfo().calls==1 && renderer.getInfo().triangles==3,"Fractional count truncates GPU instances but preserves logical statistics");
    expectPixel(renderer,{0,0,0,255},"Fractional count does not draw its truncated instance",48,32);
    mesh->count=3;renderHostTurn(renderer, scene,camera);
    require(pixel(renderer,16,32)==std::array<int,4>{0,0,0,255} && renderer.getInfo().calls==1,
            "Instance fetch beyond storage reports a draw error without GPU submission or aborting info");
    mesh->count=2;geometry->setDrawRange(0,0);renderHostTurn(renderer, scene,camera);
    require(renderer.getInfo().calls==1 && renderer.getInfo().triangles==0,"Zero vertex count preserves instanced draw statistics");
    geometry->setDrawRange(0,INFINITY);
    mesh->frustumCulled=true;mesh->position.x=10;
    left.makeTranslation(-10.5,0,0);right.makeTranslation(-9.5,0,0);
    mesh->setMatrixAt(0,left).setMatrixAt(1,right);mesh->instanceMatrix->needsUpdate=true;mesh->boundingSphere.reset();
    renderHostTurn(renderer, scene,camera);
    expectPixel(renderer,{0,255,0,255},"Instance bounds keep visible instances when mesh origin is outside the frustum",16,32);
    require(renderer.getInfo().calls==1,"Instance frustum uses the object sphere");
    mesh->position.x=0;left.makeTranslation(-.5,0,0);right.makeTranslation(.5,0,0);
    mesh->setMatrixAt(0,left).setMatrixAt(1,right);mesh->instanceMatrix->needsUpdate=true;mesh->boundingSphere.reset();
    scene.fog=std::make_shared<Fog>(Color(1,1,1),4,6);
    right.makeTranslation(.5,0,1);mesh->setMatrixAt(1,right);mesh->instanceMatrix->needsUpdate=true;
    renderHostTurn(renderer, scene,camera);
    expectPixel(renderer,{128,255,128,255},"Instance fog uses transformed view depth",16,32,2);
    expectPixel(renderer,{0,0,255,255},"Instance depth changes its own fog amount",48,32,2);
    scene.fog.reset();
    // A geometry attribute overrides the object's matrix, while both resources retain upload behavior.
    auto overrideMatrix=std::make_shared<InstancedBufferAttribute>(Float32Array(32),16);
    for (std::size_t i=0;i<16;++i) {
        std::get<Float32Array>(overrideMatrix->array)[i]=float(left.elements()[i]);
        std::get<Float32Array>(overrideMatrix->array)[i+16]=float(left.elements()[i]);
    }
    geometry->setAttribute("instanceMatrix",overrideMatrix);renderHostTurn(renderer, scene,camera);
    expectPixel(renderer,{0,0,0,255},"Geometry instanceMatrix takes precedence over the object matrix",48,32);
    geometry->deleteAttribute("instanceMatrix");renderHostTurn(renderer, scene,camera);
    expectPixel(renderer,{0,0,255,255},"Removing geometry override restores object matrix",48,32);
    mesh->onBeforeRender={};mesh->onAfterRender={};
    mesh->dispose();
    require(renderer.getInfo().attributeBuffers==buffers-1,"Mesh dispose removes current instance matrix and color caches only");
    scene.remove(mesh);geometry->dispose();renderHostTurn(renderer, scene,camera);
    require(renderer.getInfo().calls==0,"Removing an instanced mesh leaves a valid empty frame");
    require(renderer.getValidationMessages().empty(),"Instance rendering produces no validation messages");

    Scene morphScene;
    auto morphGeometry=std::make_shared<PlaneGeometry>(.3,.3);
    const auto morphPosition=std::dynamic_pointer_cast<BufferAttribute>(morphGeometry->getAttribute("position"));
    auto target=morphPosition->clone();
    for(double i=0;i<target->count;++i) target->setY(i,target->getY(i)+.4);
    morphGeometry->morphAttributes["position"]={target};
    auto morphMesh=std::make_shared<InstancedMesh>(morphGeometry,material,2);
    left.makeTranslation(-.5,0,0);right.makeTranslation(.5,0,0);
    morphMesh->setMatrixAt(0,left).setMatrixAt(1,right);
    morphMesh->setColorAt(0,Color(1,0,0)).setColorAt(1,Color(0,0,1));
    Mesh weights(morphGeometry,material);
    weights.morphTargetInfluences->at(0)=0;morphMesh->setMorphAt(0,weights);
    weights.morphTargetInfluences->at(0)=1;morphMesh->setMorphAt(1,weights);
    morphScene.add(morphMesh);
    const auto textureUploads=renderer.getInfo().textureUploads;
    renderHostTurn(renderer, morphScene,camera);
    require(morphMesh->morphTexture->version==0 && renderer.getInfo().textureUploads==textureUploads,
            "Version-zero morph textures retain the uninitialized black texture fallback");
    require(renderer.getInfo().calls==1 && renderer.getInfo().triangles==4,
            "Version-zero morph textures still submit instanced geometry");
    expectPixel(renderer,{0,0,0,255},"Black morph fallback collapses the first instance",16,32);
    expectPixel(renderer,{0,0,0,255},"Black morph fallback safely handles rows beyond its one texel",48,19);
    const auto fullMorphTexture=morphMesh->morphTexture;
    morphMesh->morphTexture=std::make_shared<DataTexture>(Float32Array{1},1,1,RedFormat,FloatType);
    morphMesh->morphTexture->needsUpdate=true;renderHostTurn(renderer, morphScene,camera);
    expectPixel(renderer,{255,0,0,255},"Missing morph weight columns fetch zero",16,32);
    expectPixel(renderer,{0,0,0,255},"Missing morph texture rows fetch zero",48,32);
    morphMesh->morphTexture=fullMorphTexture;
    morphMesh->morphTexture->needsUpdate=true;renderHostTurn(renderer, morphScene,camera);
    expectPixel(renderer,{255,0,0,255},"Instance morph base weight preserves first row",16,32);
    expectPixel(renderer,{0,0,255,255},"Instance morph weights use their own texture row",48,19);
    expectPixel(renderer,{0,0,0,255},"Morphed instance leaves its original position",48,32);
    weights.morphTargetInfluences->at(0)=0;morphMesh->setMorphAt(1,weights);
    renderHostTurn(renderer, morphScene,camera);
    expectPixel(renderer,{0,0,255,255},"Morph texture array changes require version update",48,19);
    morphMesh->morphTexture->needsUpdate=true;renderHostTurn(renderer, morphScene,camera);
    expectPixel(renderer,{0,0,255,255},"Versioned morph weights update GPU deformation",48,32);
    weights.morphTargetInfluences->at(0)=1;morphMesh->setMorphAt(1,weights);morphMesh->morphTexture->needsUpdate=true;
    for(double i=0;i<target->count;++i) target->setY(i,target->getY(i)+.3);
    target->needsUpdate=true;renderHostTurn(renderer, morphScene,camera);
    expectPixel(renderer,{0,0,255,255},"Morph target cache preserves unchanged target-count storage",48,19);
    morphMesh->instanceMatrix->meshPerAttribute=2;material->alphaTest=.01;renderHostTurn(renderer, morphScene,camera);
    expectPixel(renderer,{0,0,255,255},"Morph texture row does not use the matrix divisor",16,19);
    morphMesh->geometry=morphGeometry->toNonIndexed();morphMesh->instanceMatrix->meshPerAttribute=1;
    renderHostTurn(renderer, morphScene,camera);
    require(renderer.getInfo().calls==1 && renderer.getInfo().triangles==4,"Non-indexed instance draw preserves statistics");
    expectPixel(renderer,{0,0,255,255},"Non-indexed morph uses gl_VertexIndex",48,10);

    Scene reentryScene;
    material->alphaTest=0;
    auto reentry=std::make_shared<InstancedMesh>(std::make_shared<PlaneGeometry>(.6,.6),material,2);
    reentry->setMatrixAt(0,left).setMatrixAt(1,right);
    reentry->setColorAt(0,Color(1,0,0)).setColorAt(1,Color(0,0,1));
    reentryScene.add(reentry);
    auto coercion=JSValue::object();coercion.set("calls",0);
    coercion.set("valueOf",JSValue::function(+[](const JSValue& self,const auto&,const auto& captures)->JSValue {
        auto state=self;const auto calls=state.get("calls").toNumber()+1;state.set("calls",calls);
        auto object=std::static_pointer_cast<Object3D>(captures[0].hostPayload("three.Object3D"));
        if(calls==1) object->dispose();
        return calls;
    },{nativeObject(reentry)}));
    reentry->instanceMatrix->meshPerAttribute=coercion;
    renderHostTurn(renderer, reentryScene,camera);
    require(coercion.get("calls").number()==4,"Matrix divisor conversion runs independently for all four columns");
    expectPixel(renderer,{0,0,0,255},"Per-column divisors survive dispose during binding conversion",48,32);
    reentry->instanceMatrix->meshPerAttribute=1;
    renderHostTurn(renderer, reentryScene,camera);
    expectPixel(renderer,{0,0,255,255},"Instance cache recovers after divisor conversion disposes resources",48,32);
    auto clearing=JSValue::object();clearing.set("calls",0);
    clearing.set("valueOf",JSValue::function(+[](const JSValue& self,const auto&,const auto& captures)->JSValue {
        auto state=self;state.set("calls",state.get("calls").number()+1);
        auto object=std::dynamic_pointer_cast<InstancedMesh>(std::static_pointer_cast<Object3D>(captures[0].hostPayload("three.Object3D")));
        object->instanceColor.reset();return 1;
    },{nativeObject(reentry)}));
    reentry->instanceMatrix->meshPerAttribute=clearing;material->alphaTest=.01;
    renderHostTurn(renderer, reentryScene,camera);
    require(clearing.get("calls").number()==4,"Color replacement does not change matrix column conversion count");
    expectPixel(renderer,{0,0,0,255},"Matrix conversion clearing instanceColor disables the current color slot",48,32);
    renderHostTurn(renderer, reentryScene,camera);
    expectPixel(renderer,{255,255,255,255},"Next program omits removed instance colors",48,32);

    Scene lodScene;auto lod=std::make_shared<LOD>();
    lod->addLevel(plane(Color(1,0,0)),0);lod->addLevel(plane(Color(0,0,1)),8,.1);lodScene.add(lod);
    renderHostTurn(renderer, lodScene,camera);expectPixel(renderer,{255,0,0,255},"LOD updates before child projection");
    camera.position.z=10;renderHostTurn(renderer, lodScene,camera);
    expectPixel(renderer,{0,0,255,255},"LOD selects distant child during rendering");
    lod->autoUpdate=JSValue("true");camera.position.z=5;renderHostTurn(renderer, lodScene,camera);
    expectPixel(renderer,{0,0,255,255},"LOD automatic update requires strict true");
    lod->autoUpdate=true;renderHostTurn(renderer, lodScene,camera);
    expectPixel(renderer,{255,0,0,255},"LOD resumes automatic selection");
}

void textureTests(bool validation) {
    VulkanRenderer renderer({64, 64, validation});
    renderer.srgbOutput = false;
    renderer.setClearColor(Color(0, 0, 0), 1);
    OrthographicCamera camera(-1, 1, 1, -1, 0.1, 20);
    camera.position.set(0, 0, 5);
    Scene scene;
    auto mesh = plane(Color(1, 1, 1));
    const auto material = basic(mesh);
    scene.add(mesh);
    const auto uv = [&](double u, double v, const std::string& channel = "uv") {
        auto attribute = mesh->geometry->getAttribute(channel);
        if (!attribute) {
            attribute = std::make_shared<Float32BufferAttribute>(std::vector<float>(8), 2);
            mesh->geometry->setAttribute(channel, attribute);
        }
        for (int i = 0; i < 4; ++i) attribute->setXY(i, u, v);
        attribute->setNeedsUpdate(true);
    };
    const auto rgba = [](Uint8Array data, unsigned width = 1, unsigned height = 1) {
        auto texture = std::make_shared<DataTexture>(data, width, height);
        texture->needsUpdate = true;
        return texture;
    };
    {
        auto texture = std::make_shared<DataTexture>(Uint8Array{255, 0, 0, 255}, 1, 1);
        material->map = texture;
        int updates = 0;
        texture->onUpdate = [&](Texture&) { ++updates; };
        renderHostTurn(renderer, scene, camera);
        expectPixel(renderer, {0, 0, 0, 255}, "Texture version zero uses the upstream empty texture");
        require(updates == 0 && renderer.getInfo().textureImages == 0, "Version zero does not allocate or call onUpdate");
        texture->needsUpdate = true;
        renderHostTurn(renderer, scene, camera);
        expectPixel(renderer, {255, 0, 0, 255}, "RGBA8 DataTexture samples uploaded texels");
        require(updates == 1 && renderer.getInfo().textureImages == 1, "Initial texture upload creates one resource and callback");
        auto& data = std::get<Uint8Array>(texture->image->data);
        data.set(Uint8Array{0, 255, 0, 255});
        renderHostTurn(renderer, scene, camera);
        expectPixel(renderer, {255, 0, 0, 255}, "Raw texture mutations wait for needsUpdate");
        texture->needsUpdate = true;
        renderHostTurn(renderer, scene, camera);
        expectPixel(renderer, {0, 255, 0, 255}, "Texture version changes upload the changed data");
        require(updates == 2 && renderer.getInfo().textureImages == 1, "Superseded texture resources retire after submission");
        texture->dispose();
        require(renderer.getInfo().textureImages == 0, "Texture dispose releases an unused GPU resource immediately");
        renderHostTurn(renderer, scene, camera);
        expectPixel(renderer, {0, 255, 0, 255}, "Disposed live textures can upload again");
        require(updates == 3, "Disposal restores the first-upload callback path");
    }
    material->map.reset();
    renderHostTurn(renderer, scene, camera);
    require(renderer.getInfo().textureImages == 0, "Expired texture owners release GPU images without cycles");
    {
        auto texture = rgba(Uint8Array{255,0,0,255, 0,255,0,255, 0,0,255,255, 255,255,255,255}, 2, 2);
        material->map = texture;
        uv(.25, .25);
        renderHostTurn(renderer, scene, camera);
        expectPixel(renderer, {255,0,0,255}, "DataTexture default flipY false samples its first row at v zero");
        texture->flipY = true; texture->needsUpdate = true;
        renderHostTurn(renderer, scene, camera);
        expectPixel(renderer, {0,0,255,255}, "flipY reverses typed-array texture rows");
        texture->flipY = false; texture->needsUpdate = true;
        texture->offset.set(.5, 0);
        renderHostTurn(renderer, scene, camera);
        expectPixel(renderer, {0,255,0,255}, "Texture matrixAutoUpdate applies offset before sampling");
        texture->offset.set(0, 0); texture->repeat.set(1, 1); texture->center.set(.5, .5); texture->rotation = MATH_PI;
        renderHostTurn(renderer, scene, camera);
        expectPixel(renderer, {255,255,255,255}, "Texture matrix applies rotation about center");
        texture->matrixAutoUpdate = false;
        texture->matrix.set(1,0,0, 0,1,.5, 0,0,1);
        renderHostTurn(renderer, scene, camera);
        expectPixel(renderer, {0,0,255,255}, "Explicit texture matrix remains effective with matrixAutoUpdate false");
        texture->matrix.identity();
        uv(.75,.25,"uv1"); texture->channel = 1;
        renderHostTurn(renderer, scene, camera);
        expectPixel(renderer, {0,255,0,255}, "Texture channel one selects geometry uv1");
        uv(.25,.75,"uv2"); texture->channel = 2;
        renderHostTurn(renderer, scene, camera);
        expectPixel(renderer, {0,0,255,255}, "Texture channel two selects geometry uv2");
        texture->channel = 3;
        renderHostTurn(renderer, scene, camera);
        expectPixel(renderer, {255,0,0,255}, "A missing texture UV channel uses the default zero attribute");
        texture->channel = 0;
        uv(1.25,.25); texture->wrapS = RepeatWrapping; texture->needsUpdate = true;
        renderHostTurn(renderer, scene, camera);
        expectPixel(renderer, {255,0,0,255}, "RepeatWrapping repeats positive UV coordinates");
        texture->wrapS = ClampToEdgeWrapping; texture->needsUpdate = true;
        renderHostTurn(renderer, scene, camera);
        expectPixel(renderer, {0,255,0,255}, "ClampToEdgeWrapping clamps UV coordinates");
        texture->wrapS = MirroredRepeatWrapping; texture->needsUpdate = true;
        renderHostTurn(renderer, scene, camera);
        expectPixel(renderer, {0,255,0,255}, "MirroredRepeatWrapping mirrors odd UV periods");
        uv(-.25,.25);
        renderHostTurn(renderer, scene, camera);
        expectPixel(renderer, {255,0,0,255}, "MirroredRepeatWrapping handles negative UV coordinates");
        texture->wrapS = RepeatWrapping; texture->needsUpdate = true;
        renderHostTurn(renderer, scene, camera);
        expectPixel(renderer, {0,255,0,255}, "RepeatWrapping handles negative UV coordinates");
        uv(.5,.25); texture->magFilter = LinearFilter; texture->minFilter = LinearFilter; texture->needsUpdate = true;
        renderHostTurn(renderer, scene, camera);
        expectPixel(renderer, {128,128,0,255}, "Linear magnification interpolates neighboring texels");
    }
    {
        auto texture = rgba(Uint8Array{128,128,128,255}); material->map = texture;
        texture->colorSpace = SRGBColorSpace;
        renderHostTurn(renderer, scene, camera);
        expectPixel(renderer, {55,55,55,255}, "sRGB texture sampling decodes into the linear working space");
        renderer.srgbOutput = true;
        renderHostTurn(renderer, scene, camera);
        expectPixel(renderer, {128,128,128,255}, "sRGB texture input and sRGB output round trip");
        renderer.srgbOutput = false;
        texture->colorSpace = LinearSRGBColorSpace; texture->needsUpdate = true;
        renderHostTurn(renderer, scene, camera);
        expectPixel(renderer, {128,128,128,255}, "LinearSRGB texture bytes bypass the transfer function");
        texture->colorSpace = NoColorSpace; texture->needsUpdate = true;
        renderHostTurn(renderer, scene, camera);
        expectPixel(renderer, {128,128,128,255}, "NoColorSpace texture bytes bypass the transfer function");
        auto floating = std::make_shared<DataTexture>(Float32Array{.25f,.5f,1.f,1.f},1,1,RGBAFormat,FloatType);
        floating->needsUpdate = true; material->map = floating;
        renderHostTurn(renderer, scene, camera);
        expectPixel(renderer, {64,128,255,255}, "Float32 DataTexture samples floating-point storage");
        auto half = std::make_shared<DataTexture>(Uint16Array{toHalfFloat(.25),toHalfFloat(.5),toHalfFloat(1),toHalfFloat(1)},1,1,RGBAFormat,HalfFloatType);
        half->needsUpdate = true; material->map = half;
        renderHostTurn(renderer, scene, camera);
        expectPixel(renderer, {64,128,255,255}, "HalfFloat DataTexture samples decoded half storage");
        auto red = std::make_shared<DataTexture>(Uint8Array{128},1,1,RedFormat);
        red->needsUpdate = true; material->map = red;
        renderHostTurn(renderer, scene, camera);
        expectPixel(renderer, {128,0,0,255}, "Red texture fills absent green/blue with zero and alpha with one");
        auto rg = std::make_shared<DataTexture>(Uint8Array{64,128},1,1,RGFormat);
        rg->needsUpdate = true; material->map = rg;
        renderHostTurn(renderer, scene, camera);
        expectPixel(renderer, {64,128,0,255}, "RG texture preserves two channels and implicit alpha one");
        auto aligned = std::make_shared<DataTexture>(Uint8Array{32,0,0,0, 200},1,2,RedFormat);
        aligned->unpackAlignment = 4; aligned->needsUpdate = true; material->map = aligned;
        uv(.25,.75);
        renderHostTurn(renderer, scene, camera);
        expectPixel(renderer, {200,0,0,255}, "Typed-array uploads honor unpackAlignment row padding");
    }
    {
        auto map = rgba(Uint8Array{200,100,50,128});
        auto alpha = rgba(Uint8Array{255,128,0,0});
        material->map = map; material->alphaMap = alpha; material->blending = NoBlending; material->opacity = .5;
        renderHostTurn(renderer, scene, camera);
        expectPixel(renderer, {200,100,50,32}, "alphaMap uses green and multiplies map alpha and material opacity");
        material->alphaTest = .2;
        renderHostTurn(renderer, scene, camera);
        expectPixel(renderer, {0,0,0,255}, "alphaTest sees combined map and alphaMap alpha");
        material->alphaTest = 0; material->opacity = 1; material->alphaMap.reset();
        map->premultiplyAlpha = true; map->needsUpdate = true;
        renderHostTurn(renderer, scene, camera);
        expectPixel(renderer, {100,50,25,128}, "premultiplyAlpha multiplies typed-array RGB during upload");
        auto low=rgba(Uint8Array{1,3,5,128}); low->premultiplyAlpha=true;
        material->map=low;
        renderHostTurn(renderer, scene,camera);
        expectPixel(renderer,{0,1,2,128},"RGBA8 premultiply quantization follows Blink typed-array truncation",32,32,0);
        auto overflow=std::make_shared<DataTexture>(Uint16Array{0x7bff,0,0,0x4000},1,1,RGBAFormat,HalfFloatType);
        overflow->premultiplyAlpha=true; overflow->needsUpdate=true; material->map=overflow;
        material->color.set(1.0/131008,1,1);
        renderHostTurn(renderer, scene,camera);
        expectPixel(renderer,{255,0,0,255},"Half-float premultiplication preserves overflow to infinity instead of saturating");
        material->color.set(1,1,1);
        material->map.reset(); material->blending = NormalBlending;
        renderHostTurn(renderer, scene, camera);
        expectPixel(renderer, {255,255,255,255}, "Removing maps restores white sampler defaults");
    }
    {
        Uint8Array data(std::size_t(8*8*4));
        for (int y=0;y<8;++y) for (int x=0;x<8;++x) {
            const int offset=(y*8+x)*4, color=((x+y)%2)*255;
            data[offset]=color; data[offset+1]=color; data[offset+2]=color; data[offset+3]=255;
        }
        auto texture=rgba(data,8,8); material->map=texture;
        texture->generateMipmaps=true; texture->minFilter=LinearMipmapLinearFilter;
        texture->magFilter=LinearFilter; texture->wrapS=texture->wrapT=RepeatWrapping;
        texture->repeat.set(128,128);
        const auto standardUv=std::make_shared<PlaneGeometry>(1.6,1.6)->getAttribute("uv");
        mesh->geometry->setAttribute("uv",standardUv);
        renderHostTurn(renderer, scene,camera);
        expectPixel(renderer,{128,128,128,255},"Automatic mipmaps filter a minified checkerboard");
        texture->minFilter=NearestMipmapNearestFilter; texture->needsUpdate=true;
        renderHostTurn(renderer, scene,camera);
        expectPixel(renderer,{128,128,128,255},"Nearest mip-level selection uses generated mip levels");
        for (std::size_t i=0;i<data.size();i+=4) { data[i]=255; data[i+1]=0; data[i+2]=0; }
        texture->needsUpdate=true;
        renderHostTurn(renderer, scene,camera);
        expectPixel(renderer,{255,0,0,255},"Texture updates regenerate the automatic mip chain");
    }
    {
        uv(.25,.25);
        auto texture=rgba(Uint8Array{255,0,0,255, 0,255,0,255, 0,0,255,255, 255,255,255,255},2,2);
        material->map=texture;
        renderHostTurn(renderer, scene,camera);
        auto& data=std::get<Uint8Array>(texture->image->data);
        data.set(Uint8Array{10,20,30,255, 80,90,100,255, 110,120,130,255, 140,150,160,255});
        auto range=std::make_shared<Texture::UpdateRange>(Texture::UpdateRange{0,2});
        texture->updateRanges->push_back(range); texture->addUpdateRange(2,2);
        bool cleared=false;
        texture->onUpdate=[&](Texture& value) { cleared=value.updateRanges->empty(); };
        texture->needsUpdate=true;
        renderHostTurn(renderer, scene,camera);
        expectPixel(renderer,{10,20,30,255},"Texture updateRanges upload changed texels");
        require(cleared && range->count==4,"Texture ranges merge in place and clear before onUpdate");
        uv(.75,.25);
        renderHostTurn(renderer, scene,camera);
        expectPixel(renderer,{0,255,0,255},"Texture updateRanges preserve untouched uploaded texels");
        texture->addUpdateRange(8,4); texture->flipY=true; texture->needsUpdate=true;
        uv(.25,.75);
        renderHostTurn(renderer, scene,camera);
        expectPixel(renderer,{110,120,130,255},"Partial uploads flip a one-row region without moving its destination");
    }
    {
        auto texture=rgba(Uint8Array{255,0,0,255}); material->map=texture;
        int callbacks=0;
        texture->onUpdate=[&](Texture& value) {
            if (++callbacks==1) {
                std::get<Uint8Array>(value.image->data).set(Uint8Array{0,255,0,255});
                value.needsUpdate=true;
            }
        };
        renderHostTurn(renderer, scene,camera);
        expectPixel(renderer,{255,0,0,255},"Texture callback runs after pixel capture");
        renderHostTurn(renderer, scene,camera);
        expectPixel(renderer,{255,0,0,255},"Callback texture version increments are cached after callback");
        require(callbacks==1,"Callback version increment does not schedule a second upload");
        texture->needsUpdate=true;
        renderHostTurn(renderer, scene,camera);
        expectPixel(renderer,{0,255,0,255},"A later needsUpdate uploads callback pixel mutations");
        require(callbacks==2,"A later upload invokes onUpdate again");
        int state=0;
        texture->onUpdate=[count=0,&state](Texture&) mutable {state=++count;};
        texture->needsUpdate=true; renderHostTurn(renderer, scene,camera);
        texture->needsUpdate=true; renderHostTurn(renderer, scene,camera);
        require(state==2,"Copying onUpdate for lifetime safety preserves its mutable closure state");
        bool rejected=false;
        texture->onUpdate=[&](Texture&) {try {renderHostTurn(renderer, scene,camera);} catch(const std::logic_error&) {rejected=true;}};
        texture->needsUpdate=true; renderHostTurn(renderer, scene,camera);
        require(rejected,"Texture onUpdate rejects recursive rendering across the full render guard");
        texture->onUpdate=nullptr;
    }
    {
        auto texture=rgba(Uint8Array{255,255,255,255}); material->map=texture;
        material->blending=NoBlending;
        texture->onUpdate=[&](Texture& value) { material->color.set(1,0,0); material->opacity=.5; value.onUpdate=nullptr; };
        renderHostTurn(renderer, scene,camera);
        expectPixel(renderer,{255,255,255,255},"Texture callbacks cannot retroactively change already refreshed material uniforms");
        renderHostTurn(renderer, scene,camera);
        expectPixel(renderer,{255,0,0,128},"A later draw refreshes material uniforms modified by the upload callback");
        material->color.set(1,1,1); material->opacity=1; material->blending=NormalBlending;
    }
    {
        auto texture=rgba(Uint8Array{255,0,0,255}); material->map=texture;
        texture->source->dataReady=false;
        auto pendingRange=std::make_shared<Texture::UpdateRange>(Texture::UpdateRange{0,4});
        texture->updateRanges->push_back(pendingRange);
        int callbacks=0; texture->onUpdate=[&](Texture&){++callbacks;};
        const auto uploads=renderer.getInfo().textureUploads;
        renderHostTurn(renderer, scene,camera);
        expectPixel(renderer,{0,0,0,255},"dataReady false allocates safely initialized storage without user pixels");
        require(renderer.getInfo().textureUploads==uploads && callbacks==1 && texture->updateRanges->size()==1,
                "dataReady false records allocation/callback but preserves pending ranges and upload count");
        texture->source->dataReady=true;
        renderHostTurn(renderer, scene,camera);
        expectPixel(renderer,{0,0,0,255},"Setting dataReady alone does not change the texture version");
        texture->needsUpdate=true;
        renderHostTurn(renderer, scene,camera);
        expectPixel(renderer,{255,0,0,255},"dataReady plus needsUpdate uploads real pixel data");
        require(renderer.getInfo().textureUploads==uploads+1 && texture->updateRanges->empty(),"Ready upload consumes pending ranges once");
        texture->source->dataReady=false; texture->needsUpdate=true;
        std::get<Uint8Array>(texture->image->data).set(Uint8Array{0,255,0,255});
        renderHostTurn(renderer, scene,camera);
        expectPixel(renderer,{255,0,0,255},"dataReady false preserves previously uploaded pixels");
        texture->source->dataReady=true; texture->needsUpdate=true;
        renderHostTurn(renderer, scene,camera);
        expectPixel(renderer,{0,255,0,255},"A subsequent ready version refreshes existing storage");
    }
    {
        material->map.reset(); renderHostTurn(renderer, scene,camera);
        auto texture=rgba(Uint8Array{255,0,0,255}); material->map=texture;
        renderHostTurn(renderer, scene,camera);
        const auto images=renderer.getInfo().textureImages, uploads=renderer.getInfo().textureUploads;
        const auto clone=texture->clone();
        material->map=clone;
        renderHostTurn(renderer, scene,camera);
        expectPixel(renderer,{255,0,0,255},"Texture clone samples its shared Source");
        require(renderer.getInfo().textureImages==images && renderer.getInfo().textureUploads==uploads+1,
                "A cloned texture shares the cache entry but its incremented source version uploads");
        std::get<Uint8Array>(clone->image->data).set(Uint8Array{0,255,0,255}); clone->needsUpdate=true;
        renderHostTurn(renderer, scene,camera);
        material->map=texture; renderHostTurn(renderer, scene,camera);
        expectPixel(renderer,{0,255,0,255},"Shared Source uploads remain visible through an unchanged texture handle");
        clone->wrapS=RepeatWrapping; clone->needsUpdate=true; material->map=clone;
        renderHostTurn(renderer, scene,camera);
        require(renderer.getInfo().textureImages==images+1,"Different upstream texture cache keys create separate GPU storage");
        clone->dispose();
        require(renderer.getInfo().textureImages==images,"Disposing one cache key retains a separately used Source entry");
        material->map=texture; texture->dispose();
    }
    {
        auto texture=rgba(Uint8Array{255,0,0,255}); material->map=texture;
        renderHostTurn(renderer, scene,camera);
        auto alpha=texture->clone(); material->alphaMap=alpha; material->blending=NoBlending;
        std::get<Uint8Array>(texture->image->data).set(Uint8Array{0,255,0,128});
        renderHostTurn(renderer, scene,camera);
        expectPixel(renderer,{0,255,0,128},"Shared map/alphaMap storage observes the later upload in both bindings");
        material->alphaMap.reset(); material->blending=NormalBlending;
    }
    {
        auto texture=rgba(Uint8Array{255,0,0,255}); material->map=texture;
        texture->version=.25; texture->source->version=.5;
        renderHostTurn(renderer, scene,camera);
        expectPixel(renderer,{255,0,0,255},"Positive fractional texture versions upload without integer truncation");
        std::get<Uint8Array>(texture->image->data).set(Uint8Array{0,255,0,255});
        texture->version=.5; texture->source->version=.75;
        renderHostTurn(renderer, scene,camera);
        expectPixel(renderer,{0,255,0,255},"Fractional source and texture version changes remain distinct");
        std::get<Uint8Array>(texture->image->data).set(Uint8Array{0,0,255,255});
        texture->version=std::numeric_limits<double>::quiet_NaN(); texture->source->version=1;
        renderHostTurn(renderer, scene,camera);
        expectPixel(renderer,{0,255,0,255},"NaN texture versions do not satisfy the upstream upload guard");
        texture->version=-1;
        renderHostTurn(renderer, scene,camera);
        expectPixel(renderer,{0,255,0,255},"Negative texture versions preserve the currently uploaded binding");
        texture->version=1;
        renderHostTurn(renderer, scene,camera);
        expectPixel(renderer,{0,0,255,255},"Returning to a positive version resumes texture updates");
    }
    {
        scene.clear(); material->map.reset(); renderHostTurn(renderer, scene,camera);
        auto texture=rgba(Uint8Array{255,0,0,255});
        auto first=plane(Color(1,1,1)), second=plane(Color(1,1,1));
        first->scale.set(.5,1,1); second->scale.set(.5,1,1);
        first->position.x=-.5; second->position.x=.5;
        basic(first)->map=texture; basic(second)->map=texture;
        first->renderOrder=0; second->renderOrder=1;
        first->onAfterRender=[&](auto&,auto&,auto&,auto*,auto*,auto*) {
            std::get<Uint8Array>(texture->image->data).set(Uint8Array{0,255,0,255}); texture->needsUpdate=true;
        };
        scene.add(first); scene.add(second);
        renderHostTurn(renderer, scene,camera);
        expectPixel(renderer,{255,0,0,255},"A same-frame texture update preserves an earlier recorded draw",16,32);
        expectPixel(renderer,{0,255,0,255},"A same-frame texture update affects the following draw",48,32);
        require(renderer.getInfo().textureImages==1,"Superseded same-frame texture resources retire after GPU completion");
        first->onAfterRender=[&](auto&,auto&,auto&,auto*,auto*,auto*) { texture->dispose(); };
        renderHostTurn(renderer, scene,camera);
        expectPixel(renderer,{0,255,0,255},"Disposal during a frame preserves previously recorded texture references",16,32);
        expectPixel(renderer,{0,255,0,255},"A later draw recreates a texture disposed by the earlier hook",48,32);
        scene.clear();
    }
    material->map.reset(); scene.add(mesh); renderHostTurn(renderer, scene,camera);
    require(renderer.getInfo().textureImages==0,"Texture frame fixtures leave no owned images");
    {
        auto texture=rgba(Uint8Array{255,0,0,255}); material->map=texture;
        int callbacks=0; texture->onUpdate=[&](Texture&){++callbacks;};
        mesh->geometry->setDrawRange(0,0);
        renderHostTurn(renderer, scene,camera);
        require(callbacks==1 && renderer.getInfo().calls==1,"Zero-count draws still upload textures and execute a draw call");
        texture->needsUpdate=true; mesh->geometry->setDrawRange(9,0);
        renderHostTurn(renderer, scene,camera);
        require(callbacks==2 && renderer.getInfo().calls==0,"Texture uploads precede rejection of negative clipped draw ranges");
        mesh->geometry->setDrawRange(0,std::numeric_limits<double>::infinity());
        texture->onUpdate=[&](Texture& value) { value.dispose(); value.onUpdate=nullptr; };
        texture->needsUpdate=true;
        renderHostTurn(renderer, scene,camera);
        expectPixel(renderer,{255,0,0,255},"Disposing a texture from its upload callback keeps the current draw alive");
        require(renderer.getInfo().textureImages==0,"Upload callback disposal does not leave a reinserted cache entry");
        texture->needsUpdate=true; renderHostTurn(renderer, scene,camera);
        expectPixel(renderer,{255,0,0,255},"A texture disposed during upload can be used again");
    }
    {
        auto texture=rgba(Uint8Array{255,0,0,255}); material->map=texture;
        const auto reject=[&](const char* message) {
            bool rejected=false;
            try {renderHostTurn(renderer, scene,camera);} catch(const std::invalid_argument&) {rejected=true;}
            require(rejected,message);
        };
        texture->magFilter=1234;
        reject("Unsupported texture filter fails explicitly");
        texture->magFilter=NearestFilter; texture->image->width=2;
        reject("Texture upload rejects short typed arrays before any GPU out-of-bounds access");
        texture->image->width=1;
        renderHostTurn(renderer, scene,camera);
        expectPixel(renderer,{255,0,0,255},"A failed first upload can recover with valid image data");
        texture->mipmaps->push_back(std::make_shared<TextureImage>(Uint8Array{255,0,0,255},-1,1));
        texture->needsUpdate=true;
        reject("Manual mipmap uploads reject negative dimensions");
        texture->mipmaps->clear();
        texture->image->width=2;
        texture->image->data=Uint8Array{255,0,0,255,0,255,0,255};
        reject("Resizing immutable uploaded texture storage requires disposal");
        texture->dispose();
        renderHostTurn(renderer, scene,camera);
        uv(.75,.25);
        renderHostTurn(renderer, scene,camera);
        expectPixel(renderer,{0,255,0,255},"Disposal permits reallocation with changed texture dimensions");
        texture->addUpdateRange(4,8); texture->needsUpdate=true;
        reject("An update range crossing its image row fails explicitly");
        texture->clearUpdateRanges();
        renderHostTurn(renderer, scene,camera);
        expectPixel(renderer,{0,255,0,255},"The renderer recovers after a rejected texture update range");
        texture->channel=4;
        reject("Unsupported UV channel fails explicitly");
        texture->channel=0;
        auto tooShort=std::make_shared<Float32BufferAttribute>(std::vector<float>{0,0},2);
        mesh->geometry->setAttribute("uv",tooShort);
        renderHostTurn(renderer, scene,camera);
        require(pixel(renderer,32,32)==std::array<int,4>{0,0,0,255} && renderer.getInfo().calls==1,
                "Out-of-bounds UV fetch is a draw error without GPU submission");
        mesh->geometry->setAttribute("uv",std::make_shared<PlaneGeometry>(1.6,1.6)->getAttribute("uv"));
        renderHostTurn(renderer, scene,camera);
        require(renderer.getInfo().calls==1,"UV fetch validation failure leaves the next frame usable");
    }
    {
        auto texture=rgba(Uint8Array{255,0,0,255}); material->map=texture;
        auto local=std::make_unique<VulkanRenderer>(VulkanRendererParameters{64,64,validation});
        local->render(scene,camera);
        int disposes=0; texture->addEventListener("dispose",[&](auto&){++disposes;});
        local.reset(); texture->dispose();
        require(disposes==1,"Renderer destruction unregisters texture dispose listeners");
    }
    material->map.reset(); renderHostTurn(renderer, scene,camera);
    require(renderer.getInfo().textureImages==0,"Texture tests release application-owned cached images");
    require(renderer.getValidationMessages().empty(),"Texture rendering emitted no validation warning or error");
}

void mipmapTests(bool validation) {
    VulkanRenderer renderer({64,64,validation}); renderer.srgbOutput=false;
    OrthographicCamera camera(-1,1,1,-1,.1,20);camera.position.z=5;
    Scene scene;auto mesh=std::make_shared<Mesh>(std::make_shared<PlaneGeometry>(2,2),std::make_shared<MeshBasicMaterial>(Color(1,1,1)));
    const auto material=basic(mesh);material->blending=NoBlending;scene.add(mesh);
    const auto solid=[](unsigned width,unsigned height,std::array<int,4> color) {
        Uint8Array pixels(std::size_t(width)*height*4);
        for(std::size_t i=0;i<pixels.size();++i)pixels[i]=color[i%4];
        return std::make_shared<TextureImage>(pixels,width,height);
    };
    const auto texture=[&](unsigned width=8,unsigned height=4) {
        auto value=std::make_shared<DataTexture>(solid(width,height,{255,0,0,255})->data,width,height);
        value->minFilter=NearestMipmapNearestFilter;value->wrapS=value->wrapT=RepeatWrapping;value->needsUpdate=true;
        return value;
    };
    const auto sample=[&](const std::shared_ptr<Texture>& value,double repeat,std::array<int,4> expected,const char* message) {
        material->map=value;value->repeat.set(repeat,repeat);renderHostTurn(renderer, scene,camera);expectPixel(renderer,expected,message);
    };
    {
        auto value=texture();*value->mipmaps={solid(8,4,{255,0,0,255}),solid(4,2,{0,255,0,255}),
            solid(2,1,{0,0,255,255}),solid(1,1,{255,255,0,255})};
        int updates=0;value->onUpdate=[&](Texture&){++updates;};
        sample(value,1,{255,0,0,255},"Manual mipmap level zero supplies the base texels");
        sample(value,16,{0,255,0,255},"Minification selects manual mipmap level one");
        sample(value,32,{0,0,255,255},"Minification selects manual mipmap level two");
        sample(value,64,{255,255,0,255},"Minification selects the final rectangular mipmap level");
        require(updates==1,"Sampling different mip levels does not upload again");
        value->mipmaps->at(2)->data=solid(2,1,{255,0,255,255})->data;
        sample(value,32,{0,0,255,255},"Manual mip mutations wait for needsUpdate");
        value->addUpdateRange(0,1);value->needsUpdate=true;
        sample(value,32,{255,0,255,255},"needsUpdate uploads all manual mip levels");
        require(updates==2 && value->updateRanges->size()==1,"Manual mip uploads ignore and retain base updateRanges");
        const auto images=renderer.getInfo().textureImages;
        auto clone=value->clone();int cloneUpdates=0;clone->onUpdate=[&](Texture&){++cloneUpdates;};
        sample(clone,32,{255,0,255,255},"A cloned texture samples shared manual mip storage");
        require(renderer.getInfo().textureImages==images && cloneUpdates==1,"Matching Source and texture parameters share one mip image");
        clone->mipmaps->at(2)->data=solid(2,1,{0,255,255,255})->data;clone->needsUpdate=true;
        sample(clone,32,{0,255,255,255},"A clone uploads new shared manual mip data");
        sample(value,32,{0,255,255,255},"An unchanged texture handle observes its shared Source mip upload");
        clone->wrapS=MirroredRepeatWrapping;clone->needsUpdate=true;
        sample(clone,32,{0,255,255,255},"Changed sampler parameters create an independent manual mip image");
        require(renderer.getInfo().textureImages==images+1,"Different texture parameter keys retain separate mip resources");
        clone->dispose();require(renderer.getInfo().textureImages==images,"Disposing one mip key preserves another owner");
        value->dispose();sample(value,32,{0,255,255,255},"Disposed manual mip textures upload again");
        require(updates==3,"Manual mip disposal restores the initial upload callback");
        value->mipmaps->resize(1);value->clearUpdateRanges();value->needsUpdate=true;
        sample(value,32,{0,255,255,255},"Removing mip descriptors preserves the allocated untouched levels");
        value->mipmaps->clear();value->image->data=solid(8,4,{0,0,255,255})->data;value->needsUpdate=true;
        sample(value,32,{0,255,255,255},"A later base-only upload preserves old manual mip levels");
        sample(value,1,{0,0,255,255},"The base-only update still changes level zero");
    }
    {
        auto value=texture();*value->mipmaps={solid(8,4,{255,0,0,255}),solid(4,2,{0,255,0,255}),solid(2,1,{0,0,255,255})};
        value->minFilter=NearestMipmapLinearFilter;
        sample(value,16*std::sqrt(2.0),{0,128,128,255},"Linear mip filtering interpolates adjacent supplied mip levels");
        value->mipmaps->at(0)=solid(4,2,{255,255,255,255});value->needsUpdate=true;
        sample(value,1,{255,0,0,255},"A smaller base mip subimage preserves texels outside the updated rectangle");
        value->offset.set(-.4,-.4);
        sample(value,1,{255,255,255,255},"A smaller base mip subimage updates the origin without resizing storage");
    }
    {
        auto value=texture();value->generateMipmaps=true;value->mipmapsAutoUpdate=false;
        int updates=0;value->onUpdate=[&](Texture&){++updates;};
        sample(value,64,{0,0,0,0},"Disabled automatic generation leaves newly allocated mip levels zero initialized");
        sample(value,1,{255,0,0,255},"Disabling automatic mip updates still uploads the base level");
        value->image->data=solid(8,4,{0,255,0,255})->data;value->needsUpdate=true;
        sample(value,64,{0,0,0,0},"A version update does not generate mips when mipmapsAutoUpdate is false");
        sample(value,1,{0,255,0,255},"Base pixels update independently of automatic mip generation");
        require(updates==2 && value->generateMipmaps,"Disabled auto update retains the requested generateMipmaps flag");
        value->mipmapsAutoUpdate=true;value->needsUpdate=true;
        sample(value,64,{0,255,0,255},"Enabling automatic mip updates generates the complete chain");
    }
    {
        auto value=texture();value->generateMipmaps=true;
        sample(value,64,{255,0,0,255},"Generated levels exist before adding manual mip descriptors");
        *value->mipmaps={solid(8,4,{0,0,255,255}),solid(4,2,{0,255,0,255})};
        bool callbackSawManual=false;value->onUpdate=[&](Texture& t){callbackSawManual=!t.generateMipmaps;};value->needsUpdate=true;
        sample(value,64,{255,0,0,255},"Manual subuploads preserve previously generated higher mip levels");
        require(callbackSawManual,"Manual upload clears generateMipmaps before onUpdate");
        sample(value,16,{0,255,0,255},"Manual subuploads replace their own level without regenerating the tail");
        value->needsUpdate=true;
        sample(value,64,{0,255,0,255},"The next upload observes the changed generation cache key and allocates the manual level count");
    }
    {
        auto value=texture(7,3);*value->mipmaps={solid(7,3,{255,0,0,255}),solid(3,1,{0,255,0,255}),solid(1,1,{0,0,255,255})};
        sample(value,16,{0,255,0,255},"Non-power-of-two manual mips use floor-halved dimensions");
        sample(value,64,{0,0,255,255},"Non-power-of-two manual mips reach the final 1x1 level");
        value->mipmaps->at(1)=solid(1,1,{255,255,0,255});value->needsUpdate=true;
        sample(value,16,{255,255,0,255},"A smaller manual mip subimage updates its origin");
        value->offset.x=.5;
        sample(value,16,{0,255,0,255},"A smaller mip subimage preserves texels beyond the updated rectangle");
        int callbacks=0;value->onUpdate=[&](Texture&){++callbacks;};
        value->mipmaps->at(0)=std::make_shared<TextureImage>(Uint8Array(0),0,3);
        value->mipmaps->at(1)=std::make_shared<TextureImage>(Uint8Array(0),3,0);value->needsUpdate=true;
        sample(value,16,{0,255,0,255},"Zero-height manual mip updates preserve already uploaded texels");
        sample(value,1,{255,0,0,255},"Zero-width base mip updates preserve immutable storage and base texels");
        require(callbacks==1,"Zero-sized manual subuploads still complete the upload callback once");
        auto initial=texture();*initial->mipmaps={solid(8,4,{255,0,0,255}),std::make_shared<TextureImage>(Uint8Array(0),0,0)};
        sample(initial,16,{0,0,0,0},"An empty higher-level upload leaves new mip storage zero initialized");
    }
    {
        auto value=std::make_shared<DataTexture>(nullptr,2,2);value->minFilter=NearestMipmapNearestFilter;
        value->wrapS=value->wrapT=RepeatWrapping;value->colorSpace=SRGBColorSpace;
        *value->mipmaps={solid(2,2,{255,0,0,255}),solid(1,1,{128,128,128,255})};value->needsUpdate=true;
        sample(value,128,{55,55,55,255},"Manual sRGB mip texels decode to the linear working color space");
        auto floating=std::make_shared<DataTexture>(nullptr,2,2,RGBAFormat,FloatType);
        floating->minFilter=NearestMipmapNearestFilter;floating->wrapS=floating->wrapT=RepeatWrapping;
        *floating->mipmaps={std::make_shared<TextureImage>(Float32Array(16),2,2),
            std::make_shared<TextureImage>(Float32Array{.25f,.5f,.75f,1.f},1,1)};floating->needsUpdate=true;
        sample(floating,128,{64,128,191,255},"Float32 manual mip levels retain their own texels and dimensions");
        auto half=std::make_shared<DataTexture>(nullptr,2,2,RGBAFormat,HalfFloatType);
        half->minFilter=NearestMipmapNearestFilter;half->wrapS=half->wrapT=RepeatWrapping;
        *half->mipmaps={std::make_shared<TextureImage>(Uint16Array(16),2,2),
            std::make_shared<TextureImage>(Uint16Array{toHalfFloat(.5),toHalfFloat(.25),toHalfFloat(1),toHalfFloat(1)},1,1)};half->needsUpdate=true;
        sample(half,128,{128,64,255,255},"HalfFloat manual mip levels retain the supported half storage format");
        auto rg=std::make_shared<DataTexture>(nullptr,4,4,RGFormat);rg->minFilter=NearestMipmapNearestFilter;
        rg->wrapS=rg->wrapT=RepeatWrapping;rg->unpackAlignment=8;rg->offset.y=.5;
        *rg->mipmaps={std::make_shared<TextureImage>(Uint8Array(32),4,4),
            std::make_shared<TextureImage>(Uint8Array{64,128,64,128,0,0,0,0,192,32,192,32},2,2)};rg->needsUpdate=true;
        sample(rg,32,{64,128,0,255},"Each manual mip uses its own unpackAlignment row stride");
        rg->flipY=true;rg->needsUpdate=true;
        sample(rg,32,{192,32,0,255},"Manual mip uploads flip rows independently at each level");
    }
    {
        auto value=texture();*value->mipmaps={solid(8,4,{255,0,0,255}),solid(4,2,{0,255,0,255})};
        value->source->dataReady=false;value->generateMipmaps=true;value->addUpdateRange(0,4);
        int updates=0;value->onUpdate=[&](Texture& t){++updates;require(!t.generateMipmaps,"Deferred manual upload also disables generation before callback");};
        const auto uploads=renderer.getInfo().textureUploads;
        sample(value,16,{0,0,0,0},"dataReady false allocates zero initialized manual mip levels");
        require(renderer.getInfo().textureUploads==uploads && updates==1 && value->updateRanges->size()==1,
                "Deferred manual mip allocation preserves pending ranges and records only its callback");
        value->source->dataReady=true;value->needsUpdate=true;
        sample(value,16,{0,255,0,255},"Ready manual mip data uploads on a subsequent version");
        value->onUpdate=[&](Texture& t){++updates;t.mipmaps->at(1)->data=solid(4,2,{0,0,255,255})->data;t.needsUpdate=true;};
        value->needsUpdate=true;
        sample(value,16,{0,255,0,255},"Manual mip callback runs after all level data is captured");
        sample(value,16,{0,255,0,255},"Callback version changes are recorded after the manual upload");
        require(updates==3,"A callback version change does not automatically reupload manual levels");
        value->onUpdate=[&](Texture& t){t.dispose();t.onUpdate=nullptr;};value->needsUpdate=true;
        sample(value,16,{0,0,255,255},"Disposal from a manual mip callback keeps the current sampled image alive");
    }
    {
        auto zero=texture();*zero->mipmaps={solid(0,1,{255,0,0,255})};material->map=zero;
        bool zeroRejected=false;try{renderHostTurn(renderer, scene,camera);}catch(const std::invalid_argument&){zeroRejected=true;}
        require(zeroRejected,"Initial immutable mipmap storage cannot have zero dimensions");
        auto value=texture();*value->mipmaps={solid(8,4,{255,0,0,255}),solid(5,2,{0,255,0,255})};material->map=value;
        bool rejected=false;try{renderHostTurn(renderer, scene,camera);}catch(const std::invalid_argument&){rejected=true;}
        require(rejected,"Manual mip dimensions cannot exceed their allocated level");
        value->mipmaps->at(1)=std::make_shared<TextureImage>(Uint8Array{0,255,0,255},4,2);
        rejected=false;try{renderHostTurn(renderer, scene,camera);}catch(const std::invalid_argument&){rejected=true;}
        require(rejected,"Each manual mip validates its own typed-array byte length");
        value->mipmaps->at(1)=solid(4,2,{0,255,0,255});
        sample(value,16,{0,255,0,255},"A rejected first manual upload can recover without leaked cache resources");
        value->mipmaps->push_back(solid(2,1,{0,0,255,255}));value->needsUpdate=true;
        rejected=false;try{renderHostTurn(renderer, scene,camera);}catch(const std::invalid_argument&){rejected=true;}
        require(rejected,"Adding a mip beyond immutable storage requires disposal");
        value->dispose();sample(value,64,{0,0,255,255},"Disposal permits a larger manual mip allocation");
    }
    material->map.reset();renderHostTurn(renderer, scene,camera);
    require(renderer.getInfo().textureImages==0,"Manual mip fixtures release every application-owned image");
    require(renderer.getValidationMessages().empty(),"Manual mip uploads emitted no Vulkan validation warning or error");
}

void renderOrderTests(bool validation) {
    VulkanRenderer renderer({64,64,validation});renderer.srgbOutput=false;
    OrthographicCamera camera(-1,1,1,-1,.1,20);camera.position.z=5;
    Scene scene;
    auto red=plane(Color(1,0,0)),green=plane(Color(0,1,0));
    for(const auto& mesh:{red,green}){basic(mesh)->depthTest=false;basic(mesh)->depthWrite=false;}
    auto high=std::make_shared<Group>(),low=std::make_shared<Group>();high->add(red);low->add(green);
    high->renderOrder=.75;low->renderOrder=.25;scene.add(high);scene.add(low);
    renderHostTurn(renderer, scene,camera);
    expectPixel(renderer,{255,0,0,255},"Fractional Group renderOrder remains distinct for opaque sorting");
    basic(red)->transparent=basic(green)->transparent=true;
    renderHostTurn(renderer, scene,camera);
    expectPixel(renderer,{255,0,0,255},"Fractional Group renderOrder remains distinct for transparent sorting");
    high->renderOrder=std::numeric_limits<double>::quiet_NaN();low->renderOrder=0;
    red->renderOrder=10;green->renderOrder=-10;
    renderHostTurn(renderer, scene,camera);
    expectPixel(renderer,{0,255,0,255},"NaN group comparison returns equality before consulting per-object renderOrder");
    high->renderOrder=0;red->renderOrder=std::numeric_limits<double>::quiet_NaN();
    renderHostTurn(renderer, scene,camera);
    expectPixel(renderer,{0,255,0,255},"NaN object renderOrder preserves collected order instead of consulting depth or id");
    basic(red)->transparent=basic(green)->transparent=false;
    red->renderOrder=0;green->renderOrder=1;
    green->geometry=std::make_shared<PlaneGeometry>(1.6,1.6);
    std::dynamic_pointer_cast<BufferAttribute>(green->geometry->getAttribute("position"))->onUpload([&]{red->renderOrder=2;});
    renderHostTurn(renderer, scene,camera);
    expectPixel(renderer,{0,255,0,255},"Queued renderOrder values remain snapshots when later uploads mutate objects");
    renderHostTurn(renderer, scene,camera);
    expectPixel(renderer,{255,0,0,255},"The next frame collects the changed renderOrder");

    scene.clear();
    const std::vector<double> keys{7,NAN,3,1,NAN,9,0,8,4,NAN,2,6,5};
    const std::vector<int> expected{0,1,6,3,2,4,10,8,12,11,7,5,9};
    std::vector<int> draws;
    for (int i=0;i<int(keys.size());++i) {
        auto object=plane(i==9?Color(0,0,1):Color(1,0,0));
        object->renderOrder=keys[i];
        basic(object)->depthTest=false;basic(object)->depthWrite=false;
        object->onBeforeRender=[&,i](auto&,auto&,auto&,auto*,auto*,auto*){draws.push_back(i);};
        scene.add(object);
    }
    renderHostTurn(renderer, scene,camera);
    require(draws==expected,"Mixed NaN opaque render orders follow the pinned V8 comparison algorithm");
    expectPixel(renderer,{0,0,255,255},"Opaque nontransitive sorting preserves the upstream final draw");
    for (const auto& object:scene.children) std::static_pointer_cast<Mesh>(object)->material->transparent=true;
    draws.clear();renderHostTurn(renderer, scene,camera);
    require(draws==expected,"Mixed NaN transparent render orders follow the pinned V8 comparison algorithm");
    expectPixel(renderer,{0,0,255,255},"Transparent nontransitive sorting preserves the upstream final draw");
}

void attributeRangeSortTests(bool validation) {
    VulkanRenderer renderer({64,64,validation});renderer.srgbOutput=false;
    OrthographicCamera camera(-1,1,1,-1,.1,20);camera.position.z=5;
    Scene scene;auto mesh=plane(Color(1,1,1));basic(mesh)->vertexColors=true;
    auto colors=std::make_shared<Float32BufferAttribute>(12.0,3);
    for(int i=0;i<4;++i)colors->setXYZ(i,1,0,0);
    mesh->geometry->setAttribute("color",colors);scene.add(mesh);renderHostTurn(renderer, scene,camera);
    for(int i=0;i<4;++i)colors->setXYZ(i,0,1,0);
    // These counts and upload order are observed from the pinned WebGLAttributes.
    const std::vector<double> starts{2,NAN,1,7,NAN,0,9,4,6,3,NAN,8,5,10};
    for(const auto start:starts)colors->addUpdateRange(start,1);
    const auto handles=colors->updateRanges.value().get("slice").call(colors->updateRanges.value());
    std::vector<double> callbackCounts;bool cleared=false;
    colors->onUpload([&]{
        cleared=colors->updateRanges.get("length").strictEquals(0);
        for(std::size_t i=0;i<handles.length();++i)callbackCounts.push_back(handles.get(std::to_string(i)).get("count").toNumber());
    });
    colors->needsUpdate=true;renderHostTurn(renderer, scene,camera);
    require(callbackCounts==std::vector<double>{1,1,7,1,1,1,1,1,1,1,1,3,1,1},
            "NaN updateRange sorting and merging mutate the same range identities as WebGLAttributes");
    require(cleared,"NaN range uploads clear the original range list before the upload callback");
    expectPixel(renderer,{0,255,0,255},"Nontransitive range sorting uploads the upstream selected components");
}

void attributeNumberTests(bool validation) {
    VulkanRenderer renderer({64, 64, validation});
    renderer.srgbOutput = false;
    renderer.setClearColor(Color(0, 0, 0), 1);
    OrthographicCamera camera(-1, 1, 1, -1, .1, 20);
    camera.position.set(0, 0, 5);
    Scene scene;
    auto mesh = plane(Color(1, 1, 1));
    basic(mesh)->vertexColors = true;
    auto colors = std::make_shared<Float32BufferAttribute>(12.0, 3);
    const auto paint = [&](double r, double g, double b) {
        for (int i = 0; i < 4; ++i) colors->setXYZ(i, r, g, b);
    };
    paint(1, 0, 0);
    mesh->geometry->setAttribute("color", colors);
    scene.add(mesh);
    std::vector<int> uploads;
    colors->onUpload([state = 0, &uploads]() mutable { uploads.push_back(++state); });
    colors->version = .25;
    renderHostTurn(renderer, scene, camera);
    expectPixel(renderer, {255, 0, 0, 255}, "Fractional attribute versions preserve initial uploads");
    paint(0, 1, 0); colors->version = .5;
    renderHostTurn(renderer, scene, camera);
    expectPixel(renderer, {0, 255, 0, 255}, "Increasing fractional attribute versions upload new bytes");
    paint(0, 0, 1); colors->version = .4;
    renderHostTurn(renderer, scene, camera);
    expectPixel(renderer, {0, 255, 0, 255}, "Decreasing attribute versions retain GPU data");
    colors->version = std::numeric_limits<double>::quiet_NaN();
    renderHostTurn(renderer, scene, camera);
    expectPixel(renderer, {0, 255, 0, 255}, "NaN attribute versions fail the upstream less-than comparison");
    colors->version = 1;
    renderHostTurn(renderer, scene, camera);
    expectPixel(renderer, {0, 0, 255, 255}, "A greater numeric attribute version resumes uploads");
    require(uploads == std::vector<int>{1, 2, 3}, "Upload callbacks preserve their mutable function state");
    const auto ranges = colors->updateRanges.value();
    colors->addUpdateRange(2, 1); colors->addUpdateRange(0, 1);
    const auto merged = ranges.get("1"), later = ranges.get("0");
    colors->version = 2;
    renderHostTurn(renderer, scene, camera);
    require(merged.get("count").strictEquals(3) && merged.get("start").strictEquals(0) && later.get("count").strictEquals(1),
            "Attribute range merging changes the original range object after sorting handles");
    require(ranges.sameValue(colors->updateRanges.value()) && ranges.get("length").strictEquals(0),
            "Clearing consumed attribute ranges preserves their array identity");
    paint(0, 1, 0); colors->addUpdateRange(0, 0); colors->version = 3;
    renderHostTurn(renderer, scene, camera);
    expectPixel(renderer, {0, 255, 0, 255}, "Zero range counts upload the remaining attribute array");

    auto expected = std::get<Float32Array>(colors->array).slice();
    paint(1, 0, 0);
    auto source = std::get<Float32Array>(colors->array);
    Uint8Array expectedBytes(expected.buffer);
    expectedBytes.set(Uint8Array(source.buffer).subarray(0, 44), 1);
    colors->addUpdateRange(.25, 11.75); colors->version = 4;
    renderHostTurn(renderer, scene, camera);
    const auto fractionalImage = renderer.readPixels();
    mesh->geometry->setAttribute("color", std::make_shared<BufferAttribute>(expected, 3));
    renderHostTurn(renderer, scene, camera);
    require(fractionalImage == renderer.readPixels(),
            "Fractional starts convert byte and element offsets independently before upload");
    mesh->geometry->setAttribute("color", colors);
    paint(0, 0, 1);
    colors->addUpdateRange(std::numeric_limits<double>::quiet_NaN(),
                           std::numeric_limits<double>::quiet_NaN());
    colors->version = 5;
    renderHostTurn(renderer, scene, camera);
    expectPixel(renderer, {0, 0, 255, 255}, "WebIDL converts NaN offsets and counts to zero");
    paint(1, 0, 0); colors->addUpdateRange(-0.0, -4294967296.0); colors->version = 6;
    renderHostTurn(renderer, scene, camera);
    expectPixel(renderer, {255, 0, 0, 255}, "Attribute range counts use WebIDL unsigned 32-bit conversion");
    require(uploads == std::vector<int>{1, 2, 3, 4, 5, 6, 7, 8},
            "Each accepted attribute version invokes the same upload callback once");
    paint(0, 1, 0); colors->addUpdateRange(-.25, 1); colors->version = 7;
    renderHostTurn(renderer, scene, camera);
    expectPixel(renderer, {255, 0, 0, 255}, "Negative byte offsets produce a WebGL error without writing bytes");
    require(colors->updateRanges.get("length").strictEquals(0) && uploads.back() == 9,
            "WebGL range errors still clear ranges and invoke the upload callback");
    renderHostTurn(renderer, scene, camera);
    require(uploads.back() == 9, "WebGL range errors still advance the cached version");
    colors->clearUpdateRanges(); colors->onUploadCallback = nullptr;
    colors->needsUpdate = true;
    bool nullCallback = false;
    try { renderHostTurn(renderer, scene, camera); }
    catch (const JSTypeError&) { nullCallback = true; }
    require(nullCallback, "An explicit null attribute upload callback raises TypeError");
    colors->onUpload([] {});
    renderHostTurn(renderer, scene, camera);
    expectPixel(renderer, {0, 255, 0, 255}, "Attribute uploads recover after a callback exception");
    auto initialNull = std::make_shared<BufferAttribute>(std::get<Float32Array>(colors->array), 3);
    initialNull->onUploadCallback = nullptr;
    mesh->geometry->setAttribute("color", initialNull);
    nullCallback = false;
    try { renderHostTurn(renderer, scene, camera); }
    catch (const JSTypeError&) { nullCallback = true; }
    require(nullCallback, "First-time attribute uploads also call an explicit null callback");
    scene.clear();
    auto grouped = std::make_shared<PlaneGeometry>(1.6, 1.6);
    auto groupMaterial = std::make_shared<MeshBasicMaterial>(Color(1, 0, 0));
    auto groupMesh = std::make_shared<Mesh>(grouped, std::vector<std::shared_ptr<Material>>{groupMaterial});
    grouped->addGroup(0, 6, std::numeric_limits<double>::quiet_NaN());
    scene.add(groupMesh);
    renderHostTurn(renderer, scene, camera);
    require(renderer.getInfo().calls == 0, "NaN material indices do not resolve a material array entry");
    grouped->groups[0].materialIndex = -.25;
    renderHostTurn(renderer, scene, camera);
    require(renderer.getInfo().calls == 0, "Negative fractional material indices preserve missing-property behavior");
    grouped->groups[0].materialIndex = 0;
    grouped->groups[0].count = 2.5;
    renderHostTurn(renderer, scene, camera);
    require(renderer.getInfo().triangles == 2.5 / 3,
            "Fractional group counts survive until the Vulkan draw-count conversion");
    expectPixel(renderer, {0, 0, 0, 255}, "An incomplete grouped triangle produces no fragments");
    grouped->groups[0].count = 6;
    grouped->setDrawRange(0, 2.5);
    renderHostTurn(renderer, scene, camera);
    require(renderer.getInfo().triangles == 2.5 / 3,
            "Fractional drawRange counts preserve upstream statistics");
    grouped->setDrawRange(0, std::numeric_limits<double>::infinity());
    renderHostTurn(renderer, scene, camera);
    expectPixel(renderer, {255, 0, 0, 255}, "Infinite drawRange counts clip to the geometry extent");
}

void rendererInfoTests(bool validation) {
    VulkanRenderer renderer({64,64,validation});
    renderer.srgbOutput = false;
    OrthographicCamera camera(-1,1,1,-1,.1,20);
    camera.position.z = 5;
    Scene scene;
    auto mesh = plane(Color(0,1,0));
    mesh->frustumCulled = false;
    scene.add(mesh);
    const auto info = renderer.info;
    auto render = info.get("render");
    require(info.get("autoReset").strictEquals(true) && render.get("frame").strictEquals(0),
            "Renderer exposes the original WebGLInfo defaults");
    renderHostTurn(renderer, scene,camera);
    require(render.get("frame").strictEquals(1) && render.get("calls").strictEquals(1) &&
            render.get("triangles").strictEquals(2) && render.get("points").strictEquals(0),
            "Real Vulkan draws update the managed statistics object");
    require(info.get("memory").get("geometries").strictEquals(1) &&
            info.get("memory").get("textures").strictEquals(0),
            "Geometry registration updates logical memory without counting fallback textures");
    auto mutableInfo = info;
    mutableInfo.set("autoReset",false);
    renderHostTurn(renderer, scene,camera);
    require(render.get("frame").strictEquals(2) && render.get("calls").strictEquals(2) &&
            render.get("triangles").strictEquals(4), "autoReset false accumulates multiple renders");
    mutableInfo.set("autoReset",1);
    renderHostTurn(renderer, scene,camera);
    require(render.get("calls").strictEquals(3) && render.get("triangles").strictEquals(6),
            "Only strict true automatically resets the statistics");
    const auto originalReset = info.get("reset"), originalUpdate = info.get("update");
    originalReset.call(info);
    require(render.get("calls").strictEquals(0) && render.get("triangles").strictEquals(0) &&
            render.get("frame").strictEquals(3), "Explicit reset preserves the frame counter");
    render.set("triangles","prefix:"); render.set("calls","4");
    renderHostTurn(renderer, scene,camera);
    require(render.get("triangles").strictEquals("prefix:2") && render.get("calls").strictEquals(5),
            "Draw statistics preserve compound addition and increment conversions");

    auto events = JSValue::array();
    mutableInfo.set("autoReset",true);
    mutableInfo.set("reset",JSValue::function(+[](const JSValue& receiver,const auto&,const auto& c)->JSValue {
        auto trace=c[0]; trace.get("push").call(trace,{receiver.get("render").get("frame")});
        c[1].call(receiver); return {};
    },{events,originalReset}));
    renderHostTurn(renderer, scene,camera);
    require(events.get("0").strictEquals(5) && render.get("calls").strictEquals(1),
            "Renderer increments frame before invoking the live reset method");
    mutableInfo.set("reset",originalReset);

    auto replacement = JSValue::object();
    replacement.set("frame",100); replacement.set("calls",77); replacement.set("triangles",88);
    replacement.set("lines",99); replacement.set("points",111);
    mutableInfo.set("render",replacement);
    renderHostTurn(renderer, scene,camera);
    require(replacement.get("frame").strictEquals(101) && replacement.get("calls").strictEquals(77),
            "Renderer frame lookup observes a replacement public render object");
    require(render.get("frame").strictEquals(5) && render.get("calls").strictEquals(1) &&
            render.get("triangles").strictEquals(2),
            "Reset and draw closures retain the original render object after replacement");
    require(renderer.getInfo().frame==101 && renderer.getInfo().calls==77,
            "The native compatibility snapshot reads current public statistics");
    mutableInfo.set("render",render); mutableInfo.set("autoReset",false);
    mutableInfo.set("update",JSValue::function(+[](const JSValue& receiver,const auto& a,const auto& c)->JSValue {
        auto trace=c[0]; trace.get("push").call(trace,{receiver,a[0],a[1],a[2]}); return {};
    },{events}));
    renderHostTurn(renderer, scene,camera);
    require(events.get("1").sameValue(info) && events.get("2").strictEquals(6) &&
            events.get("3").strictEquals(4) && events.get("4").strictEquals(1),
            "Actual draws invoke a replacement update with receiver, logical count, mode and instances");
    require(render.get("calls").strictEquals(1), "Replacing update controls the original counter mutation");
    expectPixel(renderer,{0,255,0,255},"Replacing statistics methods preserves actual GPU rendering");
    mutableInfo.set("update",JSValue::function(+[](const JSValue&,const auto&,const auto&)->JSValue {
        throw JSTypeError("statistics callback");
    }));
    bool threw=false;
    try { renderHostTurn(renderer, scene,camera); } catch(const JSTypeError&) { threw=true; }
    require(threw,"A statistics callback exception propagates through the Vulkan render path");
    mutableInfo.set("update",originalUpdate); renderHostTurn(renderer, scene,camera);
    require(render.get("calls").strictEquals(2),"Rendering recovers after a statistics callback exception");
    expectPixel(renderer,{0,255,0,255},"Recovered statistics callbacks leave Vulkan command state usable");

    auto texture = std::make_shared<DataTexture>(Uint8Array{255,255,255,255},1,1);
    texture->needsUpdate = true;
    auto clone = texture->clone();
    basic(mesh)->map = texture;
    auto other = plane(Color(0,1,0)); other->frustumCulled = false;
    basic(other)->map = clone; scene.add(other);
    renderHostTurn(renderer, scene,camera);
    const auto memory = info.get("memory");
    require(memory.get("geometries").strictEquals(2) && memory.get("textures").strictEquals(1),
            "Textures sharing Source and cache key count one logical allocation");
    texture->dispose();
    require(memory.get("textures").strictEquals(1),"Disposing one shared texture preserves the remaining logical allocation");
    clone->dispose();
    require(memory.get("textures").strictEquals(0),"Disposing the last texture releases its logical memory count");
    renderHostTurn(renderer, scene,camera);
    require(memory.get("textures").strictEquals(1),"Disposed texture owners can reinitialize shared logical storage");
    clone->wrapS = RepeatWrapping; clone->needsUpdate = true;
    renderHostTurn(renderer, scene,camera);
    require(memory.get("textures").strictEquals(2),"Different cache keys allocate separate logical texture storage");
    clone->dispose(); texture->dispose();
    require(memory.get("textures").strictEquals(0),"Every active Source/cache-key allocation has a matching logical release");
    mesh->geometry->dispose();
    require(memory.get("geometries").strictEquals(1),"Explicit geometry disposal decrements the managed geometry counter");
    auto mutableMemory = memory; mutableMemory.set("geometries","8");
    renderHostTurn(renderer, scene,camera);
    require(memory.get("geometries").strictEquals(9),"Geometry memory increments preserve numeric update conversion");
}

void attributeCacheWeakLifetimeTests(bool validation) {
    VulkanRenderer renderer({64, 64, validation});
    renderer.srgbOutput = false;
    renderer.setClearColor(Color(0, 0, 0), 1);
    OrthographicCamera camera(-1, 1, 1, -1, .1, 20);
    camera.position.z = 5;
    Scene scene;
    auto geometry = std::make_shared<BufferGeometry>();
    auto positions = std::make_shared<Float32BufferAttribute>(
        std::vector<float>{-1,-1,0, 1,-1,0, 0,1,0}, 3);
    auto colors = std::make_shared<Float32BufferAttribute>(
        std::vector<float>{0,1,0, 0,1,0, 0,1,0}, 3);
    auto normals = std::make_shared<Float32BufferAttribute>(
        std::vector<float>{0,0,1, 0,0,1, 0,0,1}, 3);
    geometry->setAttribute("position", positions).setAttribute("color", colors)
        .setAttribute("normal", normals);
    auto material = std::make_shared<MeshBasicMaterial>();
    material->vertexColors = true;
    auto mesh = std::make_shared<Mesh>(geometry, material);
    mesh->frustumCulled = false;
    scene.add(mesh);
    renderHostTurn(renderer, scene, camera);
    expectPixel(renderer, {0,255,0,255}, "Weak upload cache fixture renders actual Vulkan bytes");
    require(renderer.getInfo().attributeUploadBuffers == 3,
            "Three ordinary attributes allocate three raw upload buffers");
    renderHostTurn(renderer, scene, camera);
    require(renderer.getInfo().attributeUploadBuffers == 3,
            "Unchanged versions reuse all raw upload buffers");
    geometry->dispose();
    require(renderer.getInfo().attributeUploadBuffers == 0,
            "Explicit geometry disposal releases every registered raw buffer");
    renderHostTurn(renderer, scene, camera);
    require(renderer.getInfo().attributeUploadBuffers == 3,
            "A disposed geometry can upload its attributes again");
    auto positionObject = nativeAttribute(positions);
    auto colorObject = nativeAttribute(colors);
    auto normalObject = nativeAttribute(normals);
    const std::weak_ptr<BufferAttribute> weakPosition = positions;
    const std::weak_ptr<BufferAttribute> weakColor = colors;
    const std::weak_ptr<BufferAttribute> weakNormal = normals;
    geometry->deleteAttribute("position").deleteAttribute("color").deleteAttribute("normal");
    positions.reset(); colors.reset(); normals.reset();
    require(weakPosition.expired() && weakColor.expired() && weakNormal.expired(),
            "Managed keys and renderer caches do not strongly retain native handles");
    auto heap = JSValue::object();
    heap.collectGarbage();
    require(renderer.getInfo().attributeUploadBuffers == 3,
            "Live managed keys retain their ephemeron upload data after native handle release");
    positionObject = {}; colorObject = {}; normalObject = {};
    require(heap.collectGarbage() > 0, "Unreachable managed attribute keys are collected");
    require(renderer.getInfo().attributeUploadBuffers == 2,
            "Pinned BindingStates retains the used position/color attribute keys and raw buffers after geometry fields are deleted");
    scene.clear();
    renderHostTurn(renderer, scene, camera);
    require(renderer.getInfo().attributeBuffers == 2,
            "The next render preserves derived data belonging to the source binding state's live raw resources");
    require(renderer.getInfo().attributeUploadBuffers == 2,
            "Geometry GC does not release the pinned binding state's referenced buffers");
}

void attributeManagedUploadTests(bool validation) {
    VulkanRenderer renderer({64,64,validation}); renderer.srgbOutput=false;
    OrthographicCamera camera(-1,1,1,-1,.1,20); camera.position.z=5;
    Scene scene; auto mesh=plane(Color(1,1,1)); mesh->frustumCulled=false;
    basic(mesh)->vertexColors=true; scene.add(mesh);
    const auto paint=[](Float32Array values,double r,double g,double b) {
        for(std::size_t i=0;i<values.size();i+=3) { values[i]=r;values[i+1]=g;values[i+2]=b; }
    };
    const auto compareImage=[&](const std::vector<std::uint8_t>& actual,const Float32Array& expected,
                                const std::shared_ptr<BufferAttribute>& original,const char* label) {
        mesh->geometry->setAttribute("color",std::make_shared<BufferAttribute>(expected,3));
        renderHostTurn(renderer, scene,camera);require(actual==renderer.readPixels(),label);
        mesh->geometry->setAttribute("color",original);
    };
    // Distinct prefix/suffix data makes reading backing-buffer offset zero fail.
    Float32Array backing(18);for(std::size_t i=0;i<backing.size();++i)backing[i]=1234;
    auto view=backing.subarray(3,15);paint(view,1,0,0);
    auto colors=std::make_shared<BufferAttribute>(view,3);
    auto object=nativeAttribute(colors);
    auto array=object.get("array");
    JSPropertyDescriptor offset;offset.value=99999;offset.configurable=true;
    array.defineProperty("byteOffset",offset);
    int uploads=0;
    colors->onUpload([&] {
        ++uploads;
        if(uploads==1) { colors->array=Float32Array(24);colors->version=5; }
    });
    mesh->geometry->setAttribute("color",colors);
    renderHostTurn(renderer, scene,camera);
    expectPixel(renderer,{255,0,0,255},"Initial GPU upload captures the original nonzero-offset native view");
    renderHostTurn(renderer, scene,camera);
    expectPixel(renderer,{255,0,0,255},"Callback array replacement is deferred until its version changes");
    require(uploads==1,"Initial upload captures the callback's new version");
    colors->needsUpdate=true;
    bool resized=false;
    try { renderHostTurn(renderer, scene,camera); }
    catch(const std::runtime_error& error) { resized=std::string(error.what()).find("Resizing buffer attributes")!=std::string::npos; }
    require(resized,"First array size remains authoritative after callback replacement");
    colors->array=view; colors->needsUpdate=true;renderHostTurn(renderer, scene,camera);
    require(backing[0]==1234 && backing[17]==1234,"Uploading a view preserves backing-buffer sentinels");

    auto half=std::make_shared<BufferAttribute>(Uint16Array(std::vector<std::uint16_t>{
        0,0,0x3c00,0,0,0x3c00,0,0,0x3c00,0,0,0x3c00}),3);
    auto halfObject=nativeAttribute(half);
    auto callback=JSValue::function(+[](const JSValue& self,const auto&,const auto& captures)->JSValue {
        if(!self.sameValue(captures[0]))throw std::runtime_error("Upload callback receiver changed");
        auto receiver=self;receiver.set("isFloat16BufferAttribute",true);return {};
    },{halfObject});
    half->onUpload(callback);mesh->geometry->setAttribute("color",half);
    renderHostTurn(renderer, scene,camera);
    expectPixel(renderer,{0,0,255,255},"Initial upload chooses HALF_FLOAT after the live callback changes its flag");
    half->onUpload([&] { half->isFloat16BufferAttribute=false; });
    half->needsUpdate=true;renderHostTurn(renderer, scene,camera);
    expectPixel(renderer,{0,0,255,255},"Later callbacks do not change the cached GPU component type");

    paint(view,1,0,0);colors->array=view;colors->onUpload([&]{++uploads;});
    colors->needsUpdate=true;mesh->geometry->setAttribute("color",colors);renderHostTurn(renderer, scene,camera);
    const auto before=uploads;
    auto expected=view.slice();paint(view,0,1,0);
    expected[0]=0;expected[10]=1;
    colors->addUpdateRange(0,1);colors->addUpdateRange(5,-1);colors->addUpdateRange(10,1);
    colors->needsUpdate=true;renderHostTurn(renderer, scene,camera);
    require(uploads==before+1 && colors->updateRanges.get("length").strictEquals(0),
            "An invalid middle range does not skip later writes, clear, or callback");
    compareImage(renderer.readPixels(),expected,colors,
                 "Valid ranges on both sides of a WebGL error reach Vulkan memory independently");
    renderHostTurn(renderer, scene,camera);
    require(uploads==before+1,"A range error advances the upstream cached version");

    // Second range conversion throws only inside bufferSubData. The first
    // command has already written native GPU memory, before callback/version.
    paint(view,1,0,0);colors->needsUpdate=true;renderHostTurn(renderer, scene,camera);
    const auto oldVersion=colors->version.value();const auto beforeThrow=uploads;
    expected=view.slice();paint(view,0,1,0);expected[0]=0;
    colors->addUpdateRange(0,1);colors->addUpdateRange(9,1);
    auto throwing=JSValue::object();throwing.set("valueOf",JSValue::function(
        +[](const JSValue&,const auto&,const auto&)->JSValue {throw JSTypeError("second range conversion");}));
    colors->updateRanges.get("1").set("count",throwing);
    colors->needsUpdate=true;bool failed=false;
    try {renderHostTurn(renderer, scene,camera);}catch(const JSTypeError& error){failed=std::string(error.what())=="second range conversion";}
    require(failed && uploads==beforeThrow && colors->updateRanges.get("length").strictEquals(2),
            "Throwing range conversion preserves pending ranges and skips callback/version commit");
    colors->version=oldVersion;renderHostTurn(renderer, scene,camera);
    compareImage(renderer.readPixels(),expected,colors,
                 "A completed write before an exception survives without another upload");
    colors->clearUpdateRanges();colors->onUpload([]{});

    // Replace the inherited clear method. Upload uses its live lookup and
    // receiver, while retained aliases still observe the original range array.
    auto retained=colors->updateRanges.value();colors->addUpdateRange(0,1);
    auto events=JSValue::array();
    object.set("clearUpdateRanges",JSValue::function(+[](const JSValue& self,const auto&,const auto& captures)->JSValue {
        auto values=captures[0];values.get("push").call(values,{self});
        auto receiver=self;receiver.set("updateRanges",JSValue::array());return {};
    },{events}));
    colors->needsUpdate=true;renderHostTurn(renderer, scene,camera);
    require(events.get("0").sameValue(object) && retained.get("length").strictEquals(1) &&
            !retained.sameValue(colors->updateRanges.value()),
            "GPU uploads invoke replacement clearUpdateRanges on the live native object");

    auto transient=std::make_shared<BufferAttribute>(Float32Array(12),3);
    auto transientArray=std::get<Float32Array>(transient->array);paint(transientArray,0,0,1);
    auto managed=nativeAttribute(transient);const auto identity=transient->id;
    int nativeUploads=0;transient->onUpload([&]{++nativeUploads;});
    mesh->geometry->setAttribute("color",transient);renderHostTurn(renderer, scene,camera);
    std::weak_ptr<BufferAttribute> expired=transient;
    mesh->geometry->deleteAttribute("color");transient.reset();
    require(expired.expired(),"A managed attribute retains storage without retaining its old native handle");
    paint(transientArray,0,1,0);
    auto restored=std::dynamic_pointer_cast<BufferAttribute>(vertexAttribute(managed));
    require(restored->id==identity,"Reconstructing a native handle preserves attribute identity");
    mesh->geometry->setAttribute("color",restored);renderHostTurn(renderer, scene,camera);
    require(nativeUploads==1,"Native handle retirement does not remove the managed upload cache");
    expectPixel(renderer,{0,0,255,255},"Reconstructed handles keep uploaded bytes until needsUpdate");
    restored->needsUpdate=true;renderHostTurn(renderer, scene,camera);
    require(nativeUploads==2,"A reconstructed handle can advance its existing upload cache");
    expectPixel(renderer,{0,255,0,255},"Explicit updates upload the shared storage of a reconstructed handle");

    const auto position=mesh->geometry->getAttribute("position");
    auto positionObject=nativeAttribute(position);auto removal=JSValue::object();removal.set("calls",0);
    JSPropertyDescriptor reentrant;reentrant.configurable=true;
    reentrant.getter=JSValue::function(+[](const JSValue&,const auto&,const auto& captures)->JSValue {
        auto state=captures[1];state.set("calls",state.get("calls").toNumber()+1);
        const auto geometry=std::static_pointer_cast<BufferGeometry>(captures[0].hostPayload("three.BufferGeometry"));
        geometry->deleteAttribute("position");geometry->dispose();return false;
    },{nativeGeometry(mesh->geometry),removal});
    positionObject.defineProperty("isInterleavedBufferAttribute",reentrant);
    const auto memoryBeforeReentry = renderer.info.get("memory").get("geometries").toNumber();
    mesh->geometry->dispose();
    require(removal.get("calls").strictEquals(1) && !mesh->geometry->hasAttribute("position"),
            "Managed removal can mutate and dispose its geometry without retaining invalid native iterators");
    require(renderer.info.get("memory").get("geometries").toNumber() == memoryBeforeReentry - 2,
            "Pinned nested geometry disposal decrements memory twice after its live removal callbacks");
    positionObject.erase("isInterleavedBufferAttribute");mesh->geometry->setAttribute("position",position);
    renderHostTurn(renderer, scene,camera);
    expectPixel(renderer,{0,255,0,255},"Geometry resources remain reusable after reentrant removal");
}

void attributeStorageAndReentryTests(bool validation) {
    VulkanRenderer renderer({64, 64, validation});
    renderer.srgbOutput = false;
    renderer.setClearColor(Color(0, 0, 0), 1);
    OrthographicCamera camera(-1, 1, 1, -1, .1, 20);
    camera.position.z = 5;
    Scene scene;
    auto mesh = plane(Color(1, 1, 1));
    mesh->frustumCulled = false;
    basic(mesh)->vertexColors = true;
    auto colors = std::make_shared<Float32BufferAttribute>(
        std::vector<float>{1,0,0, 1,0,0, 1,0,0, 1,0,0}, 3);
    mesh->geometry->setAttribute("color", colors);
    int uploads = 0;
    colors->onUpload([&] { ++uploads; });
    scene.add(mesh);
    renderHostTurn(renderer, scene, camera);
    expectPixel(renderer, {255,0,0,255}, "Initial attribute bytes establish the GPU format");
    const auto bufferCount = renderer.getInfo().attributeBuffers;
    const auto bytes = [](float r, float g, float b) {
        Float32Array values(std::vector<float>{r,g,b, r,g,b, r,g,b, r,g,b});
        return Uint8Array(values.buffer, double(std::size_t(values.byteOffset)),
                          double(std::size_t(values.byteLength)));
    };
    colors->array = bytes(0, 1, 0);
    renderHostTurn(renderer, scene, camera);
    expectPixel(renderer, {255,0,0,255}, "Changing array type without a version keeps cached bytes");
    require(uploads == 1, "Array replacement alone does not upload");
    colors->needsUpdate = true;
    renderHostTurn(renderer, scene, camera);
    expectPixel(renderer, {0,255,0,255}, "Same-size Uint8 storage uploads raw bytes into the retained float format");
    require(uploads == 2 && renderer.getInfo().attributeBuffers == bufferCount,
            "A same-size type change reuses the original GPU buffer");

    colors->array = bytes(0, 0, 1);
    for (int row = 0; row < 4; ++row) colors->addUpdateRange(row * 12 + 4, 8);
    colors->needsUpdate = true;
    renderHostTurn(renderer, scene, camera);
    expectPixel(renderer, {0,0,255,255}, "Partial uploads use the new source element size and old GPU format");
    require(uploads == 3 && colors->updateRanges.get("length").strictEquals(0), "Changed-type partial ranges clear before callback");

    const auto redBytes = bytes(1, 0, 0);
    colors->array = Uint16Array(redBytes.buffer, 0, 24);
    for (int row = 0; row < 4; ++row) {
        colors->addUpdateRange(row * 6 + 1, 1);
        colors->addUpdateRange(row * 6 + 5, 1);
    }
    colors->needsUpdate = true;
    renderHostTurn(renderer, scene, camera);
    expectPixel(renderer, {255,0,0,255}, "Unaligned Uint16 ranges refresh every affected cached Float32 component");
    require(uploads == 4 && colors->updateRanges.get("length").strictEquals(0), "Unaligned changed-type uploads preserve range lifecycle");
    colors->array = bytes(0, 1, 0);
    colors->addUpdateRange(0, 0);
    colors->needsUpdate = true;
    renderHostTurn(renderer, scene, camera);
    expectPixel(renderer, {0,255,0,255}, "Zero-length ranges upload the remaining changed-type source bytes");

    auto index = mesh->geometry->index;
    const auto originalIndex = std::get<Uint16Array>(index->array);
    index->array = Uint8Array(originalIndex.buffer, double(std::size_t(originalIndex.byteOffset)),
                             double(std::size_t(originalIndex.byteLength)));
    index->needsUpdate = true;
    renderHostTurn(renderer, scene, camera);
    expectPixel(renderer, {0,255,0,255}, "Index uploads also retain their original element interpretation");
    index->array = Float32Array(3);
    index->needsUpdate = true;
    renderHostTurn(renderer, scene, camera);
    expectPixel(renderer, {0,0,0,255}, "Float32 index source bytes are reinterpreted as cached Uint16 indices");
    index->array = originalIndex;
    index->needsUpdate = true;
    renderHostTurn(renderer, scene, camera);
    expectPixel(renderer, {0,255,0,255}, "Index storage recovers after several same-size type replacements");

    scene.clear();
    mesh.reset(); colors.reset(); index.reset();
    auto reentrant = plane(Color(1,1,1));
    reentrant->frustumCulled = false;
    basic(reentrant)->vertexColors = true;
    auto position = std::dynamic_pointer_cast<BufferAttribute>(reentrant->geometry->getAttribute("position"));
    auto previousColor = std::make_shared<Float32BufferAttribute>(
        std::vector<float>{1,0,0, 1,0,0, 1,0,0, 1,0,0}, 3);
    auto replacementColor = std::make_shared<Float32BufferAttribute>(
        std::vector<float>{0,1,0, 0,1,0, 0,1,0, 0,1,0}, 3);
    auto removed = std::make_shared<Float32BufferAttribute>(std::vector<float>{1}, 1);
    auto added = std::make_shared<Float32BufferAttribute>(std::vector<float>{2}, 1);
    int previousUploads = 0, replacementUploads = 0, removedUploads = 0, addedUploads = 0;
    previousColor->onUpload([&] { ++previousUploads; });
    replacementColor->onUpload([&] { ++replacementUploads; });
    removed->onUpload([&] { ++removedUploads; });
    added->onUpload([&] { ++addedUploads; });
    reentrant->geometry->setAttribute("color", previousColor).setAttribute("removed", removed);
    position->onUpload([&] {
        reentrant->geometry->setAttribute("color", replacementColor);
        reentrant->geometry->deleteAttribute("removed");
        reentrant->geometry->setAttribute("added", added);
    });
    scene.add(reentrant);
    renderHostTurn(renderer, scene, camera);
    expectPixel(renderer, {0,255,0,255}, "Earlier upload callbacks replace later attributes before they are read");
    require(previousUploads == 0 && replacementUploads == 1,
            "Geometry updates upload the current later attribute, not a prefetched pointer");
    require(removedUploads == 0 && addedUploads == 0,
            "Deleted enumeration keys skip and newly added keys wait for the next frame");
    renderHostTurn(renderer, scene, camera);
    require(addedUploads == 1, "The next geometry update enumerates newly added attributes");
    position->onUpload([&] { reentrant->geometry->setAttribute("color", nullptr); });
    position->needsUpdate = true;
    bool nullRejected = false;
    try { renderHostTurn(renderer, scene, camera); }
    catch (const JSTypeError&) { nullRejected = true; }
    require(nullRejected, "A callback replacing a later attribute with null preserves upstream TypeError");
    reentrant->geometry->setAttribute("color", replacementColor);
    renderHostTurn(renderer, scene, camera);
    expectPixel(renderer, {0,255,0,255}, "The renderer recovers after an attribute enumeration callback exception");
    require(renderer.getValidationMessages().empty(), "Attribute type changes and upload reentry preserve Vulkan state");
}

void geometryObjectProtocolTests(bool validation) {
    VulkanRenderer renderer({64,64,validation});
    renderer.srgbOutput = false;
    OrthographicCamera camera(-1,1,1,-1,.1,20); camera.position.z = 5;
    Scene scene;
    auto geometry = std::make_shared<BufferGeometry>();
    auto position = std::make_shared<Float32BufferAttribute>(
        std::vector<float>{-.6f,-.6f,0,.6f,-.6f,0,0,.6f,0},3);
    auto unused = std::make_shared<Float32BufferAttribute>(std::vector<float>{1,2,3},1);
    geometry->setAttribute("position",position).setAttribute("unused",unused);
    auto material = std::make_shared<MeshBasicMaterial>(Color(1,0,0));
    auto first = std::make_shared<Mesh>(geometry,material), second = std::make_shared<Mesh>(geometry,material);
    first->frustumCulled = second->frustumCulled = false;
    scene.add(first).add(second);
    auto reads = JSValue::object(); reads.set("count",0);
    JSPropertyDescriptor watched; watched.configurable = true;
    watched.getter = JSValue::function(+[](const JSValue&,const auto&,const auto& captures)->JSValue {
        auto state = captures[0]; state.set("count",state.get("count").toNumber()+1); return false;
    },{reads});
    nativeAttribute(unused).defineProperty("isInterleavedBufferAttribute",watched);
    renderHostTurn(renderer,scene,camera);
    require(reads.get("count").strictEquals(1),
            "Shared geometry raw update reads unused upload fields once without a decode getter pass");
    require(renderer.getInfo().calls == 2,"Two objects share the module's once-per-frame geometry upload");
    auto render = renderer.info.get("render"); render.set("frame",std::numeric_limits<double>::quiet_NaN());
    reads.set("count",0); renderHostTurn(renderer,scene,camera);
    require(reads.get("count").strictEquals(2) && std::isnan(renderer.getInfo().frame),
            "NaN frame never equals the cached frame and updates each shared object");
    render.set("frame","0"); reads.set("count",0); renderHostTurn(renderer,scene,camera);
    require(reads.get("count").strictEquals(1) && renderer.getInfo().frame == 1,
            "String frame is cached by strict identity before the renderer numerically increments it");
    scene.remove(second); material->wireframe = true;
    auto index = std::make_shared<Uint16BufferAttribute>(std::vector<std::uint16_t>{0,1,2},1);
    geometry->setIndex(index); index->version = "2";
    const auto hasLine = [&] {
        const auto pixels = renderer.readPixels();
        for (std::size_t i=0;i<pixels.size();i+=4) if (pixels[i]>128) return true;
        return false;
    };
    renderHostTurn(renderer,scene,camera); require(hasLine(),"Managed wireframe indices reach actual Vulkan draws");
    std::get<Uint16Array>(index->array).assign(3,0); index->version = "10";
    renderHostTurn(renderer,scene,camera);
    require(hasLine(),"Wireframe version strings compare lexically and retain the existing typed indices");
    index->version = 3; renderHostTurn(renderer,scene,camera);
    require(!hasLine(),"A numeric greater wireframe version replaces the managed index and Vulkan bytes");
    require(renderer.getInfo().lines == 3,"Generated wireframes use indexed line statistics");
    require(renderer.getValidationMessages().empty(),"Geometry protocol integration has no Vulkan validation errors");

    auto instance = std::make_shared<InstancedMesh>(geometry,material,1);
    instance->frustumCulled = false; instance->setMatrixAt(0,Matrix4{});
    Scene nativeScene; nativeScene.add(instance);
    auto local = std::make_unique<VulkanRenderer>(VulkanRendererParameters{64,64,validation});
    renderHostTurn(*local,nativeScene,camera);
    auto wrapper = nativeObject(instance), listener = wrapper.get("_listeners").get("dispose").get("0");
    require(listener.isCallable(),"Instance disposal retains the module's managed listener");
    local.reset();
    auto event = JSValue::object(); event.set("target",wrapper);
    listener.call(JSValue::object(),{event});
    instance->dispose();
    require(true,"Escaped disposal closures and native forwarders are inert after renderer device retirement");
}

} // namespace

int main(int argc, char** argv) {
    try {
        const bool validation = argc > 1 && std::string(argv[1]) == "--validation";
        unsupportedMeshTests(validation);
        unsupportedPrimitiveTests(validation);
        instancingTests(validation);
        textureTests(validation);
        mipmapTests(validation);
        renderOrderTests(validation);
        attributeRangeSortTests(validation);
        attributeNumberTests(validation);
        rendererInfoTests(validation);
        attributeCacheWeakLifetimeTests(validation);
        attributeManagedUploadTests(validation);
        attributeStorageAndReentryTests(validation);
        geometryObjectProtocolTests(validation);
        VulkanRenderer renderer({64, 64, validation});
        OrthographicCamera camera(-1, 1, 1, -1, 0.1, 20);
        camera.position.set(0, 0, 5);
        Scene scene;
        {
            auto halfGeometry = std::make_shared<PlaneGeometry>(1.6, 1.6);
            const auto source = halfGeometry->getAttribute("position");
            auto halfPositions = std::make_shared<Float16BufferAttribute>(12.0, 3);
            auto halfColors = std::make_shared<Float16BufferAttribute>(12.0, 3);
            for (int i = 0; i < 4; ++i) {
                halfPositions->setXYZ(i, source->getX(i), source->getY(i), source->getZ(i));
                halfColors->setXYZ(i, 0.25, 0.5, 1);
            }
            halfGeometry->setAttribute("position", halfPositions);
            halfGeometry->setAttribute("color", halfColors);
            auto halfMaterial = std::make_shared<MeshBasicMaterial>(Color(1, 1, 1));
            halfMaterial->vertexColors = true;
            scene.add(std::make_shared<Mesh>(halfGeometry, halfMaterial));
            int uploads = 0;
            halfColors->onUpload([&] { ++uploads; });
            renderHostTurn(renderer, scene, camera);
            expectPixel(renderer, {137, 188, 255, 255}, "Half-float positions and colors decode through Vulkan vertex fetch");
            halfColors->normalized = true;
            renderHostTurn(renderer, scene, camera);
            expectPixel(renderer, {137, 188, 255, 255}, "Half-float GPU vertex fetch ignores the normalized flag");
            auto& raw = std::get<Uint16Array>(halfColors->array);
            for (std::size_t i = 0; i < 4; ++i) {
                raw[i * 3] = toHalfFloat(1); raw[i * 3 + 1] = 0; raw[i * 3 + 2] = 0;
            }
            renderHostTurn(renderer, scene, camera);
            expectPixel(renderer, {137, 188, 255, 255}, "Half-float raw array edits remain pending until needsUpdate");
            halfColors->addUpdateRange(0, 12);
            halfColors->needsUpdate = true;
            renderHostTurn(renderer, scene, camera);
            expectPixel(renderer, {255, 0, 0, 255}, "Half-float updateRanges upload decoded vertex values");
            require(uploads == 2 && halfColors->updateRanges.get("length").strictEquals(0), "Half-float uploads preserve callback and range lifecycle");
            scene.clear();
        }
        renderHostTurn(renderer, scene, camera);
        require(renderer.getInfo().attributeBuffers == 2, "Pinned binding state retains half-float vertex resources after geometry owners expire");
        renderer.setClearColor(Color(0.5, 0.25, 0), 0.75);
        renderer.clear();
        expectPixel(renderer, {188, 137, 0, 191}, "Linear clear color is encoded as sRGB");
        renderer.setClearColor(Color(0, 0, 0));
        auto front = plane(Color(1, 0, 0));
        scene.add(front);
        renderHostTurn(renderer, scene, camera);
        expectPixel(renderer, {255, 0, 0, 255}, "Front-facing indexed plane reaches the GPU");
        expectPixel(renderer, {0, 0, 0, 255}, "Pixels outside geometry retain clear color", 0, 0);
        require(renderer.getInfo().calls == 1 && renderer.getInfo().triangles == 2,
                "Draw statistics match indexed plane");
        basic(front)->side = BackSide;
        renderHostTurn(renderer, scene, camera);
        expectPixel(renderer, {0, 0, 0, 255}, "BackSide culls a front-facing plane");
        basic(front)->side = DoubleSide;
        renderHostTurn(renderer, scene, camera);
        expectPixel(renderer, {255, 0, 0, 255}, "DoubleSide renders both orientations");
        basic(front)->side = FrontSide;
        front->scale.set(-1, 1, 1);
        renderHostTurn(renderer, scene, camera);
        expectPixel(renderer, {255, 0, 0, 255},
                    "Mirrored world transforms preserve three.js front-side semantics");
        front->scale.set(1, 1, 1);
        front->position.set(0, 0, 1);
        auto rear = plane(Color(0, 0, 1));
        scene.add(rear);
        renderHostTurn(renderer, scene, camera);
        expectPixel(renderer, {255, 0, 0, 255},
                    "Depth attachment prevents a later farther draw from replacing the front mesh");
        basic(rear)->depthTest = false;
        renderHostTurn(renderer, scene, camera);
        expectPixel(renderer, {0, 0, 255, 255}, "depthTest false permits the farther draw");
        basic(rear)->depthTest = true;
        basic(front)->depthWrite = false;
        renderHostTurn(renderer, scene, camera);
        expectPixel(renderer, {0, 0, 255, 255},
                    "depthWrite false leaves depth available for subsequent geometry");
        basic(front)->depthWrite = true;
        basic(front)->transparent = true;
        basic(front)->opacity = 0.5;
        renderHostTurn(renderer, scene, camera);
        expectPixel(renderer, {128, 0, 127, 255},
                    "Transparent normal blending follows opaque draws");
        basic(front)->alphaTest = 0.6;
        renderHostTurn(renderer, scene, camera);
        expectPixel(renderer, {0, 0, 255, 255}, "alphaTest discards fragments before blending");
        basic(front)->alphaTest = 0;
        basic(front)->transparent = false;
        basic(front)->opacity = 1;
        basic(front)->colorWrite = false;
        renderHostTurn(renderer, scene, camera);
        expectPixel(renderer, {0, 0, 0, 255},
                    "colorWrite false still writes depth and occludes farther color");
        basic(front)->colorWrite = true;
        scene.remove(rear);
        basic(front)->color.set(0.5, 0.5, 0.5);
        renderHostTurn(renderer, scene, camera);
        expectPixel(renderer, {188, 188, 188, 255},
                    "MeshBasicMaterial linear color is encoded as sRGB");
        renderer.srgbOutput = false;
        renderHostTurn(renderer, scene, camera);
        expectPixel(renderer, {128, 128, 128, 255}, "Linear output bypasses the sRGB transfer");
        renderer.srgbOutput = true;
        basic(front)->color.set(1, 1, 1);
        basic(front)->vertexColors = true;
        front->geometry->setAttribute(
            "color",
            std::make_shared<Uint8BufferAttribute>(
                std::vector<std::uint8_t>{0, 255, 0, 0, 255, 0, 0, 255, 0, 0, 255, 0}, 3, true));
        renderHostTurn(renderer, scene, camera);
        expectPixel(renderer, {0, 255, 0, 255},
                    "Normalized integer vertex colors are consumed by the graphics pipeline");
        basic(front)->vertexColors = false;
        basic(front)->color.set(1, 0, 0);
        front->geometry->setDrawRange(0, 0);
        renderHostTurn(renderer, scene, camera);
        expectPixel(renderer, {0, 0, 0, 255}, "An empty drawRange emits no fragments");
        require(renderer.getInfo().calls == 1 && renderer.getInfo().triangles == 0,
                "A zero-count drawRange still submits a draw with no triangles");
        front->geometry->setDrawRange(0, 6);
        front->layers.set(1);
        renderHostTurn(renderer, scene, camera);
        expectPixel(renderer, {0, 0, 0, 255}, "Camera layers filter mesh draws");
        front->layers.set(0);
        auto parent = std::make_shared<Group>();
        scene.remove(front);
        parent->add(front);
        scene.add(parent);
        parent->visible = false;
        renderHostTurn(renderer, scene, camera);
        expectPixel(renderer, {0, 0, 0, 255}, "An invisible parent excludes its subtree");
        parent->visible = true;
        parent->layers.set(1);
        renderHostTurn(renderer, scene, camera);
        expectPixel(renderer, {255, 0, 0, 255},
                    "A parent's unmatched layer still allows matching descendants");
        front->position.set(20, 0, 0);
        renderHostTurn(renderer, scene, camera);
        require(renderer.getInfo().calls == 0, "Frustum culling removes fully out-of-view meshes");
        front->frustumCulled = false;
        renderHostTurn(renderer, scene, camera);
        require(renderer.getInfo().calls == 1, "frustumCulled false preserves the draw submission");
        scene.clear();
        {
            auto projective = plane(Color(1, 0, 0));
            projective->matrixAutoUpdate = false;
            projective->matrix.makeTranslation(2.5, 0, 0);
            projective->matrix.elements()[15] = 2;
            scene.add(projective);
            renderHostTurn(renderer, scene, camera);
            require(renderer.getInfo().calls == 1,
                    "Projective world matrices transform the culling sphere with homogeneous division");
            expectPixel(renderer, {255, 0, 0, 255},
                        "Projective geometry remains visible at the right viewport edge", 61, 32);
            const auto culledPixels = renderer.readPixels();
            projective->frustumCulled = false;
            renderHostTurn(renderer, scene, camera);
            expectPixel(renderer, {255, 0, 0, 255},
                        "The projective geometry also renders with culling disabled", 61, 32);
            require(renderer.readPixels() == culledPixels,
                    "Culling preserves the complete projective geometry output");
            scene.clear();
        }
        auto fogged = plane(Color(1, 0, 0));
        scene.add(fogged);
        scene.fog = std::make_shared<Fog>(Color(0, 0, 1), 4, 6);
        renderHostTurn(renderer, scene, camera);
        expectPixel(renderer, {128, 0, 128, 255},
                    "Linear fog interpolates after output color conversion");
        scene.fog = std::make_shared<FogExp2>(Color(0, 0, 1), 0.2);
        renderHostTurn(renderer, scene, camera);
        expectPixel(renderer, {94, 0, 161, 255},
                    "Exponential squared fog uses interpolated view-space depth");
        auto fogPositionView = std::get<TypedArray<float>>(fogged->geometry->getAttribute("position")->getArray().storage());
        for (std::size_t i = 2; i < fogPositionView.size(); i += 3)
            fogPositionView[i] = 2;
        renderHostTurn(renderer, scene, camera);
        expectPixel(renderer, {94, 0, 161, 255},
                    "TypedArray alias writes without needsUpdate preserve cached positions used for fog");
        for (std::size_t i = 2; i < fogPositionView.size(); i += 3)
            fogPositionView[i] = 0;
        basic(fogged)->fog = false;
        renderHostTurn(renderer, scene, camera);
        expectPixel(renderer, {255, 0, 0, 255}, "material.fog false disables scene fog");
        scene.fog.reset();
        basic(fogged)->blending = NoBlending;
        basic(fogged)->opacity = 0.5;
        renderHostTurn(renderer, scene, camera);
        expectPixel(renderer, {255, 0, 0, 128},
                    "NoBlending preserves opacity without the OPAQUE define");
        basic(fogged)->blending = NormalBlending;
        basic(fogged)->transparent = true;
        basic(fogged)->side = DoubleSide;
        const auto doubleSideVersion = basic(fogged)->version;
        renderHostTurn(renderer, scene, camera);
        require(renderer.getInfo().calls == 2,
                "Transparent DoubleSide defaults to back and front draws");
        require(basic(fogged)->version == doubleSideVersion + 2,
                "Transparent DoubleSide invalidates material state for both side passes");
        basic(fogged)->forceSinglePass = true;
        renderHostTurn(renderer, scene, camera);
        require(renderer.getInfo().calls == 1,
                "forceSinglePass disables the transparent DoubleSide second draw");
        require(basic(fogged)->version == doubleSideVersion + 2,
                "forceSinglePass does not invalidate material state for side passes");
        basic(fogged)->transparent = false;
        basic(fogged)->opacity = 1;
        basic(fogged)->wireframe = true;
        renderHostTurn(renderer, scene, camera);
        require(renderer.getInfo().calls == 1 && renderer.getInfo().lines == 6,
                "Wireframe expands indexed triangle edges into GPU lines");
        scene.clear();
        auto nearTransparent = plane(Color(1, 0, 0)), farTransparent = plane(Color(0, 0, 1));
        nearTransparent->position.set(0, 0, 1);
        for (const auto& mesh : {nearTransparent, farTransparent}) {
            basic(mesh)->transparent = true;
            basic(mesh)->opacity = 0.5;
            basic(mesh)->depthWrite = false;
            scene.add(mesh);
        }
        renderHostTurn(renderer, scene, camera);
        expectPixel(renderer, {128, 0, 64, 255},
                    "Transparent objects sort far to near independently of creation order");
        nearTransparent->renderOrder = -1;
        renderHostTurn(renderer, scene, camera);
        expectPixel(renderer, {64, 0, 128, 255},
                    "Explicit renderOrder precedes transparent depth sorting");
        scene.clear();
        {
            PerspectiveCamera sortingCamera(60, 1, 0.1, 20);
            sortingCamera.position.z = 5;
            auto clipNear = plane(Color(1, 0, 0)), clipFar = plane(Color(0, 0, 1));
            clipNear->position.z = 1;
            clipFar->matrixAutoUpdate = false;
            std::vector<std::string> order;
            clipNear->onBeforeRender = [&](auto&, auto&, auto&, auto*, auto*, auto*) {
                order.push_back("red");
            };
            clipFar->onBeforeRender = [&](auto&, auto&, auto&, auto*, auto*, auto*) {
                order.push_back("blue");
            };
            for (const auto& mesh : {clipNear, clipFar}) {
                basic(mesh)->transparent = true;
                basic(mesh)->opacity = 0.5;
                basic(mesh)->depthWrite = false;
                scene.add(mesh);
            }
            for (const bool reversed : {false, true}) {
                sortingCamera.reversedDepth = reversed;
                sortingCamera.updateProjectionMatrix();
                clipFar->matrix.elements()[15] = reversed ? 4 : 0.25;
                order.clear();
                renderHostTurn(renderer, scene, sortingCamera);
                require(order == std::vector<std::string>{"red", "blue"},
                        "Transparent sorting uses homogeneous clip z, including reversed depth");
                expectPixel(renderer, {64, 0, 128, 255},
                            "Clip-z transparent ordering determines the overlapping blended pixel");
            }
            auto sharedMaterial = std::make_shared<MeshBasicMaterial>(Color(1, 1, 1));
            sharedMaterial->vertexColors = true;
            sharedMaterial->depthTest = false;
            for (const auto& mesh : {clipNear, clipFar}) {
                mesh->material = sharedMaterial;
                const bool red = mesh == clipNear;
                mesh->geometry->setAttribute("color", std::make_shared<Float32BufferAttribute>(
                    std::vector<float>{red ? 1.f : 0.f, 0, red ? 0.f : 1.f,
                                       red ? 1.f : 0.f, 0, red ? 0.f : 1.f,
                                       red ? 1.f : 0.f, 0, red ? 0.f : 1.f,
                                       red ? 1.f : 0.f, 0, red ? 0.f : 1.f}, 3));
            }
            for (const bool reversed : {false, true}) {
                sortingCamera.reversedDepth = reversed;
                sortingCamera.updateProjectionMatrix();
                clipFar->matrix.elements()[15] = reversed ? 4 : 0.25;
                order.clear();
                renderHostTurn(renderer, scene, sortingCamera);
                require(order == std::vector<std::string>{"blue", "red"},
                        "Opaque objects sharing a material sort by ascending clip z");
                expectPixel(renderer, {255, 0, 0, 255},
                            "Opaque clip-z ordering determines the final pixel without depth testing");
            }
            scene.clear();
        }
        {
            auto emptyGeometry = std::make_shared<BufferGeometry>();
            auto emptyPosition = std::make_shared<Float32BufferAttribute>(std::vector<float>{}, 3);
            auto emptyUnused = std::make_shared<Float32BufferAttribute>(std::vector<float>{}, 1);
            emptyGeometry->setAttribute("position", emptyPosition);
            emptyGeometry->setAttribute("unused", emptyUnused);
            emptyGeometry->morphAttributes["position"] = {
                std::make_shared<Float32BufferAttribute>(std::vector<float>{}, 3)};
            auto emptyMaterial = std::make_shared<MeshBasicMaterial>(Color(1, 0, 0));
            auto emptyMesh = std::make_shared<Mesh>(emptyGeometry, emptyMaterial);
            emptyMesh->morphTargetInfluences->at(0) = 1;
            std::vector<std::string> order;
            std::uint64_t expectedCalls = 1;
            emptyPosition->onUpload([&] { order.push_back("position-upload"); });
            emptyUnused->onUpload([&] { order.push_back("unused-upload"); });
            emptyMesh->onBeforeRender = [&](auto&, auto&, auto&, auto*, auto*, auto*) {
                order.push_back("object-before");
            };
            emptyMaterial->onBeforeRender = [&](auto&, auto&, auto&, auto*, auto* object, auto*) {
                require(std::abs(object->modelViewMatrix.elements()[14] + 5) < 1e-10,
                        "An empty geometry's material hook observes the updated model-view matrix");
                order.push_back("material-before");
            };
            emptyMesh->onAfterRender = [&](auto& activeRenderer, auto&, auto&, auto*, auto*, auto*) {
                require(activeRenderer.getInfo().calls == expectedCalls,
                        "An empty geometry's after hook observes the submitted draw count");
                order.push_back("object-after");
            };
            scene.add(emptyMesh);
            renderHostTurn(renderer, scene, camera);
            require(order == std::vector<std::string>{"position-upload", "unused-upload",
                                                      "object-before", "material-before", "object-after"},
                    "Zero position count uploads ordinary attributes before object and material hooks");
            require(renderer.getInfo().calls == 1 && renderer.getInfo().triangles == 0,
                    "A zero-position non-indexed mesh submits a zero-count draw");
            expectPixel(renderer, {0, 0, 0, 255}, "A zero-position draw produces no fragments");
            auto emptyIndex = std::make_shared<Uint16BufferAttribute>(std::vector<std::uint16_t>{}, 1);
            emptyIndex->onUpload([&] { order.push_back("index-upload"); });
            emptyGeometry->setIndex(emptyIndex);
            order.clear();
            renderHostTurn(renderer, scene, camera);
            require(order == std::vector<std::string>{"object-before", "material-before",
                                                      "index-upload", "object-after"},
                    "A zero-count indexed draw uploads its index after the material hook");
            require(renderer.getInfo().calls == 1 && renderer.getInfo().triangles == 0,
                    "An empty index still submits a zero-count indexed draw");
            emptyPosition->needsUpdate = true;
            emptyIndex->needsUpdate = true;
            emptyGeometry->setDrawRange(1, 0);
            expectedCalls = 0;
            order.clear();
            renderHostTurn(renderer, scene, camera);
            require(order == std::vector<std::string>{"position-upload", "object-before",
                                                      "material-before", "object-after"},
                    "A negative clipped draw range preserves ordinary upload and hooks but skips index upload");
            require(renderer.getInfo().calls == 0,
                    "A draw range starting beyond an empty geometry skips draw submission");
            emptyGeometry->setDrawRange(0, 0);
            expectedCalls = 1;
            order.clear();
            renderHostTurn(renderer, scene, camera);
            require(order == std::vector<std::string>{"object-before", "material-before",
                                                      "index-upload", "object-after"},
                    "The pending index version uploads when a later zero-count range permits binding");
            emptyMaterial->wireframe = true;
            scene.fog = std::make_shared<Fog>(Color(0, 0, 1), 4, 6);
            order.clear();
            renderHostTurn(renderer, scene, camera);
            require(order == std::vector<std::string>{"object-before", "material-before", "object-after"},
                    "Empty wireframe rendering preserves hooks without reuploading unchanged attributes");
            require(renderer.getInfo().calls == 1 && renderer.getInfo().lines == 0,
                    "An empty wireframe submits a zero-count indexed line draw");
            expectPixel(renderer, {0, 0, 0, 255},
                        "Empty fogged morph and wireframe buffers produce no fragments");
            scene.fog.reset();
            scene.clear();
        }
        {
            auto geometry = std::make_shared<BufferGeometry>();
            auto positions = std::make_shared<Float32BufferAttribute>(
                std::vector<float>{-.8f, -.8f, 0, .8f, -.8f, 0, 0, .8f, 0, 7}, 3);
            geometry->setAttribute("position", positions);
            auto mesh = std::make_shared<Mesh>(geometry,
                std::make_shared<MeshBasicMaterial>(Color(1, 0, 0)));
            mesh->frustumCulled = false;
            geometry->boundingSphere = Sphere(Vector3(), 2);
            scene.add(mesh);
            renderHostTurn(renderer, scene, camera);
            expectPixel(renderer, {255, 0, 0, 255},
                        "A fractional position count does not fetch its incomplete trailing row");
            require(std::abs(renderer.getInfo().triangles - (10.0 / 3) / 3) < 1e-12,
                    "Triangle statistics retain the logical fractional draw count");
            geometry->setDrawRange(0, 3);
            renderHostTurn(renderer, scene, camera);
            expectPixel(renderer, {255, 0, 0, 255},
                        "A drawRange excludes the incomplete position row before CPU validation");
            require(renderer.getInfo().triangles == 1, "Draw ranges clip logical triangle statistics");
            positions->count = 100;
            renderHostTurn(renderer, scene, camera);
            expectPixel(renderer, {255, 0, 0, 255},
                        "An enlarged position.count permits an in-bounds drawRange");
            require(renderer.getInfo().triangles == 1,
                    "Actual draw-range storage determines safety rather than position.count metadata");
            positions->count = 2.5;
            renderHostTurn(renderer, scene, camera);
            expectPixel(renderer, {0, 0, 0, 255},
                        "Vulkan receives the integer conversion of a fractional vertex count");
            require(std::abs(renderer.getInfo().triangles - 2.5 / 3) < 1e-12,
                    "Statistics keep the fractional count before Vulkan integer conversion");
            positions->count = 10.0 / 3;
            auto index = std::make_shared<Uint16BufferAttribute>(
                std::vector<std::uint16_t>{0, 1, 2}, 1);
            index->count = 2.5;
            geometry->setIndex(index);
            renderHostTurn(renderer, scene, camera);
            require(std::abs(renderer.getInfo().triangles - 2.5 / 3) < 1e-12,
                    "Indexed statistics also retain the logical fractional count");
            scene.clear();
        }
        {
            const auto nan = std::numeric_limits<float>::quiet_NaN();
            auto geometry = std::make_shared<BufferGeometry>();
            geometry->setAttribute("position", std::make_shared<Float32BufferAttribute>(
                std::vector<float>{nan, nan, nan, -.8f, -.8f, 0, .8f, -.8f, 0,
                                   0, .8f, 0, nan, nan, nan}, 3));
            geometry->morphTargetsRelative = true;
            geometry->morphAttributes["position"] = {std::make_shared<Float32BufferAttribute>(
                std::vector<float>{nan, nan, nan, 0, 0, 0, 0, 0, 0,
                                   0, 0, 0, nan, nan, nan}, 3)};
            // Four color rows suffice: only vertices 1, 2 and 3 are fetched.
            geometry->setAttribute("color", std::make_shared<Float32BufferAttribute>(
                std::vector<float>{nan, nan, nan, 1, 0, 0, 1, 0, 0, 1, 0, 0}, 3));
            geometry->setDrawRange(1, 3);
            geometry->boundingSphere = Sphere(Vector3(), 2);
            auto material = std::make_shared<MeshBasicMaterial>(Color(1, 1, 1));
            material->vertexColors = true;
            auto mesh = std::make_shared<Mesh>(geometry, material);
            mesh->frustumCulled = false;
            mesh->morphTargetInfluences->at(0) = 1;
            scene.add(mesh);
            scene.fog = std::make_shared<Fog>(Color(0, 0, 1), 4, 6);
            renderHostTurn(renderer, scene, camera);
            expectPixel(renderer, {128, 0, 128, 255},
                        "Fog and morph calculations ignore non-finite vertices outside the draw range");
            auto index = std::make_shared<Uint16BufferAttribute>(
                std::vector<std::uint16_t>{999, 1, 2, 3, 999}, 1);
            geometry->setIndex(index);
            renderHostTurn(renderer, scene, camera);
            expectPixel(renderer, {128, 0, 128, 255},
                        "Indexed fog and morph draws inspect only the referenced vertices");
            index->setX(2, 999);
            index->needsUpdate = true;
            renderHostTurn(renderer, scene, camera);
            require(pixel(renderer,32,32)==std::array<int,4>{0,0,0,255} && renderer.getInfo().calls==1,
                    "An out-of-bounds GPU vertex fetch rejects the draw while preserving info");
            index->setX(2, 2);
            index->needsUpdate = true;
            renderHostTurn(renderer, scene, camera);
            expectPixel(renderer, {128, 0, 128, 255},
                        "Corrected geometry renders after an out-of-bounds fetch rejection");
            scene.fog.reset();
            scene.clear();
        }
        {
            auto geometry = std::make_shared<BufferGeometry>();
            geometry->setAttribute("position", std::make_shared<Float32BufferAttribute>(
                std::vector<float>{-.8f, 0, 0, .8f, 0, 0, 0, .8f, 0}, 3));
            auto index = std::make_shared<Uint32BufferAttribute>(
                std::vector<std::uint32_t>{0, 1}, 1);
            geometry->setIndex(index);
            auto material = std::make_shared<MeshBasicMaterial>(Color(1, 0, 0));
            material->wireframe = true;
            auto mesh = std::make_shared<Mesh>(geometry, material);
            mesh->frustumCulled = false;
            scene.add(mesh);
            const auto hasRedLine = [&] {
                return pixel(renderer, 32, 31)[0] == 255 || pixel(renderer, 32, 32)[0] == 255;
            };
            renderHostTurn(renderer, scene, camera);
            require(hasRedLine(), "An incomplete indexed triangle still produces its valid wireframe edge");
            require(renderer.getInfo().lines == 3,
                    "Wireframe statistics use all six generated indices including undefined-to-zero entries");
            index->count = 0;
            renderHostTurn(renderer, scene, camera);
            require(hasRedLine() && renderer.getInfo().lines == 3,
                    "Wireframe generation and clipping use raw index array length instead of index.count");
            geometry->setDrawRange(1, 1);
            renderHostTurn(renderer, scene, camera);
            require(hasRedLine() && renderer.getInfo().lines == 1,
                    "Wireframe draw ranges are doubled before generated-index clipping");
            geometry->setDrawRange(3, 0);
            renderHostTurn(renderer, scene, camera);
            require(renderer.getInfo().calls == 1 && renderer.getInfo().lines == 0,
                    "A zero draw at the generated wireframe endpoint still submits");
            geometry->setDrawRange(0, 3);
            index->set(std::vector<std::uint32_t>{65536, 65537});
            index->needsUpdate = true;
            renderHostTurn(renderer, scene, camera);
            require(hasRedLine() && renderer.getInfo().lines == 3,
                    "Small wireframe geometries truncate source indices to the upstream Uint16 format");
            scene.clear();
        }
        {
            auto geometry = std::make_shared<BufferGeometry>();
            auto positions = std::make_shared<Float32BufferAttribute>(65535.0 * 3, 3);
            positions->setXYZ(0, -.8, 0, 0);
            positions->setXYZ(1, .8, 0, 0);
            geometry->setAttribute("position", positions);
            geometry->setIndex(std::make_shared<Uint32BufferAttribute>(
                std::vector<std::uint32_t>{65536, 65537}, 1));
            geometry->boundingSphere = Sphere(Vector3(), 2);
            auto material = std::make_shared<MeshBasicMaterial>(Color(1, 0, 0));
            material->wireframe = true;
            auto mesh = std::make_shared<Mesh>(geometry, material);
            mesh->frustumCulled = false;
            scene.add(mesh);
            renderHostTurn(renderer, scene, camera);
            require(pixel(renderer,32,32)==std::array<int,4>{0,0,0,255} && renderer.getInfo().calls==1,
                    "The 65535-position threshold preserves Uint32 wireframe indices instead of truncating them");
            positions->count = 65534;
            geometry->dispose();
            renderHostTurn(renderer, scene, camera);
            require(renderer.getInfo().lines == 3 &&
                        (pixel(renderer, 32, 31)[0] == 255 || pixel(renderer, 32, 32)[0] == 255),
                    "Below the threshold, regenerated wireframe indices use Uint16 conversion");
            scene.clear();
        }
        {
            auto geometry = std::make_shared<BufferGeometry>();
            auto positions = std::make_shared<Float32BufferAttribute>(
                std::vector<float>{-.8f, 0, 0, 9, .8f, 0, 0, 9, 0, .8f, 0, 9, 7}, 4);
            geometry->setAttribute("position", positions);
            geometry->boundingSphere = Sphere(Vector3(), 2);
            auto material = std::make_shared<MeshBasicMaterial>(Color(1, 0, 0));
            material->wireframe = true;
            auto mesh = std::make_shared<Mesh>(geometry, material);
            mesh->frustumCulled = false;
            scene.add(mesh);
            renderHostTurn(renderer, scene, camera);
            require(pixel(renderer,32,32)==std::array<int,4>{0,0,0,255} && renderer.getInfo().calls==1,
                    "Non-indexed wireframe generation follows array.length / 3 even with itemSize four");
            geometry->setDrawRange(0, 3);
            renderHostTurn(renderer, scene, camera);
            require(renderer.getInfo().lines == 3,
                    "Clipping wireframe indices excludes generated vertices outside uploaded storage");
            require(pixel(renderer, 32, 31)[0] == 255 || pixel(renderer, 32, 32)[0] == 255,
                    "The safe wireframe range renders after rejecting a generated out-of-bounds edge");
            scene.clear();
        }
        auto morphGeometry = std::make_shared<PlaneGeometry>(0.4, 0.4);
        morphGeometry->morphTargetsRelative = true;
        morphGeometry->morphAttributes["position"] = {std::make_shared<Float32BufferAttribute>(
            std::vector<float>{0.8f, 0, 0, 0.8f, 0, 0, 0.8f, 0, 0, 0.8f, 0, 0}, 3)};
        auto morphed = std::make_shared<Mesh>(morphGeometry,
                                              std::make_shared<MeshBasicMaterial>(Color(1, 0, 0)));
        morphed->morphTargetInfluences->at(0) = 1;
        scene.add(morphed);
        renderHostTurn(renderer, scene, camera);
        expectPixel(renderer, {0, 0, 0, 255},
                    "Morph target positions move fragments away from the undeformed position");
        expectPixel(renderer, {255, 0, 0, 255},
                    "Morph target influences update the GPU vertex positions", 57, 32);
        require(morphGeometry->boundingSphere.has_value(),
                "Renderer computes and retains geometry boundingSphere like three.js");
        auto influences = nativeObject(morphed).get("morphTargetInfluences");
        influences.erase("0");
        bool missingRejected = false;
        try { renderHostTurn(renderer, scene, camera); }
        catch (const std::invalid_argument& error) {
            missingRejected = std::string(error.what()) == "Non-finite geometry position";
        }
        require(missingRejected,"Morph influence holes become NaN instead of silently rendering undeformed positions");
        influences.set("length", 0); missingRejected = false;
        try { renderHostTurn(renderer, scene, camera); }
        catch (const std::invalid_argument& error) {
            missingRejected = std::string(error.what()).find("retained WebGL uniform state") != std::string::npos;
        }
        require(missingRejected,"Short GPU morph arrays explicitly report unsupported retained uniform state");
        morphGeometry->setDrawRange(0,0);
        int emptyHooks=0,emptyUploads=0;
        morphed->onBeforeRender=[&](auto&,auto&,auto&,auto*,auto*,auto*){++emptyHooks;};
        morphed->onAfterRender=[&](auto&,auto&,auto&,auto*,auto*,auto*){++emptyHooks;};
        std::dynamic_pointer_cast<BufferAttribute>(morphGeometry->getAttribute("position"))->onUpload([&]{++emptyUploads;});
        morphGeometry->getAttribute("position")->setNeedsUpdate(true);
        renderHostTurn(renderer, scene,camera);
        require(renderer.getInfo().calls==1 && renderer.getInfo().triangles==0 && emptyHooks==2 && emptyUploads==1,
                "Zero-count draws retain upload and hooks without reading absent morph weights");
        expectPixel(renderer,{0,0,0,255},"Zero-count draws with short morph influences produce no fragments");
        morphed->onBeforeRender=nullptr;morphed->onAfterRender=nullptr;
        std::dynamic_pointer_cast<BufferAttribute>(morphGeometry->getAttribute("position"))->onUpload([]{});
        morphGeometry->setDrawRange(0,INFINITY);
        influences.set("0", 1);renderHostTurn(renderer, scene, camera);
        expectPixel(renderer,{255,0,0,255},"Valid morph influences render after missing-influence rejection",57,32);
        morphGeometry->boundingSphere = Sphere(Vector3(30, 0, 0), 0.1);
        renderHostTurn(renderer, scene, camera);
        require(renderer.getInfo().calls == 0,
                "An explicit boundingSphere controls frustum culling");
        scene.clear();
        auto hooked = plane(Color(1, 0, 0));
        scene.add(hooked);
        std::vector<std::string> callbackOrder;
        scene.onBeforeRender = [&](auto&, auto&, auto&, auto*, auto*, auto*) {
            callbackOrder.push_back("scene-before");
        };
        hooked->onBeforeRender = [&](auto&, auto&, auto&, auto* geometry, auto* material,
                                     auto* group) {
            require(geometry == hooked->geometry.get() && material == hooked->material.get() &&
                        group == nullptr,
                    "Object hook receives render item geometry/material/group");
            callbackOrder.push_back("object-before");
        };
        basic(hooked)->onBeforeRender = [&](auto&, auto&, auto&, auto*, auto* object, auto*) {
            require(std::abs(object->modelViewMatrix.elements()[14] + 5) < 1e-10,
                    "Material hook observes the updated model-view matrix");
            basic(hooked)->color.set(0, 1, 0);
            callbackOrder.push_back("material-before");
        };
        hooked->onAfterRender = [&](auto&, auto&, auto&, auto*, auto*, auto*) {
            callbackOrder.push_back("object-after");
        };
        scene.onAfterRender = [&](auto&, auto&, auto&, auto*, auto*, auto*) {
            callbackOrder.push_back("scene-after");
        };
        renderHostTurn(renderer, scene, camera);
        require(callbackOrder == std::vector<std::string>{"scene-before", "object-before",
                                                          "material-before", "object-after",
                                                          "scene-after"},
                "Render hooks follow three.js scene/object/material order");
        expectPixel(renderer, {0, 255, 0, 255},
                    "Material changes in a render hook reach that draw");
        hooked->onBeforeRender = [&](auto&, auto&, auto&, auto* geometry, auto*, auto*) {
            geometry->setDrawRange(0, 0);
        };
        renderHostTurn(renderer, scene, camera);
        require(renderer.getInfo().calls == 1 && renderer.getInfo().triangles == 0,
                "A callback changing drawRange to zero retains the zero-count draw submission");
        scene.onBeforeRender = {};
        scene.onAfterRender = {};
        hooked->onBeforeRender = {};
        hooked->onAfterRender = {};
        basic(hooked)->onBeforeRender = {};
        hooked->geometry->setDrawRange(0, 6);
        basic(hooked)->alphaHash = true;
        bool rejected = false;
        try {
            renderHostTurn(renderer, scene, camera);
        } catch (const std::invalid_argument&) {
            rejected = true;
        }
        require(
            rejected,
            "Unsupported alphaHash is rejected instead of silently rendering a different material");
        basic(hooked)->alphaHash = false;
        renderHostTurn(renderer, scene, camera);
        expectPixel(renderer, {0, 255, 0, 255},
                    "Renderer remains usable after unsupported input rejection");
        scene.clear();
        {
            auto first =
                std::make_shared<Mesh>(std::make_shared<PlaneGeometry>(0.8, 1.6),
                                       std::make_shared<MeshBasicMaterial>(Color(1, 0, 0)));
            auto second =
                std::make_shared<Mesh>(std::make_shared<PlaneGeometry>(0.8, 1.6),
                                       std::make_shared<MeshBasicMaterial>(Color(0, 1, 0)));
            first->position.set(-0.5, 0, 0);
            second->position.set(0.5, 0, 0);
            bool secondHookCalled = false;
            first->onBeforeRender = [&](auto&, auto&, auto&, auto*, auto*, auto*) {
                second->layers.set(1);
            };
            second->onBeforeRender = [&](auto&, auto&, auto&, auto*, auto*, auto*) {
                secondHookCalled = true;
            };
            scene.add(first);
            scene.add(second);
            renderHostTurn(renderer, scene, camera);
            expectPixel(renderer, {255, 0, 0, 255}, "The first layer mutation draw remains visible",
                        16, 32);
            expectPixel(renderer, {0, 0, 0, 255},
                        "Layers are rechecked after earlier object callbacks", 48, 32);
            require(!secondHookCalled && renderer.getInfo().calls == 1,
                    "A queued object with newly unmatched layers skips hooks and draw");
            first->onBeforeRender = {};
            second->onBeforeRender = {};
            second->layers.set(0);
            auto originalOverride = std::make_shared<MeshBasicMaterial>(Color(0, 1, 0));
            auto laterOverride = std::make_shared<MeshBasicMaterial>(Color(0, 0, 1));
            scene.overrideMaterial = originalOverride;
            first->onBeforeRender = [&](auto&, auto&, auto&, auto*, auto*, auto*) {
                scene.overrideMaterial = laterOverride;
            };
            renderHostTurn(renderer, scene, camera);
            expectPixel(renderer, {0, 255, 0, 255},
                        "Override material is cached for the current opaque list", 48, 32);
            scene.overrideMaterial = originalOverride;
            basic(second)->transparent = true;
            renderHostTurn(renderer, scene, camera);
            expectPixel(renderer, {0, 255, 0, 255}, "The opaque list keeps its original override",
                        16, 32);
            expectPixel(renderer, {0, 0, 255, 255},
                        "The transparent list captures the later override", 48, 32);
            scene.overrideMaterial.reset();
            scene.clear();
        }
        {
            auto changingClear = plane(Color(1, 0, 0));
            scene.add(changingClear);
            changingClear->onBeforeRender = [](auto& activeRenderer, auto&, auto&, auto*, auto*,
                                               auto*) {
                activeRenderer.setClearColor(Color(0, 0, 1));
            };
            renderHostTurn(renderer, scene, camera);
            expectPixel(renderer, {0, 0, 0, 255},
                        "Object hooks change clear color after this frame's background clear", 0,
                        0);
            renderHostTurn(renderer, scene, camera);
            expectPixel(renderer, {0, 0, 255, 255},
                        "The changed clear color applies to the next frame", 0, 0);
            renderer.setClearColor(Color(0, 0, 0));
            scene.clear();
        }
        {
            auto clearing = plane(Color(1, 0, 0));
            auto farther = plane(Color(0, 1, 0));
            clearing->position.set(0, 0, 1);
            bool clearDepth = false;
            clearing->onBeforeRender = [&](auto& activeRenderer, auto&, auto&, auto*, auto*,
                                           auto*) {
                expectPixel(activeRenderer, {0, 0, 0, 255},
                            "Readback inside onBeforeRender sees the current background clear");
            };
            clearing->onAfterRender = [&](auto& activeRenderer, auto&, auto&, auto*, auto*, auto*) {
                expectPixel(activeRenderer, {255, 0, 0, 255},
                            "Readback inside onAfterRender sees the current draw");
                activeRenderer.setClearColor(Color(0, 0, 1));
                activeRenderer.clear(true, clearDepth);
                expectPixel(activeRenderer, {0, 0, 255, 255},
                            "Clear inside onAfterRender follows the current draw");
            };
            scene.add(clearing);
            scene.add(farther);
            renderHostTurn(renderer, scene, camera);
            expectPixel(renderer, {0, 0, 255, 255},
                        "Callback readback and color clear preserve depth for the next draw");
            renderer.setClearColor(Color(0, 0, 0));
            clearDepth = true;
            renderHostTurn(renderer, scene, camera);
            expectPixel(renderer, {0, 255, 0, 255},
                        "Callback depth clear allows the following farther draw");
            renderer.setClearColor(Color(0, 0, 0));
            scene.clear();
        }
        {
            auto masked = plane(Color(1, 0, 0));
            basic(masked)->colorWrite = false;
            masked->onAfterRender = [](auto& activeRenderer, auto&, auto&, auto*, auto*, auto*) {
                activeRenderer.setClearColor(Color(0, 0, 1));
                activeRenderer.clear(true, false);
            };
            scene.add(masked);
            renderHostTurn(renderer, scene, camera);
            expectPixel(renderer, {0, 0, 0, 255},
                        "Manual callback clear respects the last material's colorWrite mask");
            renderer.clear();
            expectPixel(renderer, {0, 0, 0, 255},
                        "Manual clear outside rendering also respects colorWrite");
            scene.clear();
            renderHostTurn(renderer, scene, camera);
            expectPixel(renderer, {0, 0, 255, 255},
                        "Automatic background clear restores color writes");
            renderer.setClearColor(Color(0, 0, 0));
        }
        {
            scene.background = Color(0, 0, 1);
            auto backgroundClear = plane(Color(1, 0, 0));
            backgroundClear->onAfterRender = [](auto& activeRenderer, auto&, auto&, auto*, auto*,
                                                auto*) { activeRenderer.clear(true, false); };
            scene.add(backgroundClear);
            renderHostTurn(renderer, scene, camera);
            expectPixel(renderer, {0, 0, 255, 255},
                        "Callback clear uses the active scene background clear color");
            require(renderer.getClearColor().equals(Color(0, 0, 0)),
                    "Scene background leaves the user's clear color unchanged");
            renderer.clear();
            expectPixel(renderer, {0, 0, 255, 255},
                        "Manual clear after rendering retains the active background clear state");
            scene.background.reset();
            scene.clear();
            renderHostTurn(renderer, scene, camera);
            expectPixel(
                renderer, {0, 0, 0, 255},
                "Removing scene background restores the user's clear color on the next frame");
        }
        {
            auto guarded = plane(Color(1, 0, 0));
            bool renderRejected = false, resizeRejected = false;
            guarded->onBeforeRender = [&](auto& activeRenderer, auto& activeScene,
                                          auto& activeCamera, auto*, auto*, auto*) {
                try {
                    renderHostTurn(activeRenderer, activeScene, activeCamera);
                } catch (const std::logic_error&) {
                    renderRejected = true;
                }
                try {
                    activeRenderer.setSize(8, 8);
                } catch (const std::logic_error&) {
                    resizeRejected = true;
                }
            };
            scene.add(guarded);
            renderHostTurn(renderer, scene, camera);
            require(
                renderRejected && resizeRejected,
                "Unsafe nested render and resize are explicitly rejected during active callbacks");
            expectPixel(renderer, {255, 0, 0, 255},
                        "The outer frame continues after caught callback-operation rejections");
            scene.clear();
        }
        std::weak_ptr<Mesh> detachedMesh;
        bool detachedBefore = false, detachedAfter = false;
        {
            auto transient = plane(Color(1, 0, 0));
            auto* pointer = transient.get();
            detachedMesh = transient;
            transient->onBeforeRender = [&, pointer](auto&, auto&, auto&, auto*, auto*, auto*) {
                detachedBefore = true;
                pointer->removeFromParent();
            };
            transient->onAfterRender = [&](auto&, auto&, auto&, auto*, auto*, auto*) {
                detachedAfter = true;
                require(!detachedMesh.expired(),
                        "A detached render item remains alive through onAfterRender");
            };
            scene.add(transient);
        }
        renderHostTurn(renderer, scene, camera);
        expectPixel(renderer, {255, 0, 0, 255},
                    "A scene-owned mesh detached by onBeforeRender still draws this frame");
        finishHostTurn();
        require(detachedBefore && detachedAfter && detachedMesh.expired(),
                "Render item lifetime ends safely after both hooks and the frame");
        renderHostTurn(renderer, scene, camera);
        require(renderer.getInfo().calls == 0,
                "A detached scene-owned mesh is absent from the next frame");
        expectPixel(renderer, {0, 0, 0, 255}, "The next frame clears the detached mesh's pixels");

        auto managedScene = std::make_shared<Scene>();
        std::weak_ptr<Scene> releasedScene = managedScene;
        std::weak_ptr<Camera> releasedCamera;
        std::weak_ptr<Mesh> releasedFirst, releasedSecond;
        Camera* managedCameraPointer = nullptr;
        bool afterReleasedScene = false;
        {
            auto managedCamera = std::make_shared<OrthographicCamera>(-1, 1, 1, -1, 0.1, 20);
            managedCamera->position.set(0, 0, 5);
            managedCameraPointer = managedCamera.get();
            releasedCamera = managedCamera;
            managedScene->add(managedCamera);
            auto first =
                std::make_shared<Mesh>(std::make_shared<PlaneGeometry>(0.8, 1.6),
                                       std::make_shared<MeshBasicMaterial>(Color(1, 0, 0)));
            auto second =
                std::make_shared<Mesh>(std::make_shared<PlaneGeometry>(0.8, 1.6),
                                       std::make_shared<MeshBasicMaterial>(Color(0, 1, 0)));
            first->position.set(-0.5, 0, 0);
            second->position.set(0.5, 0, 0);
            releasedFirst = first;
            releasedSecond = second;
            first->onBeforeRender = [&](auto&, auto& activeScene, auto&, auto*, auto*, auto*) {
                activeScene.clear();
                managedScene.reset();
            };
            second->onBeforeRender = [&](auto&, auto&, auto&, auto*, auto*, auto*) {
                require(
                    !releasedFirst.expired() && !releasedSecond.expired() &&
                        !releasedCamera.expired() && !releasedScene.expired(),
                    "Clearing the scene retains queued meshes, camera and scene during rendering");
            };
            managedScene->onAfterRender = [&](auto&, auto&, auto&, auto*, auto*, auto*) {
                afterReleasedScene = true;
                require(!releasedScene.expired() && !releasedCamera.expired(),
                        "Managed scene and camera survive through the scene's final hook");
            };
            managedScene->add(first);
            managedScene->add(second);
        }
        renderHostTurn(renderer, *managedScene, *managedCameraPointer);
        expectPixel(renderer, {255, 0, 0, 255},
                    "Clearing scene ownership during a hook preserves the first queued draw", 16,
                    32);
        expectPixel(renderer, {0, 255, 0, 255},
                    "Clearing scene ownership during a hook preserves subsequent queued draws", 48,
                    32);
        finishHostTurn();
        require(afterReleasedScene && releasedScene.expired() && releasedCamera.expired() &&
                    releasedFirst.expired() && releasedSecond.expired(),
                "Managed render objects are released after the frame without ownership cycles");
        auto square = std::make_shared<BufferGeometry>();
        square->setAttribute(
            "position",
            std::make_shared<Float32BufferAttribute>(
                std::vector<float>{-.8f, -.8f, 0, .8f, -.8f, 0, .8f, .8f, 0, -.8f, .8f, 0}, 3));
        square->setIndex(std::vector<std::uint32_t>{0, 1, 2, 0, 2, 3});
        square->addGroup(0, 3, 0);
        square->addGroup(3, 3, 1);
        auto red = std::make_shared<MeshBasicMaterial>(Color(1, 0, 0)),
             green = std::make_shared<MeshBasicMaterial>(Color(0, 1, 0));
        auto groupedMesh =
            std::make_shared<Mesh>(square, std::vector<std::shared_ptr<Material>>{red, green});
        scene.add(groupedMesh);
        renderHostTurn(renderer, scene, camera);
        expectPixel(renderer, {255, 0, 0, 255}, "Geometry group zero selects material zero", 48,
                    48);
        expectPixel(renderer, {0, 255, 0, 255}, "Geometry group one selects material one", 16, 16);
        groupedMesh->onBeforeRender = [&](auto&, auto&, auto&, auto*, auto*, auto* group) {
            if (group->materialIndex == 0) {
                require(group == &square->groups[0],
                        "Render callbacks receive the original geometry group object");
                group->count = 0;
            }
        };
        renderHostTurn(renderer, scene, camera);
        expectPixel(renderer, {0, 0, 0, 255},
                    "Changing the original group count in a hook changes this frame's draw", 48,
                    48);
        expectPixel(renderer, {0, 255, 0, 255},
                    "Mutating one queued group leaves the other group visible", 16, 16);
        require(renderer.getInfo().calls == 2 && renderer.getInfo().triangles == 1,
                "The mutated zero-count group retains its draw call with no triangles");
        square->groups[0].count = 3;
        std::weak_ptr<BufferGeometry::Group> queuedFirst = square->groups.owner(square->groups[0]);
        std::weak_ptr<BufferGeometry::Group> queuedSecond = square->groups.owner(square->groups[1]);
        const auto* queuedSecondPointer = &square->groups[1];
        groupedMesh->onBeforeRender = [&](auto&, auto&, auto&, auto*, auto*, auto* group) {
            if (group->materialIndex == 0) {
                square->clearGroups();
                require(!queuedFirst.expired() && !queuedSecond.expired(),
                        "Clearing geometry groups retains both queued group objects");
            } else {
                require(group == queuedSecondPointer && group->count == 3,
                        "A later callback receives the original group after clearGroups");
            }
        };
        renderHostTurn(renderer, scene, camera);
        expectPixel(renderer, {255, 0, 0, 255},
                    "Clearing groups in a hook preserves the current queued group draw", 48, 48);
        expectPixel(renderer, {0, 255, 0, 255},
                    "Clearing groups in a hook preserves a later queued group draw", 16, 16);
        require(queuedFirst.expired() && queuedSecond.expired(),
                "Cleared groups are released when this frame's render items expire");
        renderHostTurn(renderer, scene, camera);
        require(renderer.getInfo().calls == 0,
                "Cleared geometry groups are absent from the next frame");
        expectPixel(renderer, {0, 0, 0, 255},
                    "The next frame clears pixels from removed geometry groups");
        groupedMesh->onBeforeRender = {};
        square->addGroup(0, 3, 0);
        square->addGroup(3, 3, 1);
        square->setIndex(std::make_shared<Uint16BufferAttribute>(
            std::vector<std::uint16_t>{0, 1, 2, 0, 2, 3}, 1, true));
        renderHostTurn(renderer, scene, camera);
        expectPixel(renderer, {255, 0, 0, 255},
                    "Index fetching ignores the vertex attribute normalized flag", 48, 48);
        square->setIndex(
            std::make_shared<Int32BufferAttribute>(std::vector<std::int32_t>{0, 1, 2, 0, 2, 3}, 1));
        rejected = false;
        try {
            renderHostTurn(renderer, scene, camera);
        } catch (const std::invalid_argument&) {
            rejected = true;
        }
        require(rejected, "Signed integer index types are rejected instead of silently converted");
        square->setIndex(std::vector<std::uint32_t>{0, 1, 2, 0, 2, 3});
        square->setDrawRange(3, 3);
        renderHostTurn(renderer, scene, camera);
        expectPixel(renderer, {0, 0, 0, 255}, "drawRange intersects geometry groups", 48, 48);
        expectPixel(renderer, {0, 255, 0, 255}, "The surviving group retains its material", 16, 16);
        scene.overrideMaterial = std::make_shared<MeshBasicMaterial>(Color(0, 0, 1));
        renderHostTurn(renderer, scene, camera);
        expectPixel(renderer, {0, 0, 255, 255}, "Scene overrideMaterial replaces a group material",
                    16, 16);
        scene.overrideMaterial.reset();
        scene.background = Color(1, 0, 1);
        renderer.autoClear = false;
        scene.clear();
        renderHostTurn(renderer, scene, camera);
        expectPixel(renderer, {255, 0, 255, 255},
                    "A solid scene background forces color clear with autoClear false");
        scene.background.reset();
        renderer.autoClear = true;
        renderHostTurn(renderer, scene, camera);
        auto attributeBufferBaseline = renderer.getInfo().attributeBuffers;
        std::weak_ptr<BufferGeometry> expiredGeometry;
        std::weak_ptr<BufferAttribute> expiredAttribute;
        {
            auto geometry = std::make_shared<BufferGeometry>();
            auto positions = std::make_shared<Float32BufferAttribute>(
                std::vector<float>{-.8f, -.8f, 0, .8f, -.8f, 0, .8f, .8f, 0,
                                   -.8f, -.8f, 0, .8f, .8f, 0, -.8f, .8f, 0}, 3);
            auto colors = std::make_shared<Float32BufferAttribute>(
                std::vector<float>{1, 0, 0, 1, 0, 0, 1, 0, 0,
                                   0, 1, 0, 0, 1, 0, 0, 1, 0}, 3);
            geometry->setAttribute("position", positions).setAttribute("color", colors);
            auto material = std::make_shared<MeshBasicMaterial>(Color(1, 1, 1));
            material->vertexColors = true;
            auto cached = std::make_shared<Mesh>(geometry, material);
            scene.add(cached);
            int positionUploads = 0, colorUploads = 0;
            bool initialRangesRetained = false, updateRangesCleared = false;
            positions->onUpload([&] { ++positionUploads; });
            colors->addUpdateRange(0, 3);
            colors->onUpload([&] {
                ++colorUploads;
                if (colorUploads == 1)
                    initialRangesRetained = colors->updateRanges.get("length").strictEquals(1);
                else
                    updateRangesCleared = colors->updateRanges.get("length").strictEquals(0);
            });
            const auto idBeforeUpload = std::make_shared<Float32BufferAttribute>(std::vector<float>{0}, 1);
            renderHostTurn(renderer, scene, camera);
            const auto idAfterUpload = std::make_shared<Float32BufferAttribute>(std::vector<float>{0}, 1);
            require(idAfterUpload->id == idBeforeUpload->id + 1,
                    "GPU uploads do not allocate BufferAttribute identities for internal snapshots");
            expectPixel(renderer, {255, 0, 0, 255}, "First attribute upload uses the complete array", 48, 48);
            expectPixel(renderer, {0, 255, 0, 255}, "Initial updateRanges do not restrict first upload", 16, 16);
            require(initialRangesRetained && positionUploads == 1 && colorUploads == 1,
                    "First uploads invoke callbacks once and retain updateRanges");
            require(renderer.getInfo().attributeBuffers == attributeBufferBaseline + 2,
                    "GPU attribute buffers persist independently of draw buffers");
            auto& colorArray = std::get<TypedArray<float>>(colors->array);
            for (std::size_t i = 0; i < 9; i += 3) {
                colorArray[i] = 0;
                colorArray[i + 2] = 1;
                colorArray[i + 9] = 1;
                colorArray[i + 10] = 0;
            }
            renderHostTurn(renderer, scene, camera);
            expectPixel(renderer, {255, 0, 0, 255}, "Array writes without needsUpdate preserve cached GPU color", 48, 48);
            require(positionUploads == 1 && colorUploads == 1 &&
                        renderer.getInfo().attributeBuffers == attributeBufferBaseline + 2,
                    "Unchanged versions reuse buffers and skip upload callbacks");
            colors->addUpdateRange(6, 3);
            colors->addUpdateRange(0, 4);
            colors->addUpdateRange(3, 3);
            colors->needsUpdate = true;
            renderHostTurn(renderer, scene, camera);
            expectPixel(renderer, {0, 0, 255, 255}, "Overlapping unsorted ranges update the selected color components", 48, 48);
            expectPixel(renderer, {0, 255, 0, 255}, "Partial uploads preserve unlisted GPU components", 16, 16);
            require(colorUploads == 2 && updateRangesCleared && colors->updateRanges.get("length").strictEquals(0),
                    "Version updates merge and clear ranges before the upload callback");
            colors->needsUpdate = true;
            renderHostTurn(renderer, scene, camera);
            expectPixel(renderer, {255, 0, 0, 255}, "An empty updateRanges list uploads the whole changed array", 16, 16);
            require(colorUploads == 3, "Full version updates invoke one upload callback");
            auto& positionArray = std::get<TypedArray<float>>(positions->array);
            for (std::size_t i = 0; i < positionArray.size(); i += 3)
                positionArray[i] = static_cast<float>(positionArray[i]) + 3;
            renderHostTurn(renderer, scene, camera);
            expectPixel(renderer, {0, 0, 255, 255}, "Position array writes also require needsUpdate", 48, 48);
            positions->needsUpdate = true;
            renderHostTurn(renderer, scene, camera);
            expectPixel(renderer, {0, 0, 0, 255}, "Position version changes reach the GPU", 48, 48);
            require(positionUploads == 2, "Position buffers upload only at first use and version changes");
            const auto originalColorArray = colorArray;
            std::vector<float> largerColorArray(colorArray.begin(), colorArray.end());
            largerColorArray.push_back(1);
            colors->array = TypedArray<float>(largerColorArray);
            colors->needsUpdate = true;
            bool resizeRejected = false;
            try { renderHostTurn(renderer, scene, camera); }
            catch (const std::runtime_error& error) { resizeRejected = std::string(error.what()).find("Resizing buffer attributes is not supported") != std::string::npos; }
            require(resizeRejected, "Uploaded attribute array resizing is rejected like WebGLAttributes");
            colors->array = originalColorArray;
            colors->needsUpdate = true;
            renderHostTurn(renderer, scene, camera);
            require(colorUploads == 4, "A corrected array remains uploadable after size rejection");
            const auto rawBeforeColorDispose=renderer.getInfo().attributeUploadBuffers;
            colors->dispose();
            require(renderer.getInfo().attributeBuffers == attributeBufferBaseline + 1,
                    "Attribute dispose releases its actual cached GPU buffer");
            renderHostTurn(renderer, scene, camera);
            require(colorUploads == 5 && renderer.getInfo().attributeUploadBuffers == rawBeforeColorDispose &&
                    renderer.getInfo().attributeBuffers == attributeBufferBaseline + 1,
                    "Reusing a disposed attribute uploads raw storage while unchanged VAO identity retains its detached color slot");
            geometry->dispose();
            require(renderer.getInfo().attributeBuffers == attributeBufferBaseline,
                    "Geometry dispose releases the buffers of its current attributes");
            renderHostTurn(renderer, scene, camera);
            require(positionUploads == 3 && colorUploads == 6,
                    "A disposed geometry registers and uploads again on its next render");
            expiredGeometry = geometry;
            expiredAttribute = positions;
            scene.clear();
        }
        require(!expiredGeometry.expired() && !expiredAttribute.expired(),
                "Geometry WeakRef registration retains its target and native graph through the current host job");
        finishHostTurn();
        require(expiredGeometry.expired() && expiredAttribute.expired(),
                "Attribute and geometry caches do not retain application ownership");
        renderHostTurn(renderer, scene, camera);
        require(renderer.getInfo().attributeBuffers == attributeBufferBaseline + 2,
                "Pinned binding caches retain the two uploaded vertex resources after geometry GC");
        attributeBufferBaseline = renderer.getInfo().attributeBuffers;
        {
            const std::vector<float> originalPositions = {-.4f, -.8f, 0, .4f, -.8f, 0,
                                                         .4f, .8f, 0, -.4f, .8f, 0};
            const std::vector<std::uint16_t> originalIndices = {0, 1, 2, 0, 2, 3};
            auto positions = std::make_shared<Float32BufferAttribute>(originalPositions, 3);
            auto colors = std::make_shared<Float32BufferAttribute>(
                std::vector<float>{1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0}, 3);
            auto indices = std::make_shared<Uint16BufferAttribute>(originalIndices, 1);
            auto firstGeometry = std::make_shared<BufferGeometry>();
            auto secondGeometry = std::make_shared<BufferGeometry>();
            firstGeometry->setAttribute("position", positions).setAttribute("color", colors).setIndex(indices);
            secondGeometry->setAttribute("position", positions).setAttribute("color", colors).setIndex(indices);
            auto material = std::make_shared<MeshBasicMaterial>(Color(1, 1, 1));
            material->vertexColors = true;
            auto first = std::make_shared<Mesh>(firstGeometry, material);
            auto second = std::make_shared<Mesh>(secondGeometry, material);
            first->position.x = -.5;
            second->position.x = .5;
            first->renderOrder = 0;
            second->renderOrder = 1;
            scene.add(first);
            scene.add(second);
            int positionUploads = 0, colorUploads = 0, indexUploads = 0;
            std::vector<std::string> uploadOrder;
            positions->onUpload([&] { ++positionUploads; uploadOrder.push_back("position"); });
            colors->onUpload([&] { ++colorUploads; uploadOrder.push_back("color"); });
            indices->onUpload([&] { ++indexUploads; uploadOrder.push_back("index"); });
            first->onBeforeRender = [&](auto&, auto&, auto&, auto*, auto*, auto*) { uploadOrder.push_back("before"); };
            first->onAfterRender = [&](auto&, auto&, auto&, auto*, auto*, auto*) { uploadOrder.push_back("after"); };
            renderHostTurn(renderer, scene, camera);
            require(uploadOrder == std::vector<std::string>{"position", "color", "before", "index", "after"},
                    "Ordinary attributes upload before object hooks and indices upload during draw binding");
            require(positionUploads == 1 && colorUploads == 1 && indexUploads == 1 &&
                        renderer.getInfo().attributeBuffers == attributeBufferBaseline + 3,
                    "Geometries sharing actual attributes share their GPU buffers and callbacks");
            expectPixel(renderer, {255, 0, 0, 255}, "The first geometry reads shared cached attributes", 16, 32);
            expectPixel(renderer, {255, 0, 0, 255}, "The second geometry reads the same attribute identity", 48, 32);
            first->onBeforeRender = {};
            first->onAfterRender = {};
            for (std::size_t i = 0; i < 4; ++i)
                colors->setXYZ(i, 0, 1, 0);
            renderHostTurn(renderer, scene, camera);
            expectPixel(renderer, {255, 0, 0, 255}, "Shared color writes stay stale without a version increment", 48, 32);
            colors->needsUpdate = true;
            renderHostTurn(renderer, scene, camera);
            expectPixel(renderer, {0, 255, 0, 255}, "A shared attribute version updates the first geometry", 16, 32);
            expectPixel(renderer, {0, 255, 0, 255}, "A shared attribute version updates the second geometry", 48, 32);
            require(colorUploads == 2, "A shared version change uploads once across multiple geometries");
            std::get<TypedArray<std::uint16_t>>(indices->array).assign(6, 0);
            renderHostTurn(renderer, scene, camera);
            expectPixel(renderer, {0, 255, 0, 255}, "Index array writes also preserve cached data without needsUpdate", 16, 32);
            indices->addUpdateRange(0, 3);
            indices->needsUpdate = true;
            renderHostTurn(renderer, scene, camera);
            expectPixel(renderer, {0, 0, 0, 255}, "Index updateRanges change the selected triangle", 22, 44);
            expectPixel(renderer, {0, 255, 0, 255}, "Index updateRanges retain the unlisted triangle", 10, 20);
            require(indices->updateRanges.get("length").strictEquals(0) && indexUploads == 2,
                    "Partial index updates clear ranges and invoke one callback");
            indices->set(originalIndices);
            indices->needsUpdate = true;
            renderHostTurn(renderer, scene, camera);
            bool changedIndex = false;
            first->onAfterRender = [&](auto&, auto&, auto&, auto*, auto*, auto*) {
                if (!changedIndex) {
                    std::get<TypedArray<std::uint16_t>>(indices->array).assign(6, 0);
                    indices->needsUpdate = true;
                    changedIndex = true;
                }
            };
            renderHostTurn(renderer, scene, camera);
            expectPixel(renderer, {0, 255, 0, 255}, "Later index uploads preserve data referenced by earlier recorded draws", 16, 32);
            expectPixel(renderer, {0, 0, 0, 255}, "A hook's index version change affects the following draw", 48, 32);
            require(indexUploads == 4 && renderer.getInfo().attributeBuffers == attributeBufferBaseline + 3,
                    "Replacement buffers are released after the queued draws finish");
            first->onAfterRender = {};
            indices->set(originalIndices);
            indices->needsUpdate = true;
            bool changedPosition = false;
            first->onBeforeRender = [&](auto&, auto&, auto&, auto*, auto*, auto*) {
                if (!changedPosition) {
                    auto& values = std::get<TypedArray<float>>(positions->array);
                    for (std::size_t i = 0; i < values.size(); i += 3)
                        values[i] = static_cast<float>(values[i]) + 3;
                    positions->needsUpdate = true;
                    changedPosition = true;
                }
            };
            renderHostTurn(renderer, scene, camera);
            expectPixel(renderer, {0, 255, 0, 255}, "Ordinary attribute changes in object hooks wait for the next geometry update", 16, 32);
            expectPixel(renderer, {0, 255, 0, 255}, "A later draw shares the unchanged projected attribute version", 48, 32);
            first->onBeforeRender = {};
            renderHostTurn(renderer, scene, camera);
            expectPixel(renderer, {0, 0, 0, 255}, "The following frame uploads the pending position version", 48, 32);
            require(positionUploads == 2, "Ordinary shared position updates happen once per projected frame");
            positions->set(originalPositions);
            positions->needsUpdate = true;
            renderHostTurn(renderer, scene, camera);
            scene.remove(first);
            firstGeometry->dispose();
            require(renderer.getInfo().attributeBuffers == attributeBufferBaseline,
                    "Disposing one geometry removes shared attribute cache entries like WebGLGeometries");
            renderHostTurn(renderer, scene, camera);
            expectPixel(renderer, {0, 0, 0, 255}, "Disposing the other geometry detaches deleted buffers from the surviving current VAO", 48, 32);
            require(positionUploads == 4 && colorUploads == 3 && indexUploads == 6,
                    "Shared buffers receive a new initial upload after geometry disposal");
            scene.clear();
        }
        renderHostTurn(renderer, scene, camera);
        require(renderer.getInfo().attributeBuffers == attributeBufferBaseline,
                "Deleted shared vertex bindings create no replacement conversion buffers without a pointer refresh");
        attributeBufferBaseline = renderer.getInfo().attributeBuffers;
        {
            auto geometry = std::make_shared<PlaneGeometry>(1.6, 1.6);
            const auto positions = std::dynamic_pointer_cast<BufferAttribute>(geometry->getAttribute("position"));
            int uploads = 0;
            positions->onUpload([&] {
                if (++uploads == 1) {
                    auto& values = std::get<TypedArray<float>>(positions->array);
                    for (std::size_t i = 0; i < values.size(); i += 3)
                        values[i] = static_cast<float>(values[i]) + 3;
                    positions->needsUpdate = true;
                }
            });
            scene.add(std::make_shared<Mesh>(geometry, std::make_shared<MeshBasicMaterial>(Color(1, 0, 0))));
            renderHostTurn(renderer, scene, camera);
            expectPixel(renderer, {255, 0, 0, 255}, "Upload callbacks run after data is captured for the GPU");
            renderHostTurn(renderer, scene, camera);
            expectPixel(renderer, {255, 0, 0, 255}, "Cached version is captured after onUploadCallback like upstream");
            require(uploads == 1, "A version increment inside the first upload callback is already cached");
            positions->needsUpdate = true;
            renderHostTurn(renderer, scene, camera);
            expectPixel(renderer, {0, 0, 0, 255}, "A later explicit version increment uploads callback array changes");
            require(uploads == 2, "Later version changes still invoke onUploadCallback");
            scene.clear();
        }
        renderHostTurn(renderer, scene, camera);
        require(renderer.getInfo().attributeBuffers == attributeBufferBaseline + 1,
                "The upload callback fixture leaves one vertex resource in the pinned binding cache");
        {
            Scene uploadScene;
            auto geometry = std::make_shared<PlaneGeometry>(1.6, 1.6);
            const auto positions = std::dynamic_pointer_cast<BufferAttribute>(geometry->getAttribute("position"));
            uploadScene.add(std::make_shared<Mesh>(geometry, std::make_shared<MeshBasicMaterial>(Color(1, 0, 0))));
            auto callbackRenderer = std::make_unique<VulkanRenderer>(VulkanRendererParameters{64, 64, validation});
            bool uploadRenderRejected = false, uploadResizeRejected = false;
            positions->onUpload([&] {
                try { callbackRenderer->render(uploadScene, camera); }
                catch (const std::logic_error&) { uploadRenderRejected = true; }
                try { callbackRenderer->setSize(8, 8); }
                catch (const std::logic_error&) { uploadResizeRejected = true; }
            });
            callbackRenderer->render(uploadScene, camera);
            require(uploadRenderRejected && uploadResizeRejected,
                    "The complete render guard rejects nested render and resize from upload callbacks");
            expectPixel(*callbackRenderer, {255, 0, 0, 255},
                        "Caught upload callback operation errors preserve the outer frame");
            int attributeDisposes = 0, geometryDisposes = 0;
            positions->addEventListener("dispose", [&](auto&) { ++attributeDisposes; });
            geometry->addEventListener("dispose", [&](auto&) { ++geometryDisposes; });
            callbackRenderer.reset();
            positions->dispose();
            geometry->dispose();
            require(attributeDisposes == 1 && geometryDisposes == 1,
                    "Renderer destruction removes upload cache listeners before later resource disposal");
        }
        scene.add(plane(Color(1, 0, 0)));
        camera.coordinateSystem = WebGPUCoordinateSystem;
        camera.updateProjectionMatrix();
        renderHostTurn(renderer, scene, camera);
        expectPixel(renderer, {255, 0, 0, 255}, "WebGPU zero-to-one projection depth is preserved");
        camera.reversedDepth = true;
        camera.updateProjectionMatrix();
        renderHostTurn(renderer, scene, camera);
        expectPixel(renderer, {255, 0, 0, 255},
                    "Reversed projection uses reversed depth clear and comparison");
        camera.coordinateSystem = WebGLCoordinateSystem;
        camera.updateProjectionMatrix();
        auto depthProbe = std::dynamic_pointer_cast<Mesh>(scene.children[0]);
        depthProbe->frustumCulled = false;
        depthProbe->position.set(0, 0, -17);
        renderHostTurn(renderer, scene, camera);
        expectPixel(
            renderer, {0, 0, 0, 255},
            "Default WebGL reversed projection clips beyond far without an extra depth remap");
        depthProbe->position.set(0, 0, 4.95);
        renderHostTurn(renderer, scene, camera);
        expectPixel(renderer, {0, 0, 0, 255},
                    "Default WebGL reversed projection clips before near");
        depthProbe->position.set(0, 0, 4.8);
        renderHostTurn(renderer, scene, camera);
        expectPixel(renderer, {255, 0, 0, 255},
                    "Default WebGL reversed projection preserves geometry just beyond near");
        depthProbe->position.set(0, 0, -14.8);
        renderHostTurn(renderer, scene, camera);
        expectPixel(renderer, {255, 0, 0, 255},
                    "Default WebGL reversed projection preserves geometry just before far");
        depthProbe->frustumCulled = true;
        depthProbe->position.set(0, 0, -17);
        renderHostTurn(renderer, scene, camera);
        require(renderer.getInfo().calls == 0,
                "Default WebGL reversed projection extracts the zero-to-one far frustum plane");
        depthProbe->position.set(0, 0, 0);
        renderer.setSize(32, 48);
        renderHostTurn(renderer, scene, camera);
        require(renderer.readPixels().size() == 32 * 48 * 4,
                "Resize recreates color, depth, framebuffer and readback resources");
        expectPixel(renderer, {255, 0, 0, 255}, "Resized framebuffer receives geometry", 16, 24);
        for (const auto& message : renderer.getValidationMessages())
            std::cerr << message << '\n';
        require(renderer.getValidationMessages().empty(),
                "Vulkan validation emitted a warning or error");
        std::cout << "GPU: " << renderer.getInfo().deviceName << "; " << assertions
                  << " assertions passed; validation=" << renderer.getInfo().validationEnabled
                  << '\n';
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "Vulkan renderer test failed: " << error.what() << '\n';
        return 1;
    }
}
