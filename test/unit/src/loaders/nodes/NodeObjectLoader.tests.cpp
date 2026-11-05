#include "catch.hpp"
#include "../../../utils/parity/NodeObjectLoaderFixtures.h"
TEST_CASE("NodeObjectLoader preserves registry identity and synchronous cache lifetime", "[loaders][node-object-loader]"){
 namespace F=NodeObjectLoaderFixtures;const auto setters=F::run(F::request("setters"));for(const auto& name:setters.keys())REQUIRE(setters.get(name).strictEquals(true));
 for(const auto* variant:{"","throw","callbackThrow","reenter","reenterThrow","nodesThrow","clearThrow","superGetterThrow"}){CAPTURE(variant);const auto out=F::run(F::request("sync",variant));REQUIRE(out.get("result").strictEquals(std::string(variant)==""||std::string(variant)=="reenter"?JSValue(true):JSValue("reason")));const auto expected=std::string(variant)==""||std::string(variant)=="reenter"||std::string(variant)=="reenterThrow"||std::string(variant)=="nodesThrow"?"null":"outer";REQUIRE(out.get("cache").strictEquals(expected));}
}

TEST_CASE("NodeObjectLoader awaits intrinsically and preserves rejected and concurrent cache state", "[loaders][node-object-loader][async]") {
 namespace F=NodeObjectLoaderFixtures;
 for(const auto* variant:{"","primitive","thenable","intrinsic","reject","throw","nodesThrow","superGetterThrow","thenGetterThrow","constructorThrow","clearThrow"}){
  CAPTURE(variant);const auto out=F::run(F::request("async",variant));REQUIRE(out.get("isPromise").strictEquals(true));REQUIRE(out.get("beforeSettled").strictEquals(false));REQUIRE(out.get("same").strictEquals(true));
  const bool success=std::string(variant)==""||std::string(variant)=="primitive"||std::string(variant)=="thenable"||std::string(variant)=="intrinsic";
  REQUIRE(out.get("status").strictEquals(success?"fulfilled":"rejected"));REQUIRE(out.get("cache").strictEquals(success||std::string(variant)=="nodesThrow"?"null":"nodes"));REQUIRE(out.get("trace").toJSON()->dump().find("forbidden:then")==std::string::npos);
 }
 for(const auto* variant:{"","reverse"}){const auto out=F::run(F::request("concurrent",variant));REQUIRE(out.get("before").strictEquals("b"));REQUIRE(out.get("between").strictEquals("null"));REQUIRE(out.get("cache").strictEquals("null"));REQUIRE(out.get("trace").length()==4);}
}
TEST_CASE("NodeObjectLoader resolves real node dependencies material aliases and duplicate keys", "[loaders][node-object-loader]") {
 namespace F=NodeObjectLoaderFixtures;
 const auto nodes=F::run(F::request("nodes"));REQUIRE(nodes.get("result").get("isNode").strictEquals(true));REQUIRE(nodes.get("result").get("value").strictEquals(.625));REQUIRE(nodes.get("result").get("uuid").strictEquals("n"));REQUIRE(nodes.get("result").get("metaRemoved").strictEquals(true));
 REQUIRE(F::run(F::request("nodes","undefined")).get("trace").length()==0);REQUIRE(F::run(F::request("nodes","null")).get("result").strictEquals("TypeError"));REQUIRE(F::run(F::request("nodes","realReadonly")).get("result").strictEquals("TypeError"));
 for(const auto* variant:{"","objectKey","symbolKey","protoKey","lengthObject","snapshot"}){CAPTURE(variant);const auto out=F::run(F::request("materials",variant));const auto result=out.get("result");REQUIRE(result.get("isNodeMaterial").strictEquals(true));REQUIRE(result.get("sameNode").strictEquals(true));REQUIRE(result.get("opacity").strictEquals(std::string(variant)=="snapshot"?.25:.75));unsigned constructors=0;for(unsigned i=0;i<out.get("trace").length();++i)if(out.get("trace").get(i).strictEquals("material:construct"))++constructors;REQUIRE(constructors==(std::string(variant)=="snapshot"?1:2));if(std::string(variant)=="symbolKey")REQUIRE(result.get("symbols").strictEquals(1));if(std::string(variant)=="protoKey")REQUIRE(result.get("ordinary").strictEquals(false));}
 REQUIRE(F::run(F::request("materials","undefined")).get("trace").length()==0);REQUIRE(F::run(F::request("materials","null")).get("result").strictEquals("TypeError"));
}
TEST_CASE("NodeObjectLoader parses complete synchronous and async source scenes", "[loaders][node-object-loader][integration]") {
 namespace F=NodeObjectLoaderFixtures;
 for(const auto* variant:{"","async"}){CAPTURE(variant);const auto out=F::run(F::request("real",variant));REQUIRE_FALSE(out.hasOwn("error"));const auto scene=out.get("scene");REQUIRE(scene.get("type").strictEquals("Scene"));REQUIRE(scene.get("groupType").strictEquals("Group"));REQUIRE(scene.get("meshType").strictEquals("Mesh"));REQUIRE(scene.get("geometryType").strictEquals("PlaneGeometry"));REQUIRE(scene.get("parentIdentity").strictEquals(true));REQUIRE(scene.get("positions").length()==18);REQUIRE(scene.get("indices").length()==12);REQUIRE(scene.get("materialType").strictEquals("FixtureMaterial"));REQUIRE(scene.get("nodeUuid").strictEquals("n"));REQUIRE(scene.get("nodeValue").strictEquals(.375));REQUIRE(scene.get("opacity").strictEquals(.6));REQUIRE(out.get("metaRemoved").strictEquals(true));REQUIRE(out.get("cacheNull").strictEquals(true));if(std::string(variant)=="async"){REQUIRE(out.get("beforeCache").strictEquals(true));REQUIRE(out.get("beforeSettled").strictEquals(false));}}
}
