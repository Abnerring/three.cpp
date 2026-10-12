#include "catch.hpp"
#include "materials/Materials.h"
#include "core/NativeObject.h"
#include "core/RenderTarget.h"
#include "animation/AnimationRuntime.h"
#include "animation/AnimationClip.h"
#include "animation/PropertyMixer.h"
#include "lights/SpotLight.h"
#include "math/Plane.h"
#include "math/interpolants/BezierInterpolant.h"
#include "objects/SkinnedMesh.h"
#include "scenes/Scene.h"
#include "textures/Texture.h"
#include <limits>

namespace {
JSValue uniform(JSValue value) { auto result=JSValue::object();result.set("value",value);return result; }
JSValue customClone(JSValue value) {
    auto result=JSValue::object();result.set("isVector2",true);result.set("value",value);
    result.set("clone",JSValue::function([](const JSValue& self,const std::vector<JSValue>&,const std::vector<JSValue>&){auto result=JSValue::object();result.set("copied",self.get("value"));return result;}));
    return result;
}
void linkTypedArray(const AnimationArray& array,const JSValue& parent) {
    auto wrapper=nativeArray(array);wrapper.set("parent",parent);wrapper.set("extension",37);
    auto buffer=wrapper.get("buffer");buffer.set("parent",parent);buffer.set("extension",91);
}
void checkTypedArray(const AnimationArray& array) {
    auto wrapper=nativeArray(array);
    REQUIRE(wrapper.get("extension").strictEquals(JSValue(37)));
    REQUIRE(wrapper.get("buffer").get("extension").strictEquals(JSValue(91)));
}
}

