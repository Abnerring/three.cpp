#include "catch.hpp"
#include "core/BufferAttribute.h"
#include "core/InstancedBufferAttribute.h"
#include "core/NativeObject.h"
#include "renderers/webgl/WebGLAttributes.h"
#include <limits>

namespace {
using Args=std::vector<JSValue>;
JSPropertyDescriptor data(JSValue value) { JSPropertyDescriptor d;d.value=value;d.writable=d.enumerable=d.configurable=true;return d; }
JSValue callback(const JSValue& self,const Args&,const Args& cap) {auto target=cap[0];target.set("self",self);return self;}
JSValue noop(const JSValue&,const Args&,const Args&){return {};}
}
TEST_CASE("BufferAttribute upload fields share one managed identity and preserve arbitrary values", "[attribute][runtime]") {
    auto a=std::make_shared<Float32BufferAttribute>(std::vector<float>{1,2,3},3);
    auto object=a->object();REQUIRE(object.sameValue(nativeAttribute(a)));
    REQUIRE(object.get("array").sameValue(nativeArray(a->getArray())));
    REQUIRE_FALSE(object.hasOwn("isFloat16BufferAttribute"));
    REQUIRE(object.get("isFloat16BufferAttribute").isUndefined());
    auto version=JSValue::object();version.setPrimitiveConverter(JSValue::function(+[](const JSValue&,const Args& args,const Args&)->JSValue {REQUIRE(args[0].strictEquals("number"));return "4";}));
    a->version=version;REQUIRE(object.get("version").sameValue(version));a->needsUpdate=1;REQUIRE(a->version.value().sameValue(version));a->needsUpdate=true;REQUIRE(a->version.strictEquals(5));
    auto usage=JSValue::object();a->setUsage(usage);REQUIRE(a->usage.value().sameValue(usage));REQUIRE(object.get("usage").sameValue(usage));
    object.set("gpuType","gpu");REQUIRE(a->gpuType.strictEquals("gpu"));object.set("isFloat16BufferAttribute","half");REQUIRE(a->isFloat16BufferAttribute.strictEquals("half"));
    const auto ranges=a->updateRanges.value();auto start=JSValue::object();a->addUpdateRange(start,"count");
    REQUIRE(ranges.sameValue(object.get("updateRanges")));REQUIRE(ranges.get("0").get("start").sameValue(start));REQUIRE(ranges.get("0").get("count").strictEquals("count"));
    a->clearUpdateRanges();REQUIRE(a->updateRanges.value().sameValue(ranges));REQUIRE(ranges.length()==0);
    const auto descriptor=object.getOwnPropertyDescriptor("version");REQUIRE(descriptor->value->strictEquals(5));REQUIRE(descriptor->writable==true);REQUIRE(descriptor->enumerable==true);REQUIRE(descriptor->configurable==true);
}
TEST_CASE("BufferAttribute native field access preserves getters receivers and dynamic method replacement", "[attribute][runtime]") {
    auto a=std::make_shared<Float32BufferAttribute>(std::vector<float>{1},1);auto object=nativeAttribute(a),log=JSValue::array();
    JSPropertyDescriptor d;d.getter=JSValue::function(+[](const JSValue& self,const Args&,const Args& cap)->JSValue {REQUIRE(self.sameValue(cap[0]));cap[1].get("push").call(cap[1],{"get"});return "8";},{object,log});
    d.setter=JSValue::function(+[](const JSValue& self,const Args& args,const Args& cap)->JSValue {REQUIRE(self.sameValue(cap[0]));cap[1].get("push").call(cap[1],{args[0]});return {};},{object,log});d.configurable=true;d.enumerable=true;object.defineProperty("version",d);
    REQUIRE(a->version.toNumber()==8);REQUIRE(a->getProperty("version").strictEquals("8"));a->needsUpdate=true;REQUIRE(log.length()==4);REQUIRE(log.get("3").strictEquals(9));
    auto target=JSValue::object();object.set("clearUpdateRanges",JSValue::function(callback,{target}));REQUIRE(a->clearUpdateRanges().sameValue(object));REQUIRE(target.get("self").sameValue(object));
    auto function=JSValue::function(callback,{target});a->onUpload(function);REQUIRE(a->onUploadCallback.value().sameValue(function));REQUIRE(a->invokeUploadCallback().sameValue(object));
    a->onUploadCallback=nullptr;REQUIRE_THROWS_AS(a->invokeUploadCallback(),JSTypeError);
    object.defineProperty("usage",data(7));JSPropertyDescriptor readonly;readonly.writable=false;object.defineProperty("usage",readonly);REQUIRE_THROWS_AS(a->setUsage(8),JSTypeError);REQUIRE(a->usage.strictEquals(7));
}
TEST_CASE("A managed BufferAttribute does not keep the native handle alive and reconstructs exact storage", "[attribute][runtime][gc]") {
    auto witness=JSValue::object();JSValue object;std::weak_ptr<BufferAttribute> weak;std::weak_ptr<BufferAttributeStorage> storage;std::uint64_t id=0;
    {auto a=std::make_shared<Float32BufferAttribute>(std::vector<float>{1,2,3},3);a->name="source";id=a->id;object=nativeAttribute(a);weak=a;storage=a->runtimeStorage();}
    REQUIRE(weak.expired());REQUIRE_FALSE(storage.expired());
    auto restored=std::dynamic_pointer_cast<BufferAttribute>(vertexAttribute(object));REQUIRE(restored);REQUIRE(std::dynamic_pointer_cast<Float32BufferAttribute>(restored));REQUIRE(restored->id==id);REQUIRE(restored->name=="source");
    REQUIRE(restored->object().sameValue(object));object.get("setX").call(object,{0,7});REQUIRE(restored->getX(0)==7);REQUIRE(restored->runtimeStorage()==storage.lock());
    restored.reset();object={};witness.collectGarbage();REQUIRE(storage.expired());
}
TEST_CASE("Stack and Float16 attribute wrappers retain storage without dangling native pointers", "[attribute][runtime][gc]") {
    JSValue object;std::uint64_t id=0;
    {Float16BufferAttribute a({0x3c00,0x4000},1);object=a.object();id=a.id;a.isFloat16BufferAttribute=false;}
    REQUIRE(object.get("getX").call(object,{0}).number()==1);
    auto restored=std::dynamic_pointer_cast<Float16BufferAttribute>(vertexAttribute(object));REQUIRE(restored);REQUIRE(restored->id==id);restored->setX(1,.5);REQUIRE(object.get("getX").call(object,{1}).number()==.5);
    JSValue instance;
    {auto a=std::make_shared<InstancedBufferAttribute>(Float32Array{1,2},1,false,7);instance=nativeAttribute(a);}
    auto instanced=std::dynamic_pointer_cast<InstancedBufferAttribute>(vertexAttribute(instance));REQUIRE(instanced);REQUIRE(instanced->meshPerAttribute.strictEquals(7));
}
TEST_CASE("Instanced attribute storage traces its real extra root after native handle retirement", "[attribute][runtime][gc]") {
    auto witness=JSValue::object();JSWeakValue weakObject,weakExtra;std::weak_ptr<BufferAttributeStorage> weakStorage;
    {auto a=std::make_shared<InstancedBufferAttribute>(Float32Array{1,2},1);auto object=nativeAttribute(a),extra=JSValue::object();extra.set("attribute",object);a->meshPerAttribute=extra;weakObject=JSWeakValue(object);weakExtra=JSWeakValue(extra);weakStorage=a->runtimeStorage();
        object={};extra={};witness.collectGarbage();REQUIRE_FALSE(weakObject.expired());REQUIRE_FALSE(weakExtra.expired());}
    witness.collectGarbage();REQUIRE(weakObject.expired());REQUIRE(weakExtra.expired());REQUIRE(weakStorage.expired());
}
TEST_CASE("Native-only attributes retain managed fields and unreachable mixed cycles are collected", "[attribute][runtime][gc]") {
    auto witness=JSValue::object();auto a=std::make_shared<Float32BufferAttribute>(std::vector<float>{1},1);JSWeakValue weakObject,weakRanges,weakFunction;std::weak_ptr<BufferAttributeStorage> weakStorage=a->runtimeStorage();
    {auto object=nativeAttribute(a),ranges=a->updateRanges.value();auto function=JSValue::function(noop,{object,ranges});a->onUpload(function);ranges.set("0",object);weakObject=JSWeakValue(object);weakRanges=JSWeakValue(ranges);weakFunction=JSWeakValue(function);}
    witness.collectGarbage();REQUIRE_FALSE(weakObject.expired());REQUIRE_FALSE(weakRanges.expired());REQUIRE_FALSE(weakFunction.expired());REQUIRE(a->updateRanges.get("0").sameValue(a->object()));
    std::weak_ptr<BufferAttribute> weakNative=a;a.reset();REQUIRE(weakNative.expired());witness.collectGarbage();REQUIRE(weakObject.expired());REQUIRE(weakRanges.expired());REQUIRE(weakFunction.expired());REQUIRE(weakStorage.expired());
}
TEST_CASE("WebGLAttributes consumes real native fields and its weak cache does not root attribute keys", "[attribute][runtime][gc]") {
    auto witness=JSValue::object(),gl=JSValue::object(),trace=JSValue::array();gl.set("FLOAT",5126);
    gl.set("createBuffer",JSValue::function(+[](const JSValue&,const Args&,const Args&)->JSValue{return JSValue::object();}));
    for(const char* name:{"bindBuffer","bufferData","bufferSubData","deleteBuffer"})gl.set(name,JSValue::function(+[](const JSValue&,const Args&,const Args&cap)->JSValue{cap[0].get("push").call(cap[0],{cap[1]});return {};},{trace,name}));
    WebGLAttributes attributes(gl);JSWeakValue weakObject,weakCached;std::weak_ptr<BufferAttributeStorage> weakStorage;
    {auto a=std::make_shared<Float32BufferAttribute>(std::vector<float>{1,2,3},3);auto object=nativeAttribute(a);weakStorage=a->runtimeStorage();unsigned callbacks=0;a->onUpload([&]{++callbacks;});a->addUpdateRange(0,1);attributes.update(object,34962);REQUIRE(callbacks==1);REQUIRE(a->updateRanges.value().length()==1);
        a->needsUpdate=true;attributes.update(object,34962);REQUIRE(callbacks==2);REQUIRE(a->updateRanges.value().length()==0);auto cached=attributes.get(object);cached.set("key",object);weakObject=JSWeakValue(object);weakCached=JSWeakValue(cached);}
    witness.collectGarbage();REQUIRE(weakObject.expired());REQUIRE(weakCached.expired());REQUIRE(weakStorage.expired());
}
