#include "catch.hpp"
#include "objects/Line.h"
#include "objects/LineLoop.h"
#include "objects/LineSegments.h"
#include "objects/Points.h"
#include "objects/Sprite.h"
#include "core/InterleavedBufferAttribute.h"
#include "core/Raycaster.h"
#include "cameras/PerspectiveCamera.h"
#include "math/Frustum.h"
#include "animation/PropertyBinding.h"
#include "core/NativeObject.h"

namespace {
std::shared_ptr<BufferGeometry> vertices(std::initializer_list<double> positions) {
    auto geometry=std::make_shared<BufferGeometry>();geometry->setAttribute("position",std::make_shared<Float32BufferAttribute>(std::vector<double>(positions),3));return geometry;
}
struct ReentrantGeometry:BufferGeometry {
    mutable std::function<void()> callback;
    JsonValue toJSON() const override {if(callback)callback();return BufferGeometry::toJSON();}
};
struct ReentrantMaterial:LineBasicMaterial {
    mutable std::function<void()> callback;
    JsonValue toJSON(TextureJSONContext* meta=nullptr)const override{if(callback)callback();return LineBasicMaterial::toJSON(meta);}
};
}
TEST_CASE("Line distances keep upstream empty odd and indexed behavior", "[objects][primitive]") {
    Line empty(vertices({}));empty.computeLineDistances();REQUIRE(empty.geometry->getAttribute("lineDistance")->getCount()==1);
    REQUIRE(empty.geometry->getAttribute("lineDistance")->getX(0)==0);
    LineSegments segments(vertices({0,0,0,3,4,0,4,4,0}));segments.computeLineDistances();
    const auto distance=segments.geometry->getAttribute("lineDistance");
    REQUIRE(distance->getCount()==4);REQUIRE(distance->getX(1)==5);REQUIRE(distance->getX(2)==5);REQUIRE(std::isnan(distance->getX(3)));
    segments.geometry->setIndex(std::vector<std::uint32_t>{0,1});REQUIRE(&segments.computeLineDistances()==&segments);
    REQUIRE(segments.geometry->getAttribute("lineDistance")==distance);
}
TEST_CASE("Line and Points copies preserve destination morph state while clones start undefined", "[objects][primitive]") {
    auto geometry=vertices({0,0,0});auto morph=std::make_shared<Float32BufferAttribute>(std::vector<double>{1,0,0},3);morph->name="target";
    geometry->morphAttributes["position"]={morph};Line source(geometry),target(geometry);Points points(geometry),pointsTarget(geometry);
    (*source.morphTargetInfluences)[0]=.9;(*target.morphTargetInfluences)[0]=.2;
    (*points.morphTargetInfluences)[0]=.8;(*pointsTarget.morphTargetInfluences)[0]=.3;
    const auto dictionary=target.morphTargetDictionary;target.copy(source);pointsTarget.copy(points);
    REQUIRE((*target.morphTargetInfluences)[0]==.2);REQUIRE(target.morphTargetDictionary==dictionary);
    REQUIRE((*pointsTarget.morphTargetInfluences)[0]==.3);
    REQUIRE_FALSE(std::dynamic_pointer_cast<Line>(source.clone())->morphTargetInfluences);
    REQUIRE_FALSE(std::dynamic_pointer_cast<Points>(points.clone())->morphTargetInfluences);
    geometry->morphAttributes.clear();source.updateMorphTargets();REQUIRE((*source.morphTargetInfluences)[0]==.9);
}
TEST_CASE("Primitive morph dictionaries use JS property key order and proto setter semantics", "[objects][primitive]") {
    auto geometry=vertices({0,0,0});auto first=std::make_shared<Float32BufferAttribute>(std::vector<double>{0,0,0},3);first->name="late";
    auto selected=std::make_shared<Float32BufferAttribute>(std::vector<double>{0,0,0},3);selected->name="__proto__";
    geometry->morphAttributes["10"]={first};geometry->morphAttributes["2"]={selected,selected};
    Points points(geometry);REQUIRE(points.morphTargetInfluences->size()==2);REQUIRE(points.morphTargetDictionary->empty());
    selected->name="name";points.updateMorphTargets();REQUIRE(points.morphTargetDictionary->at("name")==1);
}
TEST_CASE("Line threshold includes equality while Points threshold is strict", "[objects][primitive]") {
    auto geometry=vertices({-1,1,0,1,1,0});Raycaster ray(Vector3(0,0,2),Vector3(0,0,-1));
    Line line(geometry);Points points(geometry);line.updateMatrixWorld();points.updateMatrixWorld();
    std::vector<Intersection> hits;line.raycast(ray,hits);REQUIRE(hits.size()==1);REQUIRE(hits[0].point.y==1);
    geometry->setAttribute("position",std::make_shared<Float32BufferAttribute>(std::vector<double>{0,1,0},3));geometry->boundingSphere.reset();
    hits.clear();points.raycast(ray,hits);REQUIRE(hits.empty());
    ray.params["Points"]["threshold"]=1.001;points.raycast(ray,hits);REQUIRE(hits.size()==1);
    REQUIRE(*hits[0].distanceToRay==1);REQUIRE(hits[0].point.y==0);
}
TEST_CASE("LineLoop closes only the active draw range and owns returned objects", "[objects][primitive]") {
    auto geometry=vertices({-4,-4,0,-1,-1,0,1,-1,0,0,1,0});geometry->setDrawRange(1,3);
    auto loop=std::make_shared<LineLoop>(geometry);loop->updateMatrixWorld();Raycaster ray(Vector3(-.5,0,2),Vector3(0,0,-1));ray.params["Line"]["threshold"]=.01;
    auto hits=ray.intersectObject(*loop);REQUIRE(hits.size()==1);REQUIRE(*hits[0].index==3);REQUIRE(hits[0].objectOwner==loop);
    std::weak_ptr<LineLoop> weak=loop;loop.reset();REQUIRE_FALSE(weak.expired());hits.clear();REQUIRE(weak.expired());
}
TEST_CASE("Sprite geometry shares one interleaved position uv buffer", "[objects][primitive]") {
    Sprite first,second;REQUIRE(first.geometry==second.geometry);
    auto position=std::dynamic_pointer_cast<InterleavedBufferAttribute>(first.geometry->getAttribute("position"));
    auto uv=std::dynamic_pointer_cast<InterleavedBufferAttribute>(first.geometry->getAttribute("uv"));
    REQUIRE(position);REQUIRE(uv);REQUIRE(position->getProperty("data").sameValue(uv->getProperty("data")));
    REQUIRE(position->getCount()==4);REQUIRE(position->getX(0)==-.5);REQUIRE(uv->getY(2)==1);
    first.geometry=vertices({1,2,3});first.center.set(.1,.9);first.count=9;
    const auto clone=std::dynamic_pointer_cast<Sprite>(first.clone());REQUIRE(clone->geometry==second.geometry);
    REQUIRE(clone->material==first.material);REQUIRE(double(clone->count)==1);REQUIRE(clone->center.equals(first.center));
    const auto json=first.toJSON();REQUIRE_FALSE(json.at("object").contains("center"));REQUIRE_FALSE(json.at("object").contains("count"));REQUIRE_FALSE(json.at("object").contains("geometry"));
}
TEST_CASE("Sprite camera facing raycast updates modelView and interpolation", "[objects][primitive]") {
    Sprite sprite;sprite.position.z=-4;sprite.updateMatrixWorld();PerspectiveCamera camera;camera.updateMatrixWorld();
    Raycaster ray(Vector3(.25,.25,0),Vector3(0,0,-1));ray.camera=&camera;std::vector<Intersection> hits;sprite.raycast(ray,hits);
    REQUIRE(hits.size()==1);REQUIRE(hits[0].distance==4);REQUIRE(hits[0].uv->x==Approx(.75));REQUIRE(hits[0].uv->y==Approx(.75));
    REQUIRE(sprite.modelViewMatrix.elements()[14]==-4);
    ray.camera=nullptr;REQUIRE_THROWS_AS(sprite.raycast(ray,hits),JSTypeError);
}
TEST_CASE("Primitive serialization reads material after geometry callback and current array entries", "[objects][primitive]") {
    auto geometry=std::make_shared<ReentrantGeometry>();geometry->uuid="geometry";
    auto first=std::make_shared<ReentrantMaterial>();first->uuid="first";auto second=std::make_shared<LineBasicMaterial>();second->uuid="second";
    Line line(geometry,first);line.uuid="line";
    geometry->callback=[&]{line.material=second;};
    auto json=line.toJSON();REQUIRE(json.at("object").at("material").get<std::string>()=="second");
    geometry->callback={};line.materials={first,first};line.materialIsArray=true;
    first->callback=[&]{line.materials[1]=second;};json=line.toJSON();
    REQUIRE(json.at("object").at("material")[0].get<std::string>()=="first");
    REQUIRE(json.at("object").at("material")[1].get<std::string>()=="second");
}
TEST_CASE("Primitive native fields preserve resource and morph identity through animation binding", "[objects][primitive][native]") {
    auto geometry=vertices({0,0,0});auto morph=std::make_shared<Float32BufferAttribute>(std::vector<double>{1,0,0},3);morph->name="smile";geometry->morphAttributes["position"]={morph};
    for(int type=0;type<4;++type) {
        std::shared_ptr<Object3D> object;
        if(type==0)object=std::make_shared<Line>(geometry);
        if(type==1)object=std::make_shared<LineLoop>(geometry);
        if(type==2)object=std::make_shared<LineSegments>(geometry);
        if(type==3)object=std::make_shared<Points>(geometry);
        auto wrapper=nativeObject(object);auto influences=wrapper.get("morphTargetInfluences");
        PropertyBinding opacity(wrapper,".material.opacity"),binding(wrapper,".morphTargetInfluences[smile]");
        opacity.setValue(AnimationArray::array({.25}),0);binding.setValue(AnimationArray::array({.75}),0);
        REQUIRE(wrapper.get("material").get("opacity").number()==.25);
        REQUIRE(wrapper.get("material").get("version").number()==1);
        REQUIRE(influences.get("0").number()==.75);REQUIRE(object->matrixWorldNeedsUpdate);
        auto other=std::make_shared<Points>(geometry);auto second=nativeObject(other);
        wrapper.set("geometry",second.get("geometry"));wrapper.set("morphTargetInfluences",second.get("morphTargetInfluences"));
        wrapper.set("morphTargetDictionary",second.get("morphTargetDictionary"));
        REQUIRE(wrapper.get("geometry").sameValue(second.get("geometry")));
        REQUIRE(wrapper.get("morphTargetInfluences").sameValue(second.get("morphTargetInfluences")));
        other->materials={other->material};other->material.reset();other->materialIsArray=true;
        wrapper.set("material",second.get("material"));REQUIRE(wrapper.get("material").sameValue(second.get("material")));
        wrapper.get("material").set("0",nativeMaterial(std::make_shared<LineBasicMaterial>()));
        REQUIRE(wrapper.get("material").get("0").sameValue(nativeMaterial(other->materials[0])));
    }
    auto sprite=std::make_shared<Sprite>();auto wrapper=nativeObject(sprite);auto center=wrapper.get("center");
    PropertyBinding binding(wrapper,".center"),rotation(wrapper,".material.rotation"),count(wrapper,".count");
    binding.setValue(AnimationArray::array({.1,.9}),0);rotation.setValue(AnimationArray::array({.5}),0);count.setValue(AnimationArray::array({3}),0);
    REQUIRE(center.sameValue(wrapper.get("center")));REQUIRE(sprite->center.equals(Vector2(.1,.9)));
    REQUIRE(std::dynamic_pointer_cast<SpriteMaterial>(sprite->material)->rotation==.5);REQUIRE(double(sprite->count)==3);
}
TEST_CASE("Primitive native ownership retains live resource extensions and collects released JS cycles", "[objects][primitive][native-ownership]") {
    auto anchor=JSValue::object();
    for(int type=0;type<5;++type)for(int mode=0;mode<5;++mode) {
        auto geometry=vertices({0,0,0});auto morph=std::make_shared<Float32BufferAttribute>(std::vector<double>{1,0,0},3);geometry->morphAttributes["position"]={morph};
        std::shared_ptr<Object3D> object;
        if(type==0)object=std::make_shared<Line>(geometry);
        if(type==1)object=std::make_shared<LineLoop>(geometry);
        if(type==2)object=std::make_shared<LineSegments>(geometry);
        if(type==3)object=std::make_shared<Points>(geometry);
        if(type==4){auto sprite=std::make_shared<Sprite>();sprite->geometry=geometry;object=sprite;}
        std::weak_ptr<Object3D> weak=object;std::weak_ptr<BufferGeometry> weakGeometry=geometry;
        const auto property=mode==0?"geometry":mode==1?"material":mode==2?(type==4?"center":"morphTargetInfluences"):mode==3?(type==4?"count":"morphTargetDictionary"):"material";
        {
            auto wrapper=nativeObject(object);
            if(mode==4&&type!=4){if(auto line=std::dynamic_pointer_cast<Line>(object)){line->materials={line->material};line->material.reset();line->materialIsArray=true;}else{auto points=std::dynamic_pointer_cast<Points>(object);points->materials={points->material};points->material.reset();points->materialIsArray=true;}}
            if(type==4&&mode==3)wrapper.set("count",JSValue::object());
            auto field=wrapper.get(property);
            if(type!=4&&mode==3)wrapper.set("back",wrapper);else field.set("back",wrapper);
            field.set("custom",123);
        }
        geometry.reset();morph.reset();anchor.collectGarbage();
        {auto wrapper=nativeObject(object);auto field=wrapper.get(property);REQUIRE(field.get("custom").number()==123);REQUIRE((type!=4&&mode==3?wrapper:field).get("back").sameValue(wrapper));}
        object.reset();anchor.collectGarbage();REQUIRE(weak.expired());REQUIRE(weakGeometry.expired());
    }
}
