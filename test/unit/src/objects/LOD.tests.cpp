#include "catch.hpp"
#include "objects/LOD.h"
#include "core/NativeObject.h"
#include "core/Raycaster.h"
#include "cameras/PerspectiveCamera.h"
#include "cameras/OrthographicCamera.h"

namespace {
std::shared_ptr<Object3D> node(const char* name) { auto object=std::make_shared<Object3D>();object->name=name;return object; }
std::string selected(const LOD& lod,JSValue distance) { const auto object=lod.getObjectForDistance(distance);return object.isNull()?"null":object.get("name").toString(); }
struct RayNode : Object3D {
    int calls=0;bool result=false;
    bool raycast(const Raycaster& ray,std::vector<Intersection>& hits) override {
        ++calls;Intersection hit;hit.object=this;hit.distance=ray.ray.origin.length();hits.push_back(hit);return result;
    }
};
}
TEST_CASE("LOD levels preserve insertion order distance normalization and child identity", "[objects][lod]") {
    LOD lod;auto high=node("high"),middle=node("middle"),low=node("low");auto levels=lod.levels;
    REQUIRE(lod.isLOD);REQUIRE(lod.type=="LOD");REQUIRE(lod.autoUpdate.toBoolean());REQUIRE(lod.getCurrentLevel().toNumber()==0);REQUIRE(lod.getObjectForDistance(4).isNull());
    REQUIRE(&lod.addLevel(low,-50,.1)==&lod);lod.addLevel(high,5);lod.addLevel(middle,25,.05);lod.addLevel(middle,25,.2);
    REQUIRE(levels.sameValue(lod.levels));REQUIRE(levels.length()==4);REQUIRE(levels.get("0").get("distance").toNumber()==5);REQUIRE(levels.get("3").get("distance").toNumber()==50);
    REQUIRE(levels.get("1").get("object").sameValue(levels.get("2").get("object")));REQUIRE(lod.children.size()==3);REQUIRE(lod.children.back()==middle);
    REQUIRE_FALSE(lod.removeLevel("25"));REQUIRE(lod.removeLevel(25));REQUIRE(levels.length()==3);REQUIRE(lod.children.size()==2);REQUIRE(middle->parent==nullptr);
    REQUIRE(levels.get("1").get("object").sameValue(nativeObject(middle)));REQUIRE_FALSE(lod.removeLevel(-50));
}
TEST_CASE("LOD hysteresis uses current visibility and camera zoom without updating matrices", "[objects][lod]") {
    LOD lod;auto high=node("high"),middle=node("middle"),low=node("low");lod.addLevel(high,0);lod.addLevel(middle,10,.2);lod.addLevel(low,20,.1);PerspectiveCamera camera;
    camera.matrixWorld.makeTranslation(0,0,8);lod.update(camera);REQUIRE(lod.getCurrentLevel().toNumber()==1);REQUIRE_FALSE(high->visible);REQUIRE(middle->visible);REQUIRE_FALSE(low->visible);
    camera.matrixWorld.makeTranslation(0,0,7.999);lod.update(camera);REQUIRE(lod.getCurrentLevel().toNumber()==0);REQUIRE(selected(lod,8)=="high");
    camera.matrixWorld.makeTranslation(0,0,20);camera.zoom=2;lod.update(camera);REQUIRE(lod.getCurrentLevel().toNumber()==1);REQUIRE(selected(lod,18)=="middle");
    camera.zoom=0;lod.update(camera);REQUIRE(lod.getCurrentLevel().toNumber()==2);
    Camera generic;generic.matrixWorld.makeTranslation(0,0,999);lod.update(generic);REQUIRE(lod.getCurrentLevel().toNumber()==0);REQUIRE(high->visible);REQUIRE_FALSE(middle->visible);REQUIRE_FALSE(low->visible);
    lod.autoUpdate=false;camera.zoom=1;lod.update(camera);REQUIRE(lod.getCurrentLevel().toNumber()==2);
}
TEST_CASE("LOD copy appends cloned levels preserves shared resources and ignores recursive", "[objects][lod]") {
    LOD source,destination;auto high=node("high");high->add(node("nested"));source.addLevel(high,5,.1);source.add(node("untracked"));source.autoUpdate=false;source._currentLevel=7;
    destination.addLevel(node("retained"),1);destination._currentLevel=3;destination.copy(source,false);
    REQUIRE(destination.levels.length()==2);REQUIRE(destination.children.size()==2);REQUIRE(destination.getCurrentLevel().toNumber()==3);REQUIRE_FALSE(destination.autoUpdate.toBoolean());
    const auto cloned=std::static_pointer_cast<Object3D>(destination.levels.get("1").get("object").hostPayload("three.Object3D"));REQUIRE(cloned!=high);REQUIRE(cloned->children.size()==1);
    auto fresh=std::dynamic_pointer_cast<LOD>(source.clone(false));REQUIRE(fresh->levels.length()==1);REQUIRE(fresh->children.size()==1);REQUIRE(fresh->getCurrentLevel().toNumber()==0);
    source.copy(source);REQUIRE(source.levels.length()==2);REQUIRE(source.children.size()==3);
    auto json=fresh->toJSON();REQUIRE_FALSE(json.at("object").at("autoUpdate").get<bool>());REQUIRE(json.at("object").at("levels").size()==1);REQUIRE(json.at("object").at("levels")[0].at("object").get<std::string>()==fresh->children[0]->uuid);
}
TEST_CASE("LOD add and remove events observe the updated levels array", "[objects][lod]") {
    auto lod=std::make_shared<LOD>();auto first=node("first"),second=node("second");std::vector<std::size_t> trace;
    first->addEventListener("added",[&](Event&){trace.push_back(lod->levels.length());lod->addLevel(second,20);});
    first->addEventListener("removed",[&](Event&){trace.push_back(lod->levels.length());});
    lod->addLevel(first,10);REQUIRE(trace==std::vector<std::size_t>{1});REQUIRE(lod->levels.length()==2);REQUIRE(lod->children.size()==2);
    REQUIRE(lod->removeLevel(10));REQUIRE(trace==std::vector<std::size_t>{1,1});REQUIRE(lod->children.size()==1);
}
TEST_CASE("LOD raycast delegates independently of visibility and ignores child return values", "[objects][lod]") {
    LOD lod;auto first=std::make_shared<RayNode>(),second=std::make_shared<RayNode>();lod.addLevel(first,0);lod.addLevel(second,10,.2);first->visible=false;second->visible=false;
    Raycaster ray({0,0,12},{0,0,-1});std::vector<Intersection> hits;
    REQUIRE(lod.raycast(ray,hits));REQUIRE(first->calls==0);REQUIRE(second->calls==1);REQUIRE(hits.size()==1);REQUIRE(hits[0].object==second.get());
    lod.matrixWorld.makeTranslation(0,0,8);lod.raycast(ray,hits);REQUIRE(first->calls==1);REQUIRE(hits.size()==2);
    LOD empty;REQUIRE(empty.raycast(ray,hits));REQUIRE(hits.size()==2);
    auto levels=lod.levels,record=levels.get("0");record.set("object",nullptr);
    REQUIRE_THROWS_AS(lod.raycast(ray,hits),JSTypeError);REQUIRE(hits.size()==2);
}
TEST_CASE("LOD update uses world positions from parents and leaves a sole level unchanged", "[objects][lod]") {
    auto parent=std::make_shared<Object3D>(),cameraParent=std::make_shared<Object3D>();auto lod=std::make_shared<LOD>();auto camera=std::make_shared<PerspectiveCamera>();
    parent->position.set(5,3,-2);parent->scale.set(2,3,4);parent->add(lod);lod->position.set(1,2,3);
    cameraParent->position.set(-3,5,7);cameraParent->add(camera);camera->position.set(10,4,13);camera->zoom=2;
    lod->addLevel(node("high"),0);lod->addLevel(node("low"),4);parent->updateMatrixWorld();cameraParent->updateMatrixWorld();lod->update(*camera);
    REQUIRE(lod->getCurrentLevel().toNumber()==1);REQUIRE_FALSE(lod->children[0]->visible);REQUIRE(lod->children[1]->visible);
    LOD single;auto only=node("only");only->visible=false;single.addLevel(only,0);single._currentLevel=7;single.update(JSValue(nullptr));REQUIRE_FALSE(only->visible);REQUIRE(single.getCurrentLevel().toNumber()==7);
}
TEST_CASE("LOD native wrapper retains readonly level identity and collects managed owner cycles", "[objects][lod][gc]") {
    auto root=JSValue::object();
    {auto warm=std::make_shared<LOD>();auto wrapper=nativeObject(warm);warm->addLevel(node("warm"));}
    root.collectGarbage();const auto baseline=root.heapObjectCount();std::weak_ptr<LOD> weak;
    {
        auto lod=std::make_shared<LOD>();weak=lod;auto wrapper=nativeObject(lod);const auto descriptor=wrapper.getOwnPropertyDescriptor("levels");
        REQUIRE(descriptor);REQUIRE_FALSE(*descriptor->writable);REQUIRE(*descriptor->enumerable);REQUIRE_FALSE(*descriptor->configurable);REQUIRE(wrapper.get("levels").sameValue(lod->levels));
        REQUIRE_THROWS_AS(wrapper.set("levels",JSValue::array()),JSTypeError);REQUIRE_FALSE(wrapper.erase("levels"));
        wrapper.set("autoUpdate","manual");wrapper.set("_currentLevel",17);REQUIRE(lod->autoUpdate.toString()=="manual");REQUIRE(lod->getCurrentLevel().toNumber()==17);
        lod->autoUpdate=wrapper;lod->_currentLevel=wrapper;
        auto levels=lod->levels,record=JSValue::object();record.set("object",wrapper);record.set("distance",0);record.set("hysteresis",0);levels.set("0",record);
        root.collectGarbage();REQUIRE_FALSE(weak.expired());
    }
    REQUIRE(root.collectGarbage()>0);REQUIRE(weak.expired());REQUIRE(root.heapObjectCount()==baseline);
}
