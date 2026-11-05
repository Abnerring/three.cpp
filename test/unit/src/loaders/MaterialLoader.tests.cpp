#include "catch.hpp"
#include "../../utils/parity/LoaderSemanticsFixtures.h"

TEST_CASE("MaterialLoader performs dynamic material and texture lookup in source order", "[loaders][material-loader]") {
    namespace F=LoaderSemanticsFixtures;
    const auto success=F::run(F::request("parse"));REQUIRE(success.get("result").strictEquals(true));REQUIRE(success.get("trace").toJSON()->dump()==R"(["get:create","get:type",["create",true,"T"],"get:fromJSON","get:textures",["fromJSON",true,true,true]])");
    const auto missing=F::run(F::request("parse","missing"));REQUIRE(missing.get("result").strictEquals("TypeError"));REQUIRE(missing.get("trace").toJSON()->dump()==R"(["get:create","get:type",["create",true,"T"],"get:textures"])");
    REQUIRE(F::run(F::request("parse","throw")).get("result").strictEquals("reason"));
    const auto dynamic=F::run(F::request("factory","static"));REQUIRE(dynamic.get("same").toBoolean());REQUIRE(dynamic.get("trace").toJSON()->dump()==R"([["get:static",true],["call:static",true,"custom"]])");
    const auto symbol=F::run(F::request("factory","symbol"));REQUIRE(symbol.get("registerUndefined").toBoolean());REQUIRE(symbol.get("same").toBoolean());
}
TEST_CASE("Material and node loaders configure real FileLoader calls and preserve callback errors", "[loaders][material-loader][node-loader]") {
    namespace F=LoaderSemanticsFixtures;
    for(const auto* module:{"MaterialLoader","NodeLoader","NodeMaterialLoader"})for(const auto* variant:{"","parseThrow","onLoadThrow","onErrorThrow","reenter"}){
        CAPTURE(module,variant);const auto result=F::run(F::request("load",variant,module));
        REQUIRE(result.get("result").strictEquals(std::string(variant)=="onErrorThrow"?JSValue("reason"):JSValue(true)));
        const auto trace=result.get("trace");REQUIRE(trace.get("0").toJSON()->dump()==R"(["file.load","asset.json","base/",true,true,true,true])");
        REQUIRE(result.get("callback").get("callbackName").strictEquals(""));REQUIRE(result.get("callback").get("callbackLength").strictEquals(1));
        unsigned errors=0,nested=0;for(unsigned i=0;i<trace.length();++i){const auto row=trace.get(std::to_string(i));if(row.type()!=JSValue::Type::Array)continue;if(row.get("0").strictEquals("onError")){++errors;REQUIRE(row.get("1").strictEquals(true));REQUIRE(row.get("2").strictEquals(true));}if(row.get("0").strictEquals("file.load")&&row.get("1").strictEquals("nested.json"))++nested;}
        REQUIRE(errors==((std::string(variant)==""||std::string(variant)=="reenter")?0:1));REQUIRE(nested==(std::string(variant)=="reenter"?1:0));
        if(std::string(variant)=="onErrorThrow")REQUIRE(trace.toJSON()->dump().find("itemError")==std::string::npos);
    }
}

#include "loaders/Cache.h"
#include "loaders/FileLoader.h"

TEST_CASE("MaterialLoader loads real managed materials through the configured native transport", "[loaders][material-loader][fileloader]") {
    namespace F=LoaderSemanticsFixtures;using Args=std::vector<JSValue>;
    const auto previousTransport=FileLoader::transport(),previousEnabled=Cache.enabled.value(),previousFiles=Cache.files.value();
    struct RestoreTransport{JSValue transport,enabled,files;~RestoreTransport(){FileLoader::setTransport(transport);Cache.enabled=enabled;Cache.files=files;}}restore{previousTransport,previousEnabled,previousFiles};
    Cache.clear();Cache.enabled=false;
    auto state=F::object();state.set("loads",0);
    FileLoader::setTransport(JSValue::function(+[](const JSValue&,const Args& args,const Args& c)->JSValue{
        auto state=c[0];state.set("request",args[0]);auto response=F::object();response.set("status",200);response.set("body",R"({"type":"MeshBasicMaterial","uuid":"loaded-material","opacity":0.25,"color":16711680})");return response;
    },{state}));
    const auto loaded=JSValue::function(+[](const JSValue& self,const Args& args,const Args& c)->JSValue{
        auto state=c[0];state.set("receiverUndefined",self.isUndefined());state.set("material",args[0]);state.set("loads",state.get("loads").toNumber()+1);return {};
    },{state});
    MaterialLoader loader;loader.setPath("materials/").setWithCredentials(true);
    REQUIRE(loader.load("basic.json",loaded).isUndefined());
    REQUIRE(state.get("loads").strictEquals(1));REQUIRE(state.get("receiverUndefined").toBoolean());
    REQUIRE(state.get("request").get("url").strictEquals("materials/basic.json"));REQUIRE(state.get("request").get("credentials").strictEquals("include"));
    const auto material=state.get("material");REQUIRE(material.get("isMeshBasicMaterial").strictEquals(true));REQUIRE(material.get("uuid").strictEquals("loaded-material"));REQUIRE(material.get("opacity").strictEquals(.25));REQUIRE(material.get("color").get("r").strictEquals(1));REQUIRE(material.get("color").get("g").strictEquals(0));
}
