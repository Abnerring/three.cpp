#include "Three.h"
#include "math/Plane.h"
#include <iostream>

namespace {
JSValue planeArray(std::initializer_list<std::shared_ptr<Plane>> values) {
    auto result=JSValue::array();unsigned i=0;
    for(const auto& value:values)result.set(std::to_string(i++),nativePlane(value));
    return result;
}
JsonValue scenario(unsigned seed) {
    auto trace=JSValue::array();auto prototype=nativePlanePrototype();const auto original=prototype.get("applyMatrix4");
    struct Restore{JSValue p,m;~Restore(){p.set("applyMatrix4",m);}}restore{prototype,original};
    prototype.set("applyMatrix4",JSValue::function([](const JSValue& self,const auto& args,const auto& captures)->JSValue {
        const auto result=captures[0].call(self,args);auto normal=self.get("normal"),trace=captures[1];
        auto values=JSValue::fromJSON(JsonValue::Array{double(float(normal.get("x").toNumber())),
            double(float(normal.get("y").toNumber())),double(float(normal.get("z").toNumber())),
            double(float(self.get("constant").toNumber()))});
        trace.get("push").call(trace,{values});return result;
    },{original,trace}));
    VulkanRenderer renderer({64,64,false});renderer.srgbOutput=false;
    auto camera=std::make_shared<OrthographicCamera>(-1,1,1,-1,.1,20);camera->position.z=5;
    Scene scene;auto material=std::make_shared<MeshBasicMaterial>(Color(1,1,1));
    auto mesh=std::make_shared<Mesh>(std::make_shared<PlaneGeometry>(4,4),material);mesh->frustumCulled=false;scene.add(mesh);
    auto x=std::make_shared<Plane>(Vector3(1,0,0),0),y=std::make_shared<Plane>(Vector3(0,1,0),0);
    const auto local=[&](std::initializer_list<std::shared_ptr<Plane>> values,bool enabled=true){
        material->clippingPlanes.emplace();for(const auto& value:values)material->clippingPlanes->push_back(value);
        renderer.localClippingEnabled=enabled;
    };
    switch(seed) {
    case 1:renderer.clippingPlanes=planeArray({x});break;
    case 2:renderer.clippingPlanes=planeArray({y});break;
    case 3:renderer.clippingPlanes=planeArray({x,y});break;
    case 4:local({x},false);break;
    case 5:local({x});break;
    case 6:local({x,y});break;
    case 7:local({x,y});material->clipIntersection=true;break;
    case 8:renderer.clippingPlanes=planeArray({x});local({std::make_shared<Plane>(Vector3(-1,0,0),0),y});material->clipIntersection=true;break;
    case 9:renderer.clippingPlanes=planeArray({x});camera->position.x=.4;break;
    case 10:x->constant=.2;renderer.clippingPlanes=planeArray({x});camera->scale.set(2,.5,1);break;
    case 11:renderer.clippingPlanes=planeArray({x});camera->rotation.z=MathUtils::PI/2;break;
    case 12:renderer.clippingPlanes=planeArray({x});mesh->position.x=.4;break;
    case 13:x->constant=-.2;renderer.clippingPlanes=planeArray({x});break;
    case 14:local({x});material->transparent=true;material->side=2;break;
    case 15:local({x});renderer.localClippingEnabled="enabled";break;
    case 16:renderer.clippingPlanes=planeArray({x});local({y});renderer.localClippingEnabled="enabled";break;
    case 17:renderer.clippingPlanes=planeArray({x});renderer.render(scene,*camera);renderer.clippingPlanes=planeArray({});break;
    case 18:renderer.clippingPlanes=planeArray({std::make_shared<Plane>(Vector3(0,0,1),-.1)});break;
    case 19:renderer.clippingPlanes=planeArray({std::make_shared<Plane>(Vector3(0,0,1),.1)});break;
    case 20:renderer.clippingPlanes=planeArray({std::make_shared<Plane>(Vector3(0,0,1),-1)});mesh->position.z=2;break;
    case 21:x->constant=.3;renderer.clippingPlanes=planeArray({x});camera->rotation.z=.6;camera->scale.set(1.2,.8,1);break;
    default:break;
    }
    renderer.render(scene,*camera);const auto image=renderer.readPixels();JsonValue::Array pixels;
    for(const auto yPixel:{16u,32u,48u})for(const auto xPixel:{16u,32u,48u}) {
        const auto at=(yPixel*64+xPixel)*4;pixels.emplace_back(JsonValue::Array{image[at],image[at+1],image[at+2],image[at+3]});
    }
    return JsonValue::Object{{"seed",seed},{"projected",trace.toJSON().value()},{"pixels",pixels},
        {"calls",renderer.getInfo().calls},{"triangles",renderer.getInfo().triangles}};
}
}
int main(){try{for(unsigned seed=0;seed<22;++seed)std::cout<<scenario(seed).dump()<<'\n';}
catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