TEST_CASE("Material needsUpdate uses strict true and Number version", "[materials]") {
    Material material;
    material.needsUpdate=1;material.needsUpdate="true";material.needsUpdate=false;material.needsUpdate=JSValue(1);
    REQUIRE(material.version==0);
    material.needsUpdate=true;material.needsUpdate=JSValue(true);
    REQUIRE(material.version==2);
    material.version=9007199254740992.0;material.needsUpdate=true;
    REQUIRE(material.version==9007199254740992.0);
    material.setValues(JsonValue::Object{{"alphaTest",.4},{"version",5},{"needsUpdate",true}});
    REQUIRE(material.version==5);
    material.setValues(JsonValue::Object{{"version",5},{"alphaTest",0},{"needsUpdate",false}});
    REQUIRE(material.version==6);
}
TEST_CASE("Physical material feature setters preserve transition counts and reflectivity", "[materials]") {
    MeshPhysicalMaterial material;
    for(auto* feature:{&material.anisotropy,&material.clearcoat,&material.diffuseRoughness,&material.dispersion,&material.iridescence,&material.retroreflectivity,&material.sheen,&material.transmission}) {
        const auto before=material.version;
        *feature=.5;*feature=1;*feature=0;*feature=-1;*feature=std::numeric_limits<double>::quiet_NaN();*feature=.2;
        REQUIRE(material.version==before+3);
        REQUIRE(double(*feature)==.2);
    }
    material.reflectivity=.8;
    REQUIRE(material.ior==Approx((1+.4*.8)/(1-.4*.8)));
    REQUIRE(double(material.reflectivity)==Approx(.8));
    const auto version=material.version;material.ior=100;REQUIRE(double(material.reflectivity)==1);
    REQUIRE(material.version==version);
    material.iridescenceThicknessRange=JSValue::array(3);material.iridescenceThicknessRange.set("2",700);
    const auto previous=material.iridescenceThicknessRange;
    material.copy(material);
    REQUIRE_FALSE(previous.sameValue(material.iridescenceThicknessRange));
    REQUIRE(material.iridescenceThicknessRange.hasOwn("0"));
    REQUIRE(material.iridescenceThicknessRange.get("0").isUndefined());
    REQUIRE(material.iridescenceThicknessRange.get("2").number()==700);
}
TEST_CASE("Material copy clones planes and user data with upstream partial failure ordering", "[materials]") {
    MeshBasicMaterial source,target;
    source.name="source";source.alphaTest=.3;
    auto plane=std::make_shared<Plane>();plane->constant=4;
    source.clippingPlanes=std::vector{plane};
    source.userData=JsonValue::Object{{"nested",JsonValue::Object{{"a",1}}}};
    target.copy(source);
    REQUIRE(target.version==1);
    REQUIRE((*target.clippingPlanes)[0]!=plane);
    REQUIRE((*target.clippingPlanes)[0]->constant==4);
    target.userData["nested"]["a"]=2;
    REQUIRE(source.userData.at("nested").at("a").get<double>()==1);
    auto old=(*target.clippingPlanes)[0];source.clippingPlanes=std::vector<std::shared_ptr<Plane>>{nullptr};source.name="partial";
    REQUIRE_THROWS(target.copy(source));
    REQUIRE(target.name=="partial");
    REQUIRE((*target.clippingPlanes)[0]==old);
    target.copy(target);
    REQUIRE((*target.clippingPlanes)[0]!=old);
}
TEST_CASE("Material deserialization creates undeclared properties without duplicating native fields", "[materials]") {
    Material material;
    auto texture=std::make_shared<Texture>();texture->uuid="texture";
    material.fromJSON(JsonValue::Object{{"roughness",.2},{"sheenColor",0x112233},{"normalScale",.3},{"map","texture"},{"normalMap","absent"}},{{"texture",texture}});
    REQUIRE(material.extraProperties.get("roughness").number()==.2);
    REQUIRE(material.extraProperties.get("sheenColor").isHostObject("three.Color"));
    REQUIRE(material.extraProperties.get("normalScale").get("x").number()==.3);
    REQUIRE(material.extraProperties.get("map").sameValue(nativeTexture(texture)));
    REQUIRE(material.extraProperties.get("normalMap").isNull());
    const auto json=material.toJSON();
    REQUIRE(json.at("roughness").get<double>()==.2);
    REQUIRE(json.at("map").get<std::string>()=="texture");
    REQUIRE(json.at("sheenColor").get<double>()==0x112233);
    material.setValues(JsonValue::Object{{"roughness",.7}});
    REQUIRE(material.extraProperties.get("roughness").number()==.7);
    REQUIRE(material.clone()->extraProperties.keys().empty());
}
TEST_CASE("Shader uniforms clone Three values and preserve ordinary object identity", "[materials]") {
    ShaderMaterial source;
    auto object=JSValue::object();object.set("child",3);
    auto sparse=JSValue::array(3);sparse.set("1",object);
    auto first=customClone(4),second=customClone(5),typed=JSValue::array();typed.set("0",first);typed.set("1",second);
    source.uniforms.set("object",uniform(object));source.uniforms.set("sparse",uniform(sparse));source.uniforms.set("typed",uniform(typed));
    auto laterTyped=JSValue::array();laterTyped.set("0",2);laterTyped.set("1",first);source.uniforms.set("laterTyped",uniform(laterTyped));
    source.linewidth=8;source.defines.set("VALUE",object);
    const auto clone=std::dynamic_pointer_cast<ShaderMaterial>(source.clone());
    REQUIRE(clone->linewidth==1);
    REQUIRE(clone->uniforms.get("object").get("value").sameValue(object));
    const auto copiedSparse=clone->uniforms.get("sparse").get("value");
    REQUIRE_FALSE(copiedSparse.sameValue(sparse));
    REQUIRE_FALSE(copiedSparse.hasOwn("0"));
    REQUIRE(copiedSparse.get("1").sameValue(object));
    REQUIRE(clone->uniforms.get("typed").get("value").get("0").get("copied").number()==4);
    REQUIRE(clone->uniforms.get("typed").get("value").get("1").get("copied").number()==5);
    REQUIRE(clone->uniforms.get("laterTyped").get("value").get("1").sameValue(first));
    REQUIRE_FALSE(clone->defines.sameValue(source.defines));
    REQUIRE(clone->defines.get("VALUE").sameValue(object));
    REQUIRE(clone->defaultAttributeValues.get("color").sameValue(source.defaultAttributeValues.get("color")));
}
TEST_CASE("Shader copy preserves partial state when uniform clone throws", "[materials]") {
    ShaderMaterial source,target;
    source.name="partial";source.vertexShader="vertex";source.fragmentShader="fragment";
    auto broken=JSValue::object();broken.set("isVector2",true);source.uniforms.set("broken",uniform(broken));
    const auto original=target.uniforms;
    REQUIRE_THROWS_AS(target.copy(source),JSTypeError);
    REQUIRE(target.name=="partial");
    REQUIRE(target.vertexShader=="vertex");
    REQUIRE(target.fragmentShader=="fragment");
    REQUIRE(target.uniforms.sameValue(original));
    auto renderTarget=JSValue::object();renderTarget.set("isTexture",true);renderTarget.set("isRenderTargetTexture",true);
    source.uniforms.set("broken",uniform(renderTarget));target.copy(source);
    REQUIRE(target.uniforms.get("broken").get("value").isNull());
}
TEST_CASE("Material registered fields reference native storage for all new types", "[materials]") {
    MeshPhongMaterial phong;MeshPhysicalMaterial physical;ShaderMaterial shader;
    REQUIRE(std::get<Color*>(*phong.property("specular"))==&phong.specular);
    REQUIRE(std::get<Vector2*>(*phong.property("normalScale"))==&phong.normalScale);
    REQUIRE(std::get<std::shared_ptr<Texture>*>(*physical.property("clearcoatMap"))==&physical.clearcoatMap);
    REQUIRE(std::get<Material::AlphaTest*>(*physical.property("transmission"))==&physical.transmission);
    REQUIRE(std::get<JSValue*>(*shader.property("uniforms"))==&shader.uniforms);
    REQUIRE(std::get<double*>(*shader.property("version"))==&shader.version);
    REQUIRE_FALSE(phong.property("clearcoat"));
}
TEST_CASE("Shader material runtime ownership collects uniform and native owner cycles", "[materials]") {
    auto anchor=JSValue::object();std::weak_ptr<ShaderMaterial> weak;
    {
        auto material=std::make_shared<ShaderMaterial>();weak=material;auto wrapper=nativeMaterial(material);
        material->uniforms.set("self",uniform(wrapper));
        wrapper.set("custom",wrapper);
        anchor.collectGarbage();
        REQUIRE_FALSE(weak.expired());
        REQUIRE(material->uniforms.get("self").get("value").sameValue(wrapper));
    }
    anchor.collectGarbage();
    REQUIRE(weak.expired());
}
TEST_CASE("Material runtime descriptors and dynamic properties share authoritative storage", "[materials]") {
    auto material=std::make_shared<MeshPhysicalMaterial>();auto wrapper=nativeMaterial(material);
    const auto uniforms=JSValue::object();wrapper.set("custom",uniforms);
    REQUIRE(material->extraProperties.get("custom").sameValue(uniforms));
    material->extraProperties.set("custom",7);REQUIRE(wrapper.get("custom").number()==7);
    REQUIRE(wrapper.erase("custom"));REQUIRE_FALSE(material->extraProperties.hasOwn("custom"));
    wrapper.set("clearcoat",.4);REQUIRE(material->version==1);
    wrapper.set("reflectivity",.8);REQUIRE(double(material->reflectivity)==Approx(.8));
    REQUIRE_FALSE(wrapper.hasOwn("clearcoat"));REQUIRE(wrapper.hasOwn("_clearcoat"));
    REQUIRE(wrapper.getOwnPropertyDescriptor("opacity")->value->number()==1);
    REQUIRE_FALSE(wrapper.getOwnPropertyDescriptor("reflectivity")->enumerable.value());
    auto parameters=JSValue::object();parameters.set("color",0x123456);parameters.set("needsUpdate",true);
    const auto color=wrapper.get("color");wrapper.get("setValues").call(wrapper,{parameters});
    REQUIRE(material->color.getHex()==0x123456);
    REQUIRE(wrapper.get("color").sameValue(color));
    REQUIRE(material->version==1);
}

