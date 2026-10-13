#include "core/NativeObject.h"
#include "core/InstancedBufferAttribute.h"
#include "catch.hpp"
namespace {using Args=std::vector<JSValue>;
std::shared_ptr<InstancedBufferAttribute> instance(JSValue mesh=2){return std::make_shared<InstancedBufferAttribute>(Float32Array(std::vector<float>(6)),3,false,mesh);}}
TEST_CASE("Native instanced attribute exposes live flags and storage value", "[core][native-instanced-attribute]") {
    auto native=instance();auto object=nativeAttribute(native);REQUIRE(object.get("isInstancedBufferAttribute").strictEquals(true));REQUIRE(object.get("meshPerAttribute").strictEquals(2));
    native->meshPerAttribute=3;REQUIRE(object.get("meshPerAttribute").strictEquals(3));object.set("meshPerAttribute",4);REQUIRE(native->meshPerAttribute.strictEquals(4));
    object.set("isInstancedBufferAttribute",false);REQUIRE(object.get("isInstancedBufferAttribute").strictEquals(false));REQUIRE(native->isInstancedBufferAttribute);
    REQUIRE(object.erase("isInstancedBufferAttribute"));REQUIRE(object.get("isInstancedBufferAttribute").isUndefined());
}
TEST_CASE("Native instanced descriptor storage releases every displaced object", "[core][native-instanced-attribute][gc]") {
    auto anchor=JSValue::object();
    for(bool reinsert:{false,true}) {
        JSWeakValue old,live;std::weak_ptr<BufferAttribute> nativeWeak;
        {
            auto initial=JSValue::object();auto native=instance(initial);nativeWeak=native;auto object=nativeAttribute(native);
            if(reinsert){REQUIRE(object.erase("meshPerAttribute"));object.set("meshPerAttribute",initial);}
            old=JSWeakValue(initial);initial={};auto replacement=JSValue::object();live=JSWeakValue(replacement);object.set("meshPerAttribute",replacement);replacement={};
            anchor.collectGarbage();REQUIRE(old.expired());REQUIRE_FALSE(live.expired());
            object.erase("meshPerAttribute");anchor.collectGarbage();REQUIRE(live.expired());
            auto accessorState=JSValue::object();accessorState.set("attribute",object);JSWeakValue accessorWeak(accessorState);
            JSPropertyDescriptor d;d.getter=JSValue::function(+[](const JSValue&,const Args&,const Args& cap)->JSValue{return cap[0].get("attribute").get("count");},{accessorState});d.configurable=d.enumerable=true;object.defineProperty("meshPerAttribute",d);
            accessorState={};d={};native.reset();REQUIRE(nativeWeak.expired());anchor.collectGarbage();REQUIRE_FALSE(accessorWeak.expired());REQUIRE(object.get("meshPerAttribute").strictEquals(2));
            object.erase("meshPerAttribute");anchor.collectGarbage();REQUIRE(accessorWeak.expired());
        }
        anchor.collectGarbage();REQUIRE(live.expired());
    }
}
TEST_CASE("Native slot replacement immediately releases displaced managed data", "[core][native-instanced-attribute][gc]") {
    auto anchor=JSValue::object();auto initial=JSValue::object();auto native=instance(initial);auto object=nativeAttribute(native);JSWeakValue old(initial);initial={};
    auto replacement=JSValue::object();JSWeakValue current(replacement);native->meshPerAttribute=replacement;replacement={};
    anchor.collectGarbage();REQUIRE(old.expired());REQUIRE_FALSE(current.expired());REQUIRE(object.get("meshPerAttribute").sameValue(*current.lock()));
    JSPropertyDescriptor readonly;readonly.writable=false;readonly.configurable=false;object.defineProperty("meshPerAttribute",readonly);
    JSPropertyDescriptor same;same.value=native->meshPerAttribute;REQUIRE(object.defineOwnProperty("meshPerAttribute",same));
    JSPropertyDescriptor changed;changed.value=JSValue::object();REQUIRE_FALSE(object.defineOwnProperty("meshPerAttribute",changed));
}
TEST_CASE("Native instanced readonly and deleted properties obey ordinary descriptors", "[core][native-instanced-attribute]") {
    auto native=instance();auto object=nativeAttribute(native);JSPropertyDescriptor d;d.value=3;d.writable=false;d.configurable=true;d.enumerable=false;object.defineProperty("meshPerAttribute",d);
    REQUIRE_THROWS_AS(object.set("meshPerAttribute",4),JSTypeError);REQUIRE(object.get("meshPerAttribute").strictEquals(3));REQUIRE(native->meshPerAttribute.strictEquals(3));
    REQUIRE(object.erase("meshPerAttribute"));object.preventExtensions();REQUIRE_THROWS_AS(object.set("meshPerAttribute",4),JSTypeError);REQUIRE_FALSE(object.hasOwn("meshPerAttribute"));REQUIRE(native->meshPerAttribute.isUndefined());
}
