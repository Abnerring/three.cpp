#include "catch.hpp"
#include "../../utils/parity/WebGLRendererProgramFixtures.h"
#include "animation/AnimationUtils.h"
namespace {
using namespace WebGLRendererProgramFixtures;
JSValue request(const char* text){return Animation_detail::parseJSON(text);}
unsigned eventsNamed(JSValue result,const char* label){unsigned count=0;const auto events=result.get("events");for(double i=0;i<events.length();++i){const auto e=events.get(JSValue(i));if(e.type()==JSValue::Type::Array&&e.get("0").strictEquals(label))++count;}return count;}
}
TEST_CASE("Native renderer program functions retain pinned metadata", "[renderers][WebGLRendererProgram][runtime]") {
    const auto result=run(request(R"({"op":"metadata"})"));REQUIRE(result.length()==11);REQUIRE(result.get("5").get("name").strictEquals("setProgram"));REQUIRE(result.get("5").get("length").strictEquals(5));
    for(double i=0;i<result.length();++i){const auto item=result.get(JSValue(i));REQUIRE(item.get("constructible").strictEquals(true));REQUIRE(item.get("source").toString().starts_with("function "));}
}
TEST_CASE("Native renderer retains common uniforms for consecutive shared material draws", "[renderers][WebGLRendererProgram][runtime]") {
    const auto result=run(request(R"({"steps":[{},{"op":"color","value":[0,1,0]},{},{"op":"list"}]})"));REQUIRE(eventsNamed(result,"materials.refresh")==1);REQUIRE(eventsNamed(result,"material.compile")==1);REQUIRE(result.get("state").get("diffuse").get("0").strictEquals(1));REQUIRE(result.get("state").get("diffuse").get("1").strictEquals(0));REQUIRE(result.get("state").get("uniformAlias").strictEquals(true));REQUIRE(result.get("state").get("clippingAlias").strictEquals(true));
}
TEST_CASE("Native renderer camera switches refresh shared material uniforms", "[renderers][WebGLRendererProgram][runtime]") {
    const auto result=run(request(R"({"steps":[{},{"op":"color","value":[0,1,0]},{"camera":"camera2"}]})"));REQUIRE(eventsNamed(result,"materials.refresh")==2);REQUIRE(result.get("state").get("diffuse").get("0").strictEquals(0));REQUIRE(result.get("state").get("diffuse").get("1").strictEquals(1));REQUIRE(result.get("state").get("currentCamera").strictEquals("camera2"));
}
TEST_CASE("Native renderer material version and custom key control compilation", "[renderers][WebGLRendererProgram][runtime]") {
    const auto same=run(request(R"({"steps":[{},{"op":"needUpdate","value":true},{}]})"));REQUIRE(eventsNamed(same,"material.compile")==1);REQUIRE(same.get("state").get("version").strictEquals(1));
    const auto changed=run(request(R"({"steps":[{},{"op":"key","value":"new"},{"op":"needUpdate","value":true},{}]})"));REQUIRE(eventsNamed(changed,"material.compile")==2);REQUIRE(changed.get("state").get("programs").length()==2);
}
TEST_CASE("Native renderer compile failure retains source private version and unfinished cache", "[renderers][WebGLRendererProgram][runtime]") {
    const auto result=run(request(R"({"throwAt":"material.compile"})"));REQUIRE(result.get("results").get("0").get("error").get("raw").strictEquals("material.compile"));REQUIRE(result.get("state").get("version").strictEquals(0));REQUIRE(result.get("state").get("programs").length()==0);REQUIRE(result.get("state").get("currentCamera").isNull());REQUIRE(eventsNamed(result,"cache.acquire")==0);
}
TEST_CASE("Native renderer shader forced uniforms preserve original aliases and clear update flag", "[renderers][WebGLRendererProgram][runtime]") {
    const auto result=run(request(R"({"shader":true,"steps":[{},{"op":"uniform","key":"customScalar","value":0.75},{"op":"set","key":"uniformsNeedUpdate","value":true},{}]})"));REQUIRE(result.get("state").get("shaderAlias").strictEquals(true));REQUIRE(result.get("state").get("uniformsNeedUpdate").strictEquals(false));REQUIRE(result.get("state").get("uploads").get("customScalar").strictEquals(.75));
}
