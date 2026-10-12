#include "catch.hpp"
#include "core/InterleavedBufferAttribute.h"
#include "core/BufferGeometry.h"
#include "objects/Mesh.h"

TEST_CASE("Interleaved attribute preserves data identity and never inherits BufferAttribute","[core][interleaved-attribute]") {
    static_assert(!std::is_base_of_v<BufferAttribute,InterleavedBufferAttribute>);
    Float32Array values({1,2,3,4,5,6,7,8});InterleavedBuffer data(values,4);InterleavedBufferAttribute a(data,3,0),b(data,1,3);
    REQUIRE(a.array.strictEquals(nativeArray(values)));REQUIRE(a.data.strictEquals(b.data));REQUIRE(a.getProperty("isBufferAttribute").isUndefined());
    a.setY(.25,99);REQUIRE(values[2]==99);b.setX(1,42);REQUIRE(values[7]==42);
    data.count=1.5;REQUIRE(a.getCount()==1.5);a.needsUpdate=true;b.needsUpdate=true;REQUIRE(data.version.toNumber()==2);
    const auto clone=a.cloneAttribute();REQUIRE(std::dynamic_pointer_cast<BufferAttribute>(clone));REQUIRE_FALSE(std::dynamic_pointer_cast<InterleavedBufferAttribute>(clone));
    REQUIRE(clone->getArray().size()==6);REQUIRE(clone->getX(1)==5);
}
TEST_CASE("Geometry clone retains shared interleaved storage across attribute and morph maps","[core][interleaved-attribute]") {
    InterleavedBuffer data(Float32Array({-1,-1,0,0,0,1,1,-1,0,0,0,1,0,1,0,0,0,1}),6);
    auto a=std::make_shared<InterleavedBufferAttribute>(data,3,0),b=std::make_shared<InterleavedBufferAttribute>(data,3,3);
    auto geometry=std::make_shared<BufferGeometry>();geometry->setAttribute("position",a).setAttribute("normal",b);geometry->morphAttributes["normal"]={b};
    const auto clone=geometry->clone();const auto ca=std::dynamic_pointer_cast<InterleavedBufferAttribute>(clone->getAttribute("position")),cb=std::dynamic_pointer_cast<InterleavedBufferAttribute>(clone->getAttribute("normal"));
    const auto cm=std::dynamic_pointer_cast<InterleavedBufferAttribute>(clone->morphAttributes["normal"][0]);
    REQUIRE(ca->data.strictEquals(cb->data));REQUIRE(cb->data.strictEquals(cm->data));REQUIRE_FALSE(ca->data.strictEquals(data));REQUIRE_FALSE(ca->array.strictEquals(a->array));
    geometry->translate(2,3,4);REQUIRE(a->getX(0)==1);REQUIRE(b->getZ(0)==1);REQUIRE(data.version.toNumber()==2);
    geometry->computeBoundingBox();REQUIRE(geometry->boundingBox->min.equals(Vector3(1,2,4)));REQUIRE(geometry->boundingBox->max.equals(Vector3(3,4,4)));
    const auto json=geometry->toJSON().at("data");REQUIRE(json.contains("interleavedBuffers"));REQUIRE(json.contains("arrayBuffers"));
    geometry->setIndex(std::vector<std::uint32_t>{2,0,1});const auto expanded=geometry->toNonIndexed();
    REQUIRE(expanded->getAttribute("position")->getX(0)==2);REQUIRE(expanded->getAttribute("normal")->getZ(0)==1);REQUIRE(std::dynamic_pointer_cast<BufferAttribute>(expanded->getAttribute("position")));
}
TEST_CASE("Managed interleaved attributes participate in native geometry cycle collection","[core][interleaved-attribute][gc]") {
    auto anchor=JSValue::object();auto mesh=std::make_shared<Mesh>();std::weak_ptr<Mesh> weak=mesh;
    {InterleavedBuffer data(Float32Array({1,2,3,4,5,6}),3);auto attr=std::make_shared<InterleavedBufferAttribute>(data,3,0);
        mesh->geometry->setAttribute("position",attr);attr->object().set("owner",nativeObject(mesh));data.array.get("buffer").set("owner",nativeObject(mesh));}
    anchor.collectGarbage();REQUIRE_FALSE(weak.expired());mesh.reset();anchor.collectGarbage();REQUIRE(weak.expired());
}
TEST_CASE("Native interleaved transforms borrow matrices through callbacks and snapshot on unwind","[core][interleaved-attribute]") {
    using Args=std::vector<JSValue>;
    for(int operation=0;operation<3;++operation) for(bool throws:{false,true}) {
        auto saved=JSValue::object();auto matrix4=std::make_shared<Matrix4>();auto matrix3=std::make_shared<Matrix3>();
        auto native=operation==1?nativeMatrix(matrix3):nativeMatrix(matrix4);
        InterleavedBufferAttribute attribute(InterleavedBuffer(Float32Array({1,2,3}),3),3,0);
        const char* name=operation==0?"applyMatrix4":operation==1?"applyNormalMatrix":"transformDirection";
        const auto original=attribute.object().get(name);
        attribute.object().set(name,JSValue::function([](const JSValue& self,const Args& args,const Args& captures)->JSValue {
            auto saved=captures[0];saved.set("matrix",args[0]);saved.set("elements",args[0].get("elements"));return captures[1].call(self,args);
        },{saved,original}));
        const auto getX=attribute.object().get("getX");attribute.object().set("getX",JSValue::function([](const JSValue& self,const Args& args,const Args& captures)->JSValue {
            auto elements=captures[0].get("elements");elements.set(captures[1].string(),7);if(captures[2].toBoolean())throw JSTypeError("test callback");return captures[3].call(self,args);
        },{native,operation==0?"12":"0",throws,getX}));
        const auto invoke=[&]{if(operation==0)attribute.applyMatrix4(*matrix4);else if(operation==1)attribute.applyNormalMatrix(*matrix3);else attribute.transformDirection(*matrix4);};
        if(throws) REQUIRE_THROWS_AS(invoke(),JSTypeError);else {invoke();attribute.object().set("getX",getX);
            REQUIRE(attribute.getX(0)==Approx(operation==0?8:7/std::sqrt(62.)));}
        const auto key=operation==0?"12":"0";REQUIRE(saved.get("elements").get(key).toNumber()==7);
        native.get("elements").set(key,99);native={};matrix3.reset();matrix4.reset();
        REQUIRE(saved.get("elements").get(key).toNumber()==7);REQUIRE(saved.get("matrix").get("elements").sameValue(saved.get("elements")));
        auto elements=saved.get("elements");elements.set(key,11);REQUIRE(saved.get("matrix").get("elements").get(key).toNumber()==11);
    }
}
TEST_CASE("Retained matrix snapshots release callback and element cycles","[core][interleaved-attribute][gc]") {
    auto anchor=JSValue::object();std::weak_ptr<void> weak;
    {
        using Args=std::vector<JSValue>;auto saved=JSValue::object();Matrix4 matrix;
        InterleavedBufferAttribute attribute(InterleavedBuffer(Float32Array({1,2,3}),3),3,0);
        const auto original=attribute.object().get("applyMatrix4");
        attribute.object().set("applyMatrix4",JSValue::function([](const JSValue& self,const Args& args,const Args& captures)->JSValue {
            auto saved=captures[0],matrix=args[0];saved.set("matrix",matrix);matrix.set("owner",saved);matrix.get("elements").set("owner",saved);return captures[1].call(self,args);
        },{saved,original}));attribute.applyMatrix4(matrix);
        weak=saved.get("matrix").hostPayload("three.Matrix4");REQUIRE_FALSE(weak.expired());
    }
    anchor.collectGarbage();REQUIRE(weak.expired());
}
