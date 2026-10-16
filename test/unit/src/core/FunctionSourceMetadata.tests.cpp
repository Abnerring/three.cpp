#include "catch.hpp"
#include "core/JSValue.h"
#include "core/NativeObject.h"
#include "materials/MeshBasicMaterial.h"
#include "materials/MeshStandardMaterial.h"
#include <atomic>
#include <memory>
#include <thread>
namespace {
JSValue noop(const JSValue&,const std::vector<JSValue>&,const std::vector<JSValue>&) { return {}; }
}
TEST_CASE("Function source metadata uses the original prototype toString method", "[function-source]") {
    auto function=JSValue::function(noop,{},"originalName",2);
    const auto toString=function.get("toString");
    REQUIRE(toString.call(function).string()=="function originalName() { [native code] }");
    const auto keys=function.ownPropertyKeys();
    function.setFunctionSourceText("exactMethod( /* preserved */ a, b ) { return a; }");
    REQUIRE(toString.call(function).string()=="exactMethod( /* preserved */ a, b ) { return a; }");
    const auto after=function.ownPropertyKeys();
    REQUIRE(after.size()==keys.size());
    for(std::size_t i=0;i<keys.size();++i)REQUIRE(after[i].sameValue(keys[i]));
    REQUIRE_FALSE(function.getOwnPropertyDescriptor("toString").has_value());
    REQUIRE_FALSE(function.isConstructible());
    JSPropertyDescriptor name;name.value="replacementName";function.defineProperty("name",name);
    REQUIRE(toString.call(function).string()=="exactMethod( /* preserved */ a, b ) { return a; }");
    REQUIRE_THROWS_AS(JSValue::object().setFunctionSourceText("x"),JSTypeError);
    const auto copy=function;
    function.collectGarbage();
    REQUIRE(toString.call(copy).string()=="exactMethod( /* preserved */ a, b ) { return a; }");
}
TEST_CASE("Material defaults expose faithful method names and source-backed cache key", "[function-source]") {
    auto basic=nativeMaterial(std::make_shared<MeshBasicMaterial>());
    auto standard=nativeMaterial(std::make_shared<MeshStandardMaterial>());
    const auto callback=basic.get("onBeforeCompile"),key=basic.get("customProgramCacheKey");
    REQUIRE(callback.get("name").string()=="onBeforeCompile");
    REQUIRE(key.get("name").string()=="customProgramCacheKey");
    REQUIRE(callback.get("length").number()==0);
    REQUIRE(key.get("length").number()==0);
    REQUIRE_FALSE(callback.isConstructible());
    REQUIRE_FALSE(key.isConstructible());
    REQUIRE_FALSE(callback.getOwnPropertyDescriptor("toString").has_value());
    REQUIRE_FALSE(key.getOwnPropertyDescriptor("toString").has_value());
    REQUIRE(callback.sameValue(standard.get("onBeforeCompile")));
    REQUIRE(key.sameValue(standard.get("customProgramCacheKey")));
    REQUIRE(key.call(basic).string()=="onBeforeCompile( /* shaderobject, renderer */ ) {}");
    REQUIRE(key.get("toString").call(key).string()=="customProgramCacheKey() {\r\n\r\n\t\treturn this.onBeforeCompile.toString();\r\n\r\n\t}");
}
TEST_CASE("Material cache key dynamically reads inherited function toString and arbitrary return", "[function-source]") {
    auto material=nativeMaterial(std::make_shared<MeshBasicMaterial>());
    const auto key=material.get("customProgramCacheKey"),callback=material.get("onBeforeCompile");
    auto prototype=callback.getPrototype();
    const auto old=*prototype.getOwnPropertyDescriptor("toString");
    const auto result=JSValue::object();
    auto replacement=JSValue::function([](const JSValue& receiver,const std::vector<JSValue>& args,const std::vector<JSValue>& captures){
        if(!receiver.sameValue(captures[0]) || !args.empty())throw JSTypeError("Wrong dynamic receiver");
        return captures[1];
    },{callback,result});
    prototype.set("toString",replacement);
    REQUIRE(key.call(material).sameValue(result));
    prototype.defineProperty("toString",old);
    REQUIRE(key.call(material).string()=="onBeforeCompile( /* shaderobject, renderer */ ) {}");
}
TEST_CASE("Function source metadata rejects a foreign thread before mutation", "[function-source]") {
    auto function=JSValue::function(noop);
    function.setFunctionSourceText("before() {}");
    std::atomic<bool> rejected=false;
    std::thread other([&]{try {function.setFunctionSourceText("wrong() {}");}catch(const JSTypeError&){rejected=true;}});
    other.join();
    REQUIRE(rejected);
    REQUIRE(function.nativeFunctionSource()=="before() {}");
}
