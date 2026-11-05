#include "catch.hpp"
#include "loaders/ObjectLoaderShapes.h"
#include "math/Vector3.h"
#include "core/JSNumber.h"
#include <limits>

namespace {
using Args=std::vector<JSValue>;
JSValue curve() { return nativeObjectLoaderCurveConstructors().get("LineCurve").getPrototype().construct(); }
JSValue call(const JSValue& value,const char* key,const Args& args={}) { return value.get(key).call(value,args); }
}
TEST_CASE("Native Curve uses overridden point methods and retains invalidatable arc caches", "[extras][Curve][runtime]") {
    auto self=curve();self.set("arcLengthDivisions",2);auto state=JSValue::object();state.set("calls",0);
    self.set("getPoint",JSValue::function(+[](const JSValue&,const Args& a,const Args& c)->JSValue {auto counter=c[0];counter.set("calls",counter.get("calls").toNumber()+1);const auto t=a[0].toNumber();return nativeVector3Constructor().construct({t*t,0,0});},{state}));
    const auto first=call(self,"getLengths");REQUIRE(first.get("1").toNumber()==.25);REQUIRE(state.get("calls").toNumber()==3);
    REQUIRE(call(self,"getLengths").sameValue(first));self.set("needsUpdate",true);const auto second=call(self,"getLengths");
    REQUIRE_FALSE(second.sameValue(first));REQUIRE(state.get("calls").toNumber()==6);REQUIRE_FALSE(self.get("needsUpdate").toBoolean());
}
TEST_CASE("Native Curve mapping and tangent methods preserve dynamic arguments and receivers", "[extras][Curve][runtime]") {
    auto self=curve(),token=JSValue::object(),target=JSValue::object();self.set("token",token);
    self.set("getUtoTmapping",JSValue::function(+[](const JSValue& receiver,const Args&,const Args&)->JSValue {return receiver.get("token");}));
    self.set("getPoint",JSValue::function(+[](const JSValue& receiver,const Args& a,const Args&)->JSValue {if(!a[0].sameValue(receiver.get("token")))throw std::logic_error("mapping identity lost");return a[1];}));
    REQUIRE(call(self,"getPointAt",{.5,target}).sameValue(target));
    REQUIRE(call(self.get("getPointAt"),"toString").toString().find("this.getUtoTmapping")!=std::string::npos);
}
TEST_CASE("Native Number constants retain binary64 values and immutable descriptors", "[core][Number][runtime]") {
    auto number=nativeNumberConstructor();
    REQUIRE(number.get("MAX_VALUE").toNumber()==std::numeric_limits<double>::max());
    REQUIRE(number.get("MIN_VALUE").toNumber()==std::numeric_limits<double>::denorm_min());
    REQUIRE(number.get("MAX_SAFE_INTEGER").toNumber()==9007199254740991.0);
    REQUIRE(number.get("MIN_SAFE_INTEGER").toNumber()==-9007199254740991.0);
    const auto descriptor=number.getOwnPropertyDescriptor("MAX_VALUE");REQUIRE(descriptor.has_value());
    REQUIRE_FALSE(descriptor->writable.value_or(true));REQUIRE_FALSE(descriptor->enumerable.value_or(true));REQUIRE_FALSE(descriptor->configurable.value_or(true));
    REQUIRE_THROWS_AS(number.set("MAX_VALUE",1),JSTypeError);REQUIRE_FALSE(number.erase("MAX_VALUE"));
}