TEST_CASE("Native light ownership collects shadow target image cycles and retains external targets", "[materials][native-ownership]") {
    auto anchor=JSValue::object();
    for(int mode=0;mode<4;++mode) {
        std::weak_ptr<SpotLight> weakLight;std::weak_ptr<LightShadow> weakShadow;
        std::weak_ptr<RenderTarget> weakTarget;std::weak_ptr<Texture> weakTexture;
        std::weak_ptr<TextureImage> weakImage;std::shared_ptr<RenderTarget> outside;
        {
            auto light=std::make_shared<SpotLight>();weakLight=light;weakShadow=light->shadow;
            auto target=std::make_shared<RenderTarget>(2,2);weakTarget=target;light->shadow->map=target;
            light->shadow->mapPass=std::make_shared<RenderTarget>(3,3);
            light->shadow->mapPass->textures=target->textures;
            auto texture=target->texture.get();weakTexture=texture;light->map=texture;target->_depthTexture=texture;
            auto image=std::make_shared<TextureImage>();weakImage=image;texture->image=image;
            auto wrapper=nativeObject(light);auto data=JSValue::object();data.set("light",wrapper);image->data=data;
            light->shadow->biasNode=wrapper;
            if(mode==1)outside=target;
            if(mode==2){auto textureWrapper=nativeTexture(texture);textureWrapper.set("back",wrapper);}
            if(mode==3){auto vector=nativeVector(std::shared_ptr<Vector2>(light->shadow,&light->shadow->mapSize));vector.set("back",wrapper);}
            anchor.collectGarbage();REQUIRE_FALSE(weakLight.expired());
        }
        anchor.collectGarbage();
        if(mode==1){REQUIRE_FALSE(weakLight.expired());outside.reset();anchor.collectGarbage();}
        REQUIRE(weakLight.expired());REQUIRE(weakShadow.expired());REQUIRE(weakTarget.expired());
        REQUIRE(weakTexture.expired());REQUIRE(weakImage.expired());
    }
}

