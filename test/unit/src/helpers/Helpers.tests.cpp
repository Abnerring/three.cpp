#include "catch.hpp"
#include "helpers/ArrowHelper.h"
#include "helpers/AxesHelper.h"
#include "helpers/Box3Helper.h"
#include "helpers/BoxHelper.h"
#include "helpers/CameraHelper.h"
#include "helpers/DirectionalLightHelper.h"
#include "helpers/HemisphereLightHelper.h"
#include "helpers/PlaneHelper.h"
#include "helpers/PointLightHelper.h"
#include "helpers/SkeletonHelper.h"
#include "helpers/SpotLightHelper.h"
#include "cameras/PerspectiveCamera.h"
#include "cameras/ArrayCamera.h"
#include "objects/ClippingGroup.h"
#include "objects/Bone.h"
#include "core/InstancedBufferGeometry.h"
#include "geometries/BoxGeometry.h"

TEST_CASE("Helper matrix aliases remain live and ordinary Matrix4 copies are independent","[helpers][matrix]") {
    auto light=std::make_shared<PointLight>();PointLightHelper helper(light);light->matrixWorld.makeTranslation(3,4,5);
    REQUIRE(helper.matrix.elements().data()==light->matrixWorld.elements().data());REQUIRE(helper.matrix[12]==3);
    helper.matrix.setPosition(8,9,10);REQUIRE(light->matrixWorld[12]==8);auto copied=helper.matrix;copied.setPosition(2,3,4);REQUIRE(helper.matrix[12]==8);
    Matrix4 assigned;assigned=helper.matrix;assigned.identity();REQUIRE(helper.matrix[12]==8);
    auto original=light;helper.light=std::make_shared<PointLight>();helper.update();REQUIRE(helper.matrix.aliasTarget().get()==&original->matrixWorld);
    helper.matrix.multiply(Matrix4().makeTranslation(1,2,3));REQUIRE(original->matrixWorld[12]==9);
}
TEST_CASE("Helper matrix wrappers preserve source identity and replacement does not rebind","[helpers][native]") {
    auto camera=std::make_shared<PerspectiveCamera>();auto helper=std::make_shared<CameraHelper>(camera);auto h=nativeObject(helper),c=nativeObject(camera);
    REQUIRE(h.get("matrix").sameValue(c.get("matrixWorld")));h.get("matrix").get("elements").set("12",17);REQUIRE(camera->matrixWorld[12]==17);
    auto replacement=std::make_shared<PerspectiveCamera>();h.set("camera",nativeObject(replacement));REQUIRE(helper->camera==replacement);REQUIRE(h.get("matrix").sameValue(c.get("matrixWorld")));
}
TEST_CASE("Arrow shared geometries and Directional shared material emit every dispose event","[helpers]") {
    ArrowHelper first,second;REQUIRE(first.line->geometry==second.line->geometry);REQUIRE(first.cone->geometry==second.cone->geometry);REQUIRE(first.line->material!=second.line->material);
    DirectionalLightHelper helper(std::make_shared<DirectionalLight>());REQUIRE(helper.lightPlane->material==helper.targetLine->material);
    std::vector<int> trace;helper.addEventListener("dispose",[&](Event&){trace.push_back(0);});helper.lightPlane->geometry->addEventListener("dispose",[&](Event&){trace.push_back(1);});helper.lightPlane->material->addEventListener("dispose",[&](Event&){trace.push_back(2);});helper.targetLine->geometry->addEventListener("dispose",[&](Event&){trace.push_back(3);});helper.dispose();REQUIRE(trace==std::vector<int>{0,1,2,3,2});
}
TEST_CASE("Helper disposal observes resources replaced by earlier disposal listeners","[helpers]") {
    auto helper=std::make_shared<AxesHelper>();auto old=helper->material;auto replacement=std::make_shared<LineBasicMaterial>();int previous=0,next=0;
    old->addEventListener("dispose",[&](Event&){++previous;});replacement->addEventListener("dispose",[&](Event&){++next;});helper->geometry->addEventListener("dispose",[&](Event&){helper->material=replacement;});helper->dispose();REQUIRE(previous==0);REQUIRE(next==1);
}
TEST_CASE("Helper copy and clone preserve upstream class-specific omissions","[helpers]") {
    auto plane=std::make_shared<Plane>();PlaneHelper original(plane,4);auto cloned=std::dynamic_pointer_cast<PlaneHelper>(original.clone());REQUIRE(cloned);REQUIRE_FALSE(cloned->plane);REQUIRE(cloned->size==1);REQUIRE(cloned->children.size()==2);REQUIRE(cloned->geometry==original.geometry);
    Box3Helper box(std::make_shared<Box3>());REQUIRE_FALSE(std::dynamic_pointer_cast<Box3Helper>(box.clone())->box);
    PointLightHelper light(std::make_shared<PointLight>());REQUIRE_THROWS_AS(light.clone(),JSTypeError);
    ArrowHelper arrow;auto a=std::dynamic_pointer_cast<ArrowHelper>(arrow.clone());REQUIRE(a->children.size()==2);REQUIRE(a->line->geometry==arrow.line->geometry);REQUIRE(a->line->material==arrow.line->material);
}
TEST_CASE("Box3 helper empty updates preserve transforms and skip child updates","[helpers]") {
    auto box=std::make_shared<Box3>();Box3Helper helper(box);helper.position.set(5,6,7);helper.matrixWorldNeedsUpdate=true;helper.updateMatrixWorld(true);REQUIRE(helper.position.x==5);REQUIRE(helper.matrixWorldNeedsUpdate);
    box->set({1,2,3},{5,8,13});helper.updateMatrixWorld();REQUIRE(helper.position.equals(Vector3(3,5,8)));REQUIRE(helper.scale.equals(Vector3(2,3,5)));
}
TEST_CASE("Light helper color retains mutable Color identity and null overrides do not fall back","[helpers]") {
    auto color=std::make_shared<Color>(.2,.3,.4);auto light=std::make_shared<PointLight>(Color(0xff0000));PointLightHelper helper(light,1,color);REQUIRE(Helper_detail::meshColor(helper.material).equals(*color));color->setRGB(.7,.8,.9);helper.update();REQUIRE(Helper_detail::meshColor(helper.material).equals(*color));helper.color=nullptr;light->color.setHex(0x00ff00);helper.update();REQUIRE(Helper_detail::meshColor(helper.material).equals(*color));helper.color={};helper.update();REQUIRE(Helper_detail::meshColor(helper.material).equals(light->color));
}
TEST_CASE("Helpers report source aliases and runtime colors to cycle collection","[helpers][runtime-native]") {
    auto witness=JSValue::object();std::weak_ptr<PointLightHelper> weak;std::weak_ptr<PointLight> original;
    {auto light=std::make_shared<PointLight>();auto helper=std::make_shared<PointLightHelper>(light);weak=helper;original=light;auto host=nativeObject(helper);helper->color=JSValue::object();helper->color.set("owner",host);auto savedMatrix=host.get("matrix");savedMatrix.set("helper",host);helper->light=std::make_shared<PointLight>();light.reset();witness.collectGarbage();REQUIRE_FALSE(original.expired());REQUIRE(host.get("matrix").sameValue(savedMatrix));}
    witness.collectGarbage();REQUIRE(weak.expired());REQUIRE(original.expired());
}
TEST_CASE("Skeleton helper native bones and shared Camera and Plane arrays are live","[helpers][native]") {
    auto root=std::make_shared<Bone>(),child=std::make_shared<Bone>();root->add(child);auto skeleton=std::make_shared<SkeletonHelper>(root);auto host=nativeObject(skeleton);REQUIRE(host.get("isSkeletonHelper").toBoolean());REQUIRE(host.get("bones").get("length").number()==2);host.get("bones").set("0",nativeObject(child));REQUIRE(skeleton->bones[0]==child);
    auto camera=std::make_shared<ArrayCamera>();camera->cameras.push_back(std::make_shared<PerspectiveCamera>());auto c=nativeObject(camera);auto other=std::make_shared<PerspectiveCamera>();c.get("cameras").set("0",nativeObject(other));REQUIRE(camera->cameras[0]==other);
    auto group=std::make_shared<ClippingGroup>();group->clippingPlanes.push_back(std::make_shared<Plane>());auto g=nativeObject(group);g.get("clippingPlanes").get("0").set("constant",17);g.get("clippingPlanes").get("0").get("normal").set("y",3);REQUIRE(group->clippingPlanes[0]->constant==17);REQUIRE(group->clippingPlanes[0]->normal.y==3);g.set("enabled","yes");REQUIRE(group->enabled.string()=="yes");
}
TEST_CASE("New group and geometry runtime roots collect managed cycles","[helpers][runtime-native]") {
    auto witness=JSValue::object();std::weak_ptr<ClippingGroup> group;std::weak_ptr<InstancedBufferGeometry> geometry;
    {auto g=std::make_shared<ClippingGroup>();group=g;auto h=nativeObject(g);g->enabled=h;auto b=std::make_shared<InstancedBufferGeometry>();geometry=b;auto bh=nativeGeometry(b);bh.set("instanceCount",bh);REQUIRE(b->instanceCount.sameValue(bh));}
    witness.collectGarbage();REQUIRE(group.expired());REQUIRE(geometry.expired());
}
