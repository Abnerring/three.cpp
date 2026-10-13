#include "Three.h"
#include "math/Plane.h"
#include <iostream>
#include <stdexcept>

namespace {
unsigned checks=0;
void require(bool value,const char* message) {++checks;if(!value)throw std::runtime_error(message);}
JSValue planes(std::initializer_list<std::shared_ptr<Plane>> values) {
    auto result=JSValue::array();unsigned i=0;
    for(const auto& value:values)result.set(std::to_string(i++),nativePlane(value));
    return result;
}
void pixel(VulkanRenderer& renderer,unsigned x,unsigned y,bool visible,const char* message) {
    const auto image=renderer.readPixels();const auto i=(y*renderer.getWidth()+x)*4;
    require(image[i]==(visible?255:0)&&image[i+1]==(visible?255:0)&&image[i+2]==(visible?255:0),message);
}
}
int main() {
try {
    VulkanRenderer renderer({64,64,false});renderer.srgbOutput=false;
    OrthographicCamera camera(-1,1,1,-1,.1,20);camera.position.z=5;
    Scene scene;auto material=std::make_shared<MeshBasicMaterial>(Color(1,1,1));
    auto mesh=std::make_shared<Mesh>(std::make_shared<PlaneGeometry>(4,4),material);
    mesh->frustumCulled=false;scene.add(mesh);
    auto x=std::make_shared<Plane>(Vector3(1,0,0),0),y=std::make_shared<Plane>(Vector3(0,1,0),0);
    require(renderer.clippingPlanes.get("length").strictEquals(0)&&renderer.localClippingEnabled.strictEquals(false),"Renderer clipping defaults differ");
    material->clippingPlanes.emplace();material->clippingPlanes->push_back(x);
    renderer.render(scene,camera);pixel(renderer,16,32,true,"Disabled local clipping must ignore material planes");
    renderer.clippingPlanes=planes({x});renderer.render(scene,camera);
    pixel(renderer,16,32,false,"Global plane failed to clip negative half-space");
    pixel(renderer,48,32,true,"Global plane discarded positive half-space");
    renderer.clippingPlanes=planes({});renderer.render(scene,camera);
    pixel(renderer,16,32,true,"Removing globals did not reset previous clipping state");
    renderer.localClippingEnabled=true;renderer.render(scene,camera);
    pixel(renderer,16,32,false,"Local clipping did not consume native material plane array");
    pixel(renderer,48,32,true,"Local clipping discarded positive half-space");
    material->clippingPlanes->push_back(y);renderer.render(scene,camera);
    pixel(renderer,48,16,true,"Union clipped surviving quadrant");
    pixel(renderer,48,48,false,"Union failed second plane");
    pixel(renderer,16,16,false,"Union failed first plane");
    material->clipIntersection=true;renderer.render(scene,camera);
    pixel(renderer,16,48,false,"Intersection failed to clip common negative region");
    pixel(renderer,16,16,true,"Intersection incorrectly applied union to left upper region");
    pixel(renderer,48,48,true,"Intersection incorrectly applied union to right lower region");
    renderer.clippingPlanes=planes({x});renderer.render(scene,camera);
    pixel(renderer,16,16,false,"Global planes must stay union when material enables intersection");
    pixel(renderer,48,48,true,"Global/local intersection mixture discarded valid region");
    material->clipIntersection=false;material->clippingPlanes->clear();renderer.localClippingEnabled=false;
    camera.rotation.z=MathUtils::PI/2;renderer.render(scene,camera);
    pixel(renderer,32,16,false,"Camera rotation was omitted from plane projection");
    pixel(renderer,32,48,true,"Rotated view plane has incorrect orientation");
    camera.rotation.z=0;camera.position.x=.4;camera.scale.set(2,.5,1);renderer.render(scene,camera);
    pixel(renderer,16,32,false,"Camera scale/translation projected clipping plane incorrectly");
    pixel(renderer,32,32,true,"Translated camera failed world-space clipping");
    camera.position.x=0;camera.scale.set(1,1,1);mesh->position.x=.4;renderer.render(scene,camera);
    pixel(renderer,16,32,false,"Object transform incorrectly changed world clipping plane");
    pixel(renderer,48,32,true,"Object transform clipped positive world half-space");
    mesh->position.x=0;renderer.clippingPlanes=planes({});renderer.render(scene,camera);
    pixel(renderer,16,32,true,"Disabling clipping left stale GPU planes");
    require(renderer.getInfo().calls==1&&renderer.getInfo().triangles==2,"Clipping changed draw statistics");
    {
        VulkanRenderer cached({64,64,false});cached.srgbOutput=false;cached.localClippingEnabled=true;
        Scene pair;auto shared=std::make_shared<MeshBasicMaterial>(Color(1,1,1));
        auto localPlane=std::make_shared<Plane>(Vector3(0,1,0),0);
        shared->clippingPlanes.emplace();shared->clippingPlanes->push_back(localPlane);
        auto a=std::make_shared<Mesh>(std::make_shared<PlaneGeometry>(.8,.8),shared);
        auto b=std::make_shared<Mesh>(a->geometry,shared);
        a->frustumCulled=b->frustumCulled=false;a->position.x=-.5;b->position.x=.5;
        a->renderOrder=0;b->renderOrder=1;pair.add(a);pair.add(b);
        bool changed=false,dispose=false;
        a->onAfterRender=[&](auto&,auto&,auto&,auto*,auto*,auto*) {
            if(!changed){changed=true;localPlane->normal.y=-1;if(dispose)shared->dispose();}
        };
        cached.render(pair,camera);
        pixel(cached,16,24,true,"First material draw lost positive local half-space");
        pixel(cached,48,24,true,"Same-material cache reprojected a modified plane inside one frame");
        pixel(cached,48,40,false,"Same-material cache read later plane mutation");
        cached.render(pair,camera);
        pixel(cached,16,40,true,"Next frame failed to refresh cached plane");
        pixel(cached,48,24,false,"Next frame retained obsolete clipping projection");
        auto second=std::make_shared<MeshBasicMaterial>(Color(1,1,1));second->clippingPlanes=shared->clippingPlanes;
        b->material=second;localPlane->normal.y=1;changed=false;cached.render(pair,camera);
        pixel(cached,16,24,true,"Material switch case changed first draw snapshot");
        pixel(cached,48,40,true,"Different material failed to reproject shared native plane");
        pixel(cached,48,24,false,"Different material reused another material's clipping cache");
        b->material=shared;localPlane->normal.y=1;changed=false;dispose=true;cached.render(pair,camera);
        pixel(cached,16,24,true,"Material disposal damaged already submitted clipping storage");
        pixel(cached,48,40,true,"Disposed material retained its old projected clipping state");
    }
    {
        VulkanRenderer uploaded({64,64,false});uploaded.srgbOutput=false;uploaded.localClippingEnabled=true;
        Scene target;auto m=std::make_shared<MeshBasicMaterial>(Color(1,1,1));
        auto p=std::make_shared<Plane>(Vector3(1,0,0),0);m->clippingPlanes.emplace();m->clippingPlanes->push_back(p);
        auto g=std::make_shared<PlaneGeometry>(4,4);unsigned calls=0;
        g->index->onUpload([&]{++calls;p->normal.x=-1;});
        auto object=std::make_shared<Mesh>(g,m);object->frustumCulled=false;target.add(object);
        uploaded.render(target,camera);
        pixel(uploaded,16,32,false,"Index upload callback changed already projected clipping planes");
        pixel(uploaded,48,32,true,"Index upload callback damaged current draw plane snapshot");
        uploaded.render(target,camera);
        pixel(uploaded,16,32,true,"Next frame ignored plane changed by index upload callback");
        pixel(uploaded,48,32,false,"Next frame kept old upload-time clipping projection");
        require(calls==1,"Clipping refresh reuploaded an unchanged index");
    }
    {
        auto state=JSValue::object();state.set("escaped",nullptr);state.set("fail",false);state.set("translate",true);
        auto prototype=nativePlanePrototype();const auto original=prototype.get("applyMatrix4");
        struct Restore {JSValue object,method;~Restore(){object.set("applyMatrix4",method);}} restore{prototype,original};
        prototype.set("applyMatrix4",JSValue::function([](const JSValue& self,const auto& args,const auto& captures)->JSValue {
            auto state=captures[0];state.set("escaped",args[0]);
            if(state.get("translate").toBoolean())args[0].get("elements").set("12",.25);
            if(state.get("fail").toBoolean())throw std::runtime_error("clipping-projection-probe");
            return captures[1].call(self,args);
        },{state,original}));
        VulkanRenderer borrowed({64,64,false});borrowed.srgbOutput=false;borrowed.clippingPlanes=planes({x});
        OrthographicCamera localCamera(-1,1,1,-1,.1,20);localCamera.position.z=5;
        borrowed.render(scene,localCamera);
        require(localCamera.matrixWorldInverse.elements()[12]==.25,"Projection callback did not write through live borrowed camera matrix");
        pixel(borrowed,32,32,false,"Borrowed camera projection used a copied matrix");
        pixel(borrowed,48,32,true,"Borrowed camera mutation lost positive world half-space");
        bool expired=false;
        try{state.get("escaped").get("elements");}catch(const JSTypeError&){expired=true;}
        require(expired,"Successful render did not retire escaped stack-camera matrix view");
        state.set("translate",false);state.set("fail",true);bool failed=false;
        try{borrowed.render(scene,localCamera);}catch(const std::runtime_error& error){failed=std::string(error.what())=="clipping-projection-probe";}
        require(failed,"Projection exception did not propagate");expired=false;
        try{state.get("escaped").get("elements");}catch(const JSTypeError&){expired=true;}
        require(expired,"Exceptional render exit did not retire escaped stack-camera view");
        state.set("fail",false);
        auto owned=std::make_shared<OrthographicCamera>(-1,1,1,-1,.1,20);owned->position.z=5;
        borrowed.render(scene,*owned);
        require(state.get("escaped").sameValue(nativeObject(owned).get("matrixWorldInverse")),
                "Shared camera did not retain its real native matrix wrapper identity");
        require(state.get("escaped").get("elements").get("0").strictEquals(1),
                "Owned camera matrix incorrectly expired at render completion");
    }
    {
        VulkanRenderer instanced({64,64,false});instanced.srgbOutput=false;instanced.clippingPlanes=planes({x});
        Scene target;auto m=std::make_shared<MeshBasicMaterial>(Color(1,1,1));
        auto object=std::make_shared<InstancedMesh>(std::make_shared<PlaneGeometry>(.7,.7),m,2);
        object->setMatrixAt(0,Matrix4().makeTranslation(-.5,0,0));object->setMatrixAt(1,Matrix4().makeTranslation(.5,0,0));
        object->frustumCulled=false;target.add(object);instanced.render(target,camera);
        pixel(instanced,16,32,false,"Clipping omitted the negative instance transform");
        pixel(instanced,48,32,true,"Clipping discarded the positive instance transform");
        require(instanced.getInfo().calls==1&&instanced.getInfo().triangles==4,"Per-fragment clipping changed logical instance statistics");
    }
    {
        VulkanRenderer morphed({64,64,false});morphed.srgbOutput=false;morphed.clippingPlanes=planes({x});
        Scene target;auto m=std::make_shared<MeshBasicMaterial>(Color(1,1,1));
        auto g=std::make_shared<PlaneGeometry>(.4,.4);g->translate(-.5,0,0);
        auto p=std::dynamic_pointer_cast<BufferAttribute>(g->getAttribute("position"))->clone();
        for(double i=0;i<p->count;++i)p->setX(i,p->getX(i)+1);
        g->morphAttributes["position"]={p};auto object=std::make_shared<Mesh>(g,m);
        object->frustumCulled=false;object->morphTargetInfluences->at(0)=1;target.add(object);morphed.render(target,camera);
        pixel(morphed,48,32,true,"Clipping used original rather than morphed view position");
        object->morphTargetInfluences->at(0)=0;morphed.render(target,camera);
        pixel(morphed,16,32,false,"Unmorphed negative geometry was not clipped");
        pixel(morphed,48,32,false,"Morph reset left obsolete transformed geometry");
        target.clear();auto instances=std::make_shared<InstancedMesh>(g,m,2);instances->frustumCulled=false;
        Mesh weights(g,m);weights.morphTargetInfluences->at(0)=0;instances->setMorphAt(0,weights);
        weights.morphTargetInfluences->at(0)=1;instances->setMorphAt(1,weights);instances->morphTexture->needsUpdate=true;target.add(instances);
        morphed.render(target,camera);
        pixel(morphed,16,32,false,"Instanced morph clipping used pre-deformation positions");
        pixel(morphed,48,32,true,"Instanced morph's positive position was clipped");
    }
    {
        VulkanRenderer interrupted({64,64,false});interrupted.srgbOutput=false;interrupted.localClippingEnabled=true;
        Scene target;auto m=std::make_shared<MeshBasicMaterial>(Color(1,1,1));
        auto p=std::make_shared<Plane>(Vector3(1,0,0),0);m->clippingPlanes.emplace();m->clippingPlanes->push_back(p);
        auto object=std::make_shared<Mesh>(std::make_shared<PlaneGeometry>(4,4),m);object->frustumCulled=false;target.add(object);
        object->onAfterRender=[&](auto&,auto&,auto&,auto*,auto*,auto*){p->normal.x=-1;throw std::runtime_error("after-draw-probe");};
        bool failed=false;
        try{interrupted.render(target,camera);}catch(const std::runtime_error& error){failed=std::string(error.what())=="after-draw-probe";}
        require(failed,"Draw callback exception did not propagate");object->onAfterRender={};
        interrupted.render(target,camera);
        pixel(interrupted,16,32,false,"Exceptional exit unexpectedly reset current material/camera clipping cache");
        pixel(interrupted,48,32,true,"Exceptional exit lost the previous projected local plane");
        interrupted.render(target,camera);
        pixel(interrupted,16,32,true,"Successful recovery did not reset current clipping identity for next frame");
        pixel(interrupted,48,32,false,"Next successful frame retained stale clipping state");
    }
    std::cout<<checks<<" native Vulkan clipping checks passed on "<<renderer.getInfo().deviceName<<'\n';
} catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
}