TEST_CASE("Native skeleton ownership collects bone array inverse and texture cycles", "[materials][native-ownership]") {
    auto anchor=JSValue::object();
    for(int mode=0;mode<5;++mode) {
        std::weak_ptr<SkinnedMesh> weakMesh;std::weak_ptr<Skeleton> weakSkeleton;
        std::weak_ptr<Bone> weakBone;std::shared_ptr<Skeleton> outside;
        {
            auto mesh=std::make_shared<SkinnedMesh>();weakMesh=mesh;auto bone=std::make_shared<Bone>();weakBone=bone;
            mesh->skeleton=std::make_shared<Skeleton>(Skeleton::Bones{bone});weakSkeleton=mesh->skeleton;
            auto wrapper=nativeObject(mesh);
            if(mode==0){auto boneWrapper=nativeObject(bone);boneWrapper.set("back",wrapper);}
            if(mode==1){auto array=wrapper.get("skeleton").get("bones");array.set("back",wrapper);}
            if(mode==2){auto matrix=nativeMatrix(mesh->skeleton->boneInverses[0]);matrix.set("back",wrapper);}
            if(mode==3){mesh->skeleton->computeBoneTexture();auto image=std::make_shared<TextureImage>();image->data=wrapper;mesh->skeleton->boneTexture->image=image;}
            if(mode==4){outside=mesh->skeleton;auto boneWrapper=nativeObject(bone);boneWrapper.set("back",wrapper);}
            anchor.collectGarbage();REQUIRE_FALSE(weakMesh.expired());
        }
        anchor.collectGarbage();
        if(mode==4){REQUIRE_FALSE(weakMesh.expired());outside.reset();anchor.collectGarbage();}
        REQUIRE(weakMesh.expired());REQUIRE(weakSkeleton.expired());REQUIRE(weakBone.expired());
    }
}

TEST_CASE("Native ownership preserves pure C++ shared pointer cycles", "[materials][native-ownership]") {
    auto anchor=JSValue::object();std::weak_ptr<SkinnedMesh> weakMesh;std::weak_ptr<Bone> weakBone;
    {
        auto mesh=std::make_shared<SkinnedMesh>();auto bone=std::make_shared<Bone>();weakMesh=mesh;weakBone=bone;
        mesh->children.push_back(bone);bone->children.push_back(mesh);
        auto wrapper=nativeObject(mesh);wrapper.set("loop",wrapper);
    }
    anchor.collectGarbage();REQUIRE_FALSE(weakMesh.expired());REQUIRE_FALSE(weakBone.expired());
    {auto bone=weakBone.lock();bone->children.clear();}
    anchor.collectGarbage();REQUIRE(weakMesh.expired());REQUIRE(weakBone.expired());
}

TEST_CASE("Native scene ownership counts fog color aliases as internal references", "[materials][native-ownership]") {
    auto anchor=JSValue::object();std::weak_ptr<Scene> weakScene;std::weak_ptr<FogBase> weakFog;
    {
        auto scene=std::make_shared<Scene>();weakScene=scene;
        auto fog=std::make_shared<Fog>(0xffffff);scene->fog=fog;weakFog=fog;
        auto wrapper=nativeObject(scene);auto color=nativeColor(std::shared_ptr<Color>(fog,&fog->color));
        color.set("back",wrapper);
    }
    anchor.collectGarbage();REQUIRE(weakScene.expired());REQUIRE(weakFog.expired());
}

