#include "catch.hpp"
#include "../../utils/parity/WebGLRendererSceneFixtures.h"

namespace {
using namespace WebGLRendererSceneFixtures;
JSValue request(){return JSValue::object();}
unsigned eventsNamed(const JSValue& result,const char* name){unsigned count=0;const auto events=result.get("events");for(double i=0;i<events.length();++i){const auto value=events.get(JSValue(i));if(value.type()==JSValue::Type::Array&&value.get("0").strictEquals(name))++count;}return count;}
}

TEST_CASE("Native WebGL scene functions preserve original anonymous render metadata", "[renderers][WebGLRendererScene][runtime]") {
    auto input=request();input.set("op","metadata");const auto result=run(input);
    REQUIRE(result.length()==7);REQUIRE(result.get("0").get("name").strictEquals(""));REQUIRE(result.get("0").get("length").strictEquals(2));REQUIRE(result.get("0").get("constructible").strictEquals(true));
    REQUIRE(result.get("5").get("name").strictEquals("renderObject"));REQUIRE(result.get("5").get("length").strictEquals(6));REQUIRE(result.get("6").get("name").strictEquals("WebGLRenderTarget"));REQUIRE(result.get("6").get("length").strictEquals(0));
}

TEST_CASE("Native WebGL scene callbacks keep original raw argument identities", "[renderers][WebGLRendererScene][runtime]") {
    auto input=request();input.set("groups",true);input.set("override",true);const auto result=run(input);
    REQUIRE(result.get("error").isUndefined());unsigned identities=0;const auto events=result.get("events");
    for(double i=0;i<events.length();++i){const auto value=events.get(JSValue(i));if(value.type()!=JSValue::Type::Array||!value.get("0").strictEquals("arg-identities"))continue;const auto flags=value.get("2");for(double j=0;j<flags.length();++j){const auto flag=flags.get(JSValue(j));if(flag.type()==JSValue::Type::Boolean){REQUIRE(flag.toBoolean());++identities;}}}
    REQUIRE(identities>=20);REQUIRE(eventsNamed(result,"draw")==2);REQUIRE(result.get("state").get("renderListDepth").strictEquals(0));
}

TEST_CASE("Native WebGL scene draws transparent double sides and restores source state", "[renderers][WebGLRendererScene][runtime]") {
    auto input=request();input.set("transparent",true);input.set("side",2);const auto result=run(input);
    REQUIRE(result.get("error").isUndefined());REQUIRE(eventsNamed(result,"draw")==3);REQUIRE(result.get("state").get("materialSide").strictEquals(2));REQUIRE(result.get("state").get("materialVersion").strictEquals(2));REQUIRE(result.get("state").get("renderStateDepth").strictEquals(0));
}

TEST_CASE("Native WebGL scene retains unfinished source stacks and side after backend failure", "[renderers][WebGLRendererScene][runtime]") {
    auto input=request();input.set("transparent",true);input.set("side",2);input.set("drawThrow",true);input.set("throwMesh",true);const auto result=run(input);
    REQUIRE(result.get("error").get("raw").strictEquals("GPU:unsupported"));REQUIRE(result.get("state").get("materialSide").strictEquals(1));REQUIRE(result.get("state").get("materialVersion").strictEquals(1));REQUIRE(result.get("state").get("renderListDepth").strictEquals(1));REQUIRE(result.get("state").get("renderStateDepth").strictEquals(1));REQUIRE_FALSE(result.get("state").get("currentStateNull").toBoolean());
}

TEST_CASE("Native WebGL scene callback reentry isolates depth and restores outer stacks", "[renderers][WebGLRendererScene][runtime]") {
    auto input=request();input.set("reentry","object.before:mesh");input.set("clipping",true);const auto result=run(input);
    REQUIRE(result.get("error").isUndefined());REQUIRE(result.get("state").get("frame").strictEquals(2));REQUIRE(result.get("state").get("renderListDepth").strictEquals(0));REQUIRE(result.get("state").get("renderStateDepth").strictEquals(0));REQUIRE(result.get("state").get("currentListNull").strictEquals(true));
    auto depths=list();const auto events=result.get("events");for(double i=0;i<events.length();++i){const auto event=events.get(JSValue(i));if(event.type()==JSValue::Type::Array&&event.get("0").strictEquals("renderLists.get"))push(depths,event.get("3"));}
    REQUIRE(depths.length()==2);REQUIRE(depths.get("0").strictEquals(0));REQUIRE(depths.get("1").strictEquals(1));
}

TEST_CASE("Native WebGL scene visibility uses the original strict false check", "[renderers][WebGLRendererScene][runtime]") {
    auto input=request();input.set("visible",0);input.set("visibleGetter",true);const auto zero=run(input);REQUIRE(zero.get("error").isUndefined());REQUIRE(eventsNamed(zero,"draw")==2);
    input.set("visible",false);const auto hidden=run(input);REQUIRE(hidden.get("error").isUndefined());REQUIRE(eventsNamed(hidden,"draw")==1);
}
