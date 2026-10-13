#include "catch.hpp"
#include "core/JSValue.h"
#include "core/NativeArray.h"

namespace {
using Args=std::vector<JSValue>;
JSValue shift(){return JSValue::array().get("shift");}
JSValue array(const Args& values){auto result=JSValue::array();for(const auto& value:values)result.set(std::to_string(result.length()),value);return result;}
}
TEST_CASE("Array shift exposes an intrinsic nonconstructible method", "[core][shift]") {
    const auto method=shift();REQUIRE(method.get("name").strictEquals("shift"));REQUIRE(method.get("length").strictEquals(0));
    REQUIRE_FALSE(method.hasOwn("prototype"));REQUIRE_THROWS_AS(method.construct(),JSTypeError);
    auto empty=JSValue::array();JSPropertyDescriptor readonly;readonly.writable=false;empty.defineProperty("length",readonly);
    REQUIRE_THROWS_AS(method.call(empty),JSTypeError);
}
TEST_CASE("Array shift moves inherited values while preserving holes", "[core][shift]") {
    auto prototype=JSValue::object();prototype.set("1",7);auto object=JSValue::object(prototype);object.set("length",4);object.set("0",2);object.set("3",9);
    REQUIRE(shift().call(object).strictEquals(2));REQUIRE(object.get("0").strictEquals(7));
    REQUIRE_FALSE(object.hasOwn("1"));REQUIRE(object.get("2").strictEquals(9));REQUIRE_FALSE(object.hasOwn("3"));REQUIRE(object.get("length").strictEquals(3));
}
TEST_CASE("Array shift preserves writes and deletion before a length failure", "[core][shift]") {
    auto object=array({1,2,3});JSPropertyDescriptor readonly;readonly.writable=false;object.defineProperty("length",readonly);
    REQUIRE_THROWS_AS(shift().call(object),JSTypeError);REQUIRE(object.get("0").strictEquals(2));REQUIRE(object.get("1").strictEquals(3));
    REQUIRE_FALSE(object.hasOwn("2"));REQUIRE(object.length()==3);
}
TEST_CASE("Array shift roots its removed result during getter collection", "[core][shift][gc]") {
    auto object=array({JSValue::object(),7});JSWeakValue removed(object.get("0"));
    JSPropertyDescriptor d;d.configurable=true;d.getter=JSValue::function([](const JSValue&,const Args&,const Args& c)->JSValue{
        auto object=c[0];object.erase("0");object.collectGarbage();return 7;
    },{object});object.defineProperty("1",d);auto result=shift().call(object);
    REQUIRE_FALSE(removed.expired());REQUIRE(result.sameValue(*removed.lock()));REQUIRE(object.get("0").strictEquals(7));
    result={};object.collectGarbage();REQUIRE(removed.expired());
}
TEST_CASE("Array shift roots each source value across destination setter collection", "[core][shift][gc]") {
    auto moved=JSValue::object();moved.set("marker",42);auto object=array({0,moved});JSWeakValue weak(moved);moved={};
    JSPropertyDescriptor d;d.configurable=true;d.getter=JSValue::function([](const JSValue&,const Args&,const Args&)->JSValue{return 0;});
    auto state=JSValue::object();d.setter=JSValue::function([](const JSValue&,const Args& a,const Args& c)->JSValue{
        auto object=c[0];object.erase("1");object.collectGarbage();auto state=c[1];state.set("seen",a[0].get("marker"));return {};
    },{object,state});object.defineProperty("0",d);
    REQUIRE(shift().call(object).strictEquals(0));REQUIRE(state.get("seen").strictEquals(42));object.collectGarbage();REQUIRE(weak.expired());
}
TEST_CASE("Borrowed shift preserves TypedArray partial writes before undeletable tail", "[core][shift]") {
    auto native=Float32Array{1,2,3};auto object=NativeArray_detail::wrap(native);
    REQUIRE_THROWS_AS(shift().call(object),JSTypeError);REQUIRE(native[0]==2);REQUIRE(native[1]==3);REQUIRE(native[2]==3);
}