TEST_CASE("Native skeleton retains bone matrix wrapper properties while its owner is live", "[materials][native-ownership]") {
    auto anchor=JSValue::object();auto mesh=std::make_shared<SkinnedMesh>();
    mesh->skeleton=std::make_shared<Skeleton>(Skeleton::Bones{std::make_shared<Bone>()});
    {
        auto wrapper=nativeObject(mesh);auto matrices=wrapper.get("skeleton").get("boneMatrices");
        matrices.set("custom",123);
    }
    anchor.collectGarbage();
    const auto matrices=nativeObject(mesh).get("skeleton").get("boneMatrices");
    REQUIRE(matrices.get("custom").strictEquals(JSValue(123)));
}

TEST_CASE("Native geometry owns shared typed views and releases replaced array wrappers", "[materials][native-ownership]") {
    auto anchor=JSValue::object();auto mesh=std::make_shared<Mesh>();std::weak_ptr<Mesh> weakMesh=mesh;
    std::weak_ptr<void> weakView,weakBuffer;
    {
        auto attribute=std::make_shared<BufferAttribute>(Float32Array{1,2,3,4},1);
        mesh->geometry->setAttribute("position",attribute);
        mesh->geometry->setAttribute("alias",std::make_shared<BufferAttribute>(std::get<Float32Array>(attribute->array),1));
        mesh->geometry->setAttribute("subarray",std::make_shared<BufferAttribute>(std::get<Float32Array>(attribute->array).subarray(1),1));
        const auto owner=nativeObject(mesh);
        for(const auto& [name,value]:mesh->geometry->attributes)std::visit([&](const auto& array){linkTypedArray(array,owner);},std::dynamic_pointer_cast<BufferAttribute>(value)->array);
        auto wrapper=nativeArray(std::get<Float32Array>(attribute->array));
        weakView=wrapper.hostPayload("three.AnimationArray");weakBuffer=wrapper.get("buffer").hostPayload("three.ArrayBuffer");
    }
    anchor.collectGarbage();
    for(const auto& [name,value]:mesh->geometry->attributes)std::visit([](const auto& array){checkTypedArray(array);},std::dynamic_pointer_cast<BufferAttribute>(value)->array);
    mesh->geometry->deleteAttribute("alias");mesh->geometry->deleteAttribute("subarray");
    std::dynamic_pointer_cast<BufferAttribute>(mesh->geometry->getAttribute("position"))->array=Float32Array{8,9};
    anchor.collectGarbage();REQUIRE(weakView.expired());REQUIRE(weakBuffer.expired());REQUIRE_FALSE(weakMesh.expired());
    {
        auto owner=nativeObject(mesh);
        std::visit([&](const auto& array){linkTypedArray(array,owner);},std::dynamic_pointer_cast<BufferAttribute>(mesh->geometry->getAttribute("position"))->array);
    }
    mesh.reset();anchor.collectGarbage();REQUIRE(weakMesh.expired());
}

TEST_CASE("Native skeleton and bone texture share typed storage without retaining JS cycles", "[materials][native-ownership]") {
    auto anchor=JSValue::object();auto mesh=std::make_shared<SkinnedMesh>();std::weak_ptr<SkinnedMesh> weakMesh=mesh;
    mesh->skeleton=std::make_shared<Skeleton>(Skeleton::Bones{std::make_shared<Bone>()});
    mesh->skeleton->computeBoneTexture();std::weak_ptr<Skeleton> weakSkeleton=mesh->skeleton;
    std::weak_ptr<Texture> weakTexture=mesh->skeleton->boneTexture;std::weak_ptr<void> weakView;
    {
        const auto owner=nativeObject(mesh);linkTypedArray(mesh->skeleton->boneMatrices,owner);
        weakView=nativeArray(mesh->skeleton->boneMatrices).hostPayload("three.AnimationArray");
        REQUIRE(std::get<Float32Array>(mesh->skeleton->boneTexture->image->data).sameIdentity(mesh->skeleton->boneMatrices));
    }
    anchor.collectGarbage();checkTypedArray(mesh->skeleton->boneMatrices);
    mesh.reset();anchor.collectGarbage();
    REQUIRE(weakMesh.expired());REQUIRE(weakSkeleton.expired());REQUIRE(weakTexture.expired());REQUIRE(weakView.expired());
}

