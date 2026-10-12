#include "catch.hpp"
#include "core/NativeObject.h"
#include "helpers/VertexNormalsHelper.h"
#include "helpers/VertexTangentsHelper.h"
#include "objects/Group.h"
#include "objects/Points.h"
#include "objects/Sprite.h"
#include "cameras/PerspectiveCamera.h"

namespace {
std::shared_ptr<Mesh> extensionSource() {
    auto geometry=std::make_shared<BufferGeometry>();
    geometry->setAttribute("position",std::make_shared<Float32BufferAttribute>(std::vector<double>{0,0,0},3));
    geometry->setAttribute("normal",std::make_shared<Float32BufferAttribute>(std::vector<double>{0,1,0},3));
    geometry->setAttribute("tangent",std::make_shared<Float32BufferAttribute>(std::vector<double>{1,0,0,1},4));
    return std::make_shared<Mesh>(geometry);
}
template<class T> void liveFields() {
    auto source=extensionSource();auto helper=std::make_shared<T>(source);auto wrapper=nativeObject(helper),size=JSValue::object();
    size.set("marker",42);wrapper.set("size",size);REQUIRE(helper->size.sameValue(size));REQUIRE(wrapper.get("size").sameValue(size));
    helper->size="2";REQUIRE(wrapper.get("size").string()=="2");
    wrapper.set("size",JSValue());REQUIRE(helper->size.isUndefined());wrapper.set("size",nullptr);REQUIRE(helper->size.isNull());
    REQUIRE(wrapper.get("object").sameValue(nativeObject(source)));REQUIRE(nativeObject(helper).sameValue(wrapper));
    const std::vector<std::shared_ptr<Object3D>> sources{std::make_shared<Object3D>(),std::make_shared<Group>(),
        std::make_shared<Points>(),std::make_shared<Line>(),std::make_shared<Sprite>(),std::make_shared<PerspectiveCamera>(),extensionSource()};
    for(const auto& next:sources){wrapper.set("object",nativeObject(next));REQUIRE(helper->object==next);REQUIRE(wrapper.get("object").sameValue(nativeObject(next)));}
    helper->object=source;REQUIRE(wrapper.get("object").sameValue(nativeObject(source)));
    wrapper.set("object",nullptr);REQUIRE_FALSE(helper->object);REQUIRE(wrapper.get("object").isNull());
    REQUIRE_THROWS_AS(wrapper.set("object",JSValue::object()),JSTypeError);
    if constexpr(std::is_same_v<T,VertexNormalsHelper>){REQUIRE(wrapper.get("isVertexNormalsHelper").toBoolean());
        wrapper.set("isVertexNormalsHelper",false);REQUIRE_FALSE(helper->isVertexNormalsHelper);REQUIRE_FALSE(wrapper.get("isVertexNormalsHelper").toBoolean());
        wrapper.set("isVertexNormalsHelper",true);REQUIRE(helper->isVertexNormalsHelper);helper->isVertexNormalsHelper=false;REQUIRE_FALSE(wrapper.get("isVertexNormalsHelper").toBoolean());}
    else REQUIRE_FALSE(wrapper.has("isVertexNormalsHelper"));
}
template<class T> void parentOnly() {
    auto witness=JSValue::object();auto parent=std::make_shared<Object3D>();std::weak_ptr<T> weakHelper;std::weak_ptr<Object3D> weakSource;
    JSWeakValue weakSize,weakSourceWrapper;
    {
        auto helper=std::make_shared<T>(extensionSource());parent->add(helper);weakHelper=helper;weakSource=helper->object;
        // No helper wrapper is created: only the parent exposes this subclass.
        helper->size=JSValue::object();helper->size.set("marker",43);weakSize=JSWeakValue(helper->size);
        auto sourceWrapper=nativeObject(helper->object);sourceWrapper.set("extra",44);weakSourceWrapper=JSWeakValue(sourceWrapper);
        auto parentWrapper=nativeObject(parent);
    }
    witness.collectGarbage();REQUIRE_FALSE(weakHelper.expired());REQUIRE_FALSE(weakSource.expired());
    REQUIRE(weakSize.lock()->get("marker").number()==43);REQUIRE(weakSourceWrapper.lock()->get("extra").number()==44);
    parent.reset();witness.collectGarbage();REQUIRE(weakHelper.expired());REQUIRE(weakSource.expired());
    REQUIRE(weakSize.expired());REQUIRE(weakSourceWrapper.expired());
}
template<class T> void managedCycles() {
    auto witness=JSValue::object();
    for(int resource=0;resource<4;++resource){
        std::weak_ptr<T> weakHelper;std::weak_ptr<Object3D> weakSource;JSWeakValue weakWrapper;
        {
            auto helper=std::make_shared<T>(extensionSource());auto wrapper=nativeObject(helper);weakHelper=helper;weakSource=helper->object;weakWrapper=JSWeakValue(wrapper);
            if(resource==0){helper->size=JSValue::object();helper->size.set("owner",wrapper);}
            if(resource==1)nativeObject(helper->object).set("owner",wrapper);
            if(resource==2)nativeGeometry(helper->geometry).set("owner",wrapper);
            if(resource==3)nativeMaterial(helper->material).set("owner",wrapper);
        }
        witness.collectGarbage();REQUIRE(weakHelper.expired());REQUIRE(weakSource.expired());REQUIRE(weakWrapper.expired());
    }
}
template<class T> void replacement() {
    auto witness=JSValue::object();auto helper=std::make_shared<T>(extensionSource());auto wrapper=nativeObject(helper);
    std::weak_ptr<Object3D> oldSource=helper->object;std::weak_ptr<BufferGeometry> oldGeometry=helper->geometry;std::weak_ptr<Material> oldMaterial=helper->material;
    auto size=JSValue::object();helper->size=size;JSWeakValue oldSize(size),oldSourceWrapper(nativeObject(helper->object));
    nativeObject(helper->object).set("extra",45);size={};
    wrapper.set("object",nativeObject(extensionSource()));wrapper.set("size",JSValue::object());
    wrapper.set("geometry",nativeGeometry(std::make_shared<BufferGeometry>()));wrapper.set("material",nativeMaterial(std::make_shared<LineBasicMaterial>()));
    witness.collectGarbage();REQUIRE(oldSource.expired());REQUIRE(oldSourceWrapper.expired());REQUIRE(oldSize.expired());
    REQUIRE(oldGeometry.expired());REQUIRE(oldMaterial.expired());REQUIRE(wrapper.get("size").sameValue(helper->size));
}
template<class T> void externalSourceAndPureCycle() {
    auto witness=JSValue::object();std::weak_ptr<T> weakHelper;JSWeakValue weakSize,weakWrapper;
    {
        auto helper=std::make_shared<T>(extensionSource());weakHelper=helper;auto wrapper=nativeObject(helper);weakWrapper=JSWeakValue(wrapper);
        helper->object=helper;helper->size=JSValue::object();weakSize=JSWeakValue(helper->size);helper->size.set("owner",wrapper);
    }
    witness.collectGarbage();REQUIRE_FALSE(weakHelper.expired());REQUIRE_FALSE(weakSize.expired());REQUIRE_FALSE(weakWrapper.expired());
    {auto helper=weakHelper.lock();helper->object.reset();}
    witness.collectGarbage();REQUIRE(weakHelper.expired());REQUIRE(weakSize.expired());REQUIRE(weakWrapper.expired());
}
}
TEST_CASE("Addon extension exposes live fields and any supported Object3D reference","[native-extension]"){liveFields<VertexNormalsHelper>();liveFields<VertexTangentsHelper>();}
TEST_CASE("Parent-only native ownership reaches unwrapped addon fields and source extras","[native-extension]"){parentOnly<VertexNormalsHelper>();parentOnly<VertexTangentsHelper>();}
TEST_CASE("Addon size source geometry and material JS/native back references collect","[native-extension]"){managedCycles<VertexNormalsHelper>();managedCycles<VertexTangentsHelper>();}
TEST_CASE("Replacing addon and inherited fields releases the former resources","[native-extension]"){replacement<VertexNormalsHelper>();replacement<VertexTangentsHelper>();}
TEST_CASE("Actual addon shared_ptr slots preserve pure native cycles until explicitly broken","[native-extension]"){externalSourceAndPureCycle<VertexNormalsHelper>();externalSourceAndPureCycle<VertexTangentsHelper>();}
