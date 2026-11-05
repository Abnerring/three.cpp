#include "catch.hpp"
#include "../../utils/parity/ObjectLoaderShapeJSONFixtures.h"

namespace {
using namespace ObjectLoaderShapeJSONFixtures;
JSValue observable(const char* variant){auto request=object();request.set("op","observable");request.set("variant",variant);return run(request);}
std::string json(const JSValue& value){return value.toJSON()->dump();}
}

TEST_CASE("Managed curve JSON methods preserve all thirteen sources and Arc inheritance", "[loaders][shape][json][native]"){
    const auto entries=metadata().get("entries");REQUIRE(entries.length()==14);
    for(double i=0;i<entries.length();++i){const auto row=entries.get(JSValue(i));INFO(row.get("name").string());REQUIRE(row.get("length").strictEquals(0));REQUIRE_FALSE(row.get("constructible").toBoolean());REQUIRE(row.get("source").string().starts_with("toJSON()"));REQUIRE(row.get("own").toBoolean()!=row.get("name").strictEquals("ArcCurve"));REQUIRE(json(row.get("flags"))=="[true,false,true]");}
}

TEST_CASE("Managed concrete curves return full default JSON metadata and vector components", "[loaders][shape][json][native]"){
    const auto ctors=constructors();
    for(const auto& name:names()){
        INFO(name);auto self=ctors.get(name).construct();if(name=="Shape")self.set("uuid","shape");const auto result=call(self,"toJSON");
        REQUIRE(result.get("metadata").get("version").strictEquals(4.7));REQUIRE(result.get("metadata").get("generator").strictEquals("Curve.toJSON"));REQUIRE(result.get("arcLengthDivisions").strictEquals(200));REQUIRE(result.get("type").strictEquals(name));
        if(name=="LineCurve"||name=="LineCurve3")REQUIRE(result.get("v1").length()==(name=="LineCurve"?2:3));
        if(name=="Shape"){REQUIRE(result.get("holes").length()==0);REQUIRE(result.get("uuid").strictEquals("shape"));}
    }
}

TEST_CASE("Managed curve JSON preserves raw getter values and the original super return identity", "[loaders][shape][json][native]"){
    const auto raw=observable("rawGetters"),result=raw.get("value").get("result"),token=raw.get("value").get("token");
    REQUIRE(raw.get("error").isUndefined());REQUIRE(json(raw.get("events"))=="[\"get:arcLengthDivisions\",\"get:type\"]");
    REQUIRE(result.get("type").get("$ref").strictEquals(result.get("arcLengthDivisions").get("$id")));REQUIRE(token.get("$ref").strictEquals(result.get("arcLengthDivisions").get("$id")));
    for(const char* variant:{"superAlias","homeObject"}){
        const auto value=observable(variant);REQUIRE(value.get("error").isUndefined());REQUIRE(json(value.get("events"))=="[[\"get:super\",true],[\"call:super\",true]]");REQUIRE(value.get("value").get("token").get("$ref").strictEquals(value.get("value").get("result").get("$id")));
    }
    REQUIRE(observable("constructorIgnored").get("error").isUndefined());
}

TEST_CASE("Managed curve JSON evaluates method receivers and reentrant vector callbacks in source order", "[loaders][shape][json][native]"){
    const auto result=observable("vectorReenter");REQUIRE(result.get("error").isUndefined());
    REQUIRE(json(result.get("events"))=="[\"get:v1\",\"get:v1:toArray\",\"call:v1:toArray\",[\"reenter\",\"inner\",9],\"get:v2\",\"get:v2:toArray\",\"call:v2:toArray\"]");
    const auto failure=observable("vectorGetThrow");REQUIRE(failure.get("error").get("name").strictEquals("ThrownValue"));REQUIRE(failure.get("error").get("raw").strictEquals("toArray:v2"));
    for(const char* variant:{"superNull","superUndefined"}){const auto value=observable(variant);REQUIRE(value.get("error").get("name").strictEquals("TypeError"));REQUIRE(json(value.get("events"))=="[\"get:v1\",\"get:v1:toArray\",\"call:v1:toArray\"]");}
}

TEST_CASE("Managed shape curve and point arrays resolve push before child callbacks and coerce cached length each iteration", "[loaders][shape][json][native]"){
    for(const char* variant:{"pushPoints","pushCurves","pushHoles","pushReenter"}){
        INFO(variant);const auto value=observable(variant);REQUIRE(value.get("error").isUndefined());const auto events=value.get("events");
        REQUIRE(events.get("2").strictEquals("get:length"));REQUIRE(events.get("3").strictEquals("coerce:length"));REQUIRE(events.get("6").strictEquals("get:push"));REQUIRE(events.get(JSValue(events.length()-1)).strictEquals("coerce:length"));REQUIRE(value.get("value").get("token").get("$ref").strictEquals(value.get("value").get("result").get("$id")));
    }
    const auto pushFailure=observable("pushGetThrow");REQUIRE(pushFailure.get("error").get("raw").strictEquals("push"));REQUIRE(pushFailure.get("events").length()==7);
    const auto lengthFailure=observable("lengthThrow");REQUIRE(lengthFailure.get("error").get("raw").strictEquals("length"));REQUIRE(lengthFailure.get("events").length()==4);
    const auto childFailure=observable("childGetThrow");REQUIRE(childFailure.get("error").get("raw").strictEquals("child"));REQUIRE(childFailure.get("events").length()==8);
}

TEST_CASE("Managed shape serialization recursively retains holes and nondefault child geometry", "[loaders][shape][json][native]"){
    const auto ctors=constructors();auto shape=ctors.get("Shape").construct(),hole=ctors.get("Path").construct(),curve=ctors.get("LineCurve").construct();
    shape.set("uuid","shape");shape.set("arcLengthDivisions",13);hole.set("autoClose",true);call(curve.get("v1"),"set",{1,2});call(curve.get("v2"),"set",{3,4});push(hole.get("curves"),curve);push(shape.get("holes"),hole);
    const auto result=call(shape,"toJSON"),serializedHole=result.get("holes").get("0");REQUIRE(result.get("arcLengthDivisions").strictEquals(13));REQUIRE(serializedHole.get("type").strictEquals("Path"));REQUIRE(serializedHole.get("autoClose").strictEquals(true));REQUIRE(json(serializedHole.get("curves").get("0").get("v1"))=="[1,2]");REQUIRE(json(serializedHole.get("curves").get("0").get("v2"))=="[3,4]");
}