TEST_CASE("Native texture image ownership covers every typed array and external buffers", "[materials][native-ownership]") {
    auto anchor=JSValue::object();
    for(int kind=0;kind<9;++kind) {
        auto texture=std::make_shared<Texture>();std::weak_ptr<Texture> weakTexture=texture;
        texture->image=std::make_shared<TextureImage>();std::weak_ptr<TextureImage> weakImage=texture->image.get();
        switch(kind){case 0:texture->image->data=Int8Array{1,2};break;case 1:texture->image->data=Uint8Array{1,2};break;
            case 2:texture->image->data=Int16Array{1,2};break;case 3:texture->image->data=Uint16Array{1,2};break;
            case 4:texture->image->data=Int32Array{1,2};break;case 5:texture->image->data=Uint32Array{1,2};break;
            case 6:texture->image->data=Float32Array{1,2};break;case 7:texture->image->data=Float64Array{1,2};break;
            default:texture->image->data=Uint8ClampedArray{1,2};break;}
        std::optional<ArrayBuffer> outside;
        {
            const auto owner=nativeTexture(texture);
            std::visit([&](const auto& array){if constexpr(!std::is_same_v<std::decay_t<decltype(array)>,std::nullptr_t>){
                linkTypedArray(array,owner);outside=arrayBuffer(nativeArray(array).get("buffer"));
            }},texture->image->data);
        }
        anchor.collectGarbage();
        std::visit([](const auto& array){if constexpr(!std::is_same_v<std::decay_t<decltype(array)>,std::nullptr_t>)checkTypedArray(array);},texture->image->data);
        texture.reset();anchor.collectGarbage();REQUIRE_FALSE(weakTexture.expired());
        outside.reset();anchor.collectGarbage();REQUIRE(weakTexture.expired());REQUIRE(weakImage.expired());
    }
}

TEST_CASE("Native object animation clips retain track and tangent typed array wrappers", "[materials][native-ownership]") {
    auto anchor=JSValue::object();auto object=std::make_shared<Object3D>();std::weak_ptr<Object3D> weakObject=object;
    std::weak_ptr<AnimationClip> weakClip;std::weak_ptr<KeyframeTrack> weakTrack;
    {
        auto track=std::make_shared<KeyframeTrack>(".value",Float32Array{0,1},Float32Array{4,5});weakTrack=track;
        track->settings=AnimationSettings(track->values,Float64Array{6,7});
        auto clip=std::make_shared<AnimationClip>("clip",1,std::make_shared<AnimationClip::Tracks>(AnimationClip::Tracks{track}));weakClip=clip;
        object->animations.push_back(clip);
        const auto owner=nativeObject(object);
        linkTypedArray(track->times,owner);linkTypedArray(track->values,owner);
        linkTypedArray(track->settings.get("inTangents"),owner);linkTypedArray(track->settings.get("outTangents"),owner);
    }
    anchor.collectGarbage();
    {
        const auto& track=object->animations[0]->tracks->at(0);
        checkTypedArray(track->times);checkTypedArray(track->values);
        checkTypedArray(track->settings.get("inTangents"));checkTypedArray(track->settings.get("outTangents"));
    }
    object.reset();anchor.collectGarbage();REQUIRE(weakObject.expired());REQUIRE(weakClip.expired());REQUIRE(weakTrack.expired());
}

TEST_CASE("Native interpolants count shared typed arrays and Bezier tangent owners", "[materials][native-ownership]") {
    auto anchor=JSValue::object();auto interpolant=std::make_shared<BezierInterpolant>(Float32Array{0,1},Float32Array{2,3},1);
    std::weak_ptr<BezierInterpolant> weak=interpolant;
    interpolant->inTangents=interpolant->sampleValues;interpolant->outTangents=Float64Array{4,5};
    {
        const auto owner=nativeInterpolant(interpolant);
        for(const auto* array:{&interpolant->parameterPositions,&interpolant->sampleValues,&interpolant->resultBuffer,&interpolant->inTangents,&interpolant->outTangents})linkTypedArray(*array,owner);
    }
    anchor.collectGarbage();
    for(const auto* array:{&interpolant->parameterPositions,&interpolant->sampleValues,&interpolant->resultBuffer,&interpolant->inTangents,&interpolant->outTangents})checkTypedArray(*array);
    interpolant.reset();anchor.collectGarbage();REQUIRE(weak.expired());
}

TEST_CASE("Native property mixer buffer wrappers follow their native owner lifetime", "[materials][native-ownership]") {
    auto anchor=JSValue::object();auto mixer=std::make_shared<PropertyMixer>(nullptr,"number",1);std::weak_ptr<PropertyMixer> weak=mixer;
    {const auto owner=nativePropertyMixer(mixer);linkTypedArray(mixer->buffer,owner);}
    anchor.collectGarbage();checkTypedArray(mixer->buffer);
    mixer.reset();anchor.collectGarbage();REQUIRE(weak.expired());
}
