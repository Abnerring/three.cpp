#include "catch.hpp"
#include "core/JSString.h"
#include "core/JSIterator.h"
#include <limits>
namespace {using Args=std::vector<JSValue>;JSValue prefix(const JSValue& value,const JSValue& search,const JSValue& position={}){return nativeStringPrototype().get("startsWith").call(value,{search,position});}}
TEST_CASE("Native string objects retain StringData and UTF16 indexed descriptors", "[string]"){
    auto value=createNativeStringObject("A\xF0\x9F\x98\x80Z"),prototype=nativeStringPrototype();
    REQUIRE(isNativeStringObject(value));REQUIRE(value.get("length").strictEquals(4));REQUIRE(value.get("0").strictEquals("A"));REQUIRE(value.get("3").strictEquals("Z"));
    REQUIRE(value.getOwnPropertyDescriptor("1")->writable==false);REQUIRE(value.getOwnPropertyDescriptor("1")->enumerable==true);REQUIRE(value.getOwnPropertyDescriptor("1")->configurable==false);
    REQUIRE(value.getOwnPropertyDescriptor("length")->enumerable==false);REQUIRE_THROWS_AS(value.set("1","x"),JSTypeError);
    REQUIRE(value.get("valueOf").call(value).strictEquals("A\xF0\x9F\x98\x80Z"));REQUIRE(prototype.get("toString").call(prototype).strictEquals(""));
    REQUIRE(JSValue::object().get("toString").call(value).strictEquals("[object String]"));
    REQUIRE_THROWS_AS(prototype.get("valueOf").call(JSValue::object()),JSTypeError);
}
TEST_CASE("startsWith uses UTF16 positions and ToIntegerOrInfinity", "[string]"){
    REQUIRE(prefix("A\xF0\x9F\x98\x80Z","Z",3).strictEquals(true));REQUIRE(prefix("A\xF0\x9F\x98\x80Z","\xED\xB8\x80",2).strictEquals(true));
    REQUIRE(prefix("abc","a",-3).strictEquals(true));REQUIRE(prefix("abc","b",1.9).strictEquals(true));REQUIRE(prefix("abc","a",std::numeric_limits<double>::quiet_NaN()).strictEquals(true));
    REQUIRE(prefix("abc","",std::numeric_limits<double>::infinity()).strictEquals(true));REQUIRE(prefix("abc","c",std::numeric_limits<double>::infinity()).strictEquals(false));
    REQUIRE(prefix("undefined",JSValue()).strictEquals(true));REQUIRE_THROWS_AS(prefix(nullptr,""),JSTypeError);
    const auto method=nativeStringPrototype().get("startsWith");REQUIRE_FALSE(method.isConstructible());REQUIRE(method.get("name").strictEquals("startsWith"));REQUIRE(method.get("length").strictEquals(1));
}
TEST_CASE("startsWith observes real Symbol match before search and position conversion", "[string][symbols]"){
    auto trace=JSValue::array(),self=JSValue::object(),search=JSValue::object(),position=JSValue::object();
    auto converter=[](JSValue trace,const char* name,JSValue result){return JSValue::function(+[](const JSValue&,const Args& a,const Args& c)->JSValue {auto t=c[0];t.get("push").call(t,{c[1],a[0]});t.collectGarbage();return c[2];},{trace,name,result});};
    self.setPrimitiveConverter(converter(trace,"self","abc"));search.setPrimitiveConverter(converter(trace,"search","b"));position.setPrimitiveConverter(converter(trace,"position",1));
    JSPropertyDescriptor d;d.getter=JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue{auto t=c[0];t.get("push").call(t,{"match"});return false;},{trace});d.configurable=true;
    search.defineProperty(JSValue::wellKnownSymbol("match"),d);
    REQUIRE(prefix(self,search,position).strictEquals(true));REQUIRE(trace.get("join").call(trace,{","}).strictEquals("self,string,match,search,string,position,number"));
    trace.set("length",0);search.erase(JSValue::wellKnownSymbol("match"));search.set(JSValue::wellKnownSymbol("match"),true);
    REQUIRE_THROWS_AS(prefix(self,search,position),JSTypeError);REQUIRE(trace.get("join").call(trace,{","}).strictEquals("self,string"));
}
