#include "catch.hpp"
#include "../../utils/parity/ObjectLoaderNativeFixtures.h"
#include "animation/AnimationUtils.h"

TEST_CASE("ObjectLoader managed parsing retains source object and resource identities", "[loaders][object-loader][managed]") {
    namespace F=ObjectLoaderNativeFixtures;
    const auto loader=nativeObjectLoaderConstructor().construct();
    const auto json=Animation_detail::parseJSON(R"({"object":{"type":"Scene","uuid":"root","children":[{"type":"Group","uuid":"group","children":[{"type":"Mesh","uuid":"one","geometry":"geometry","material":"material"},{"type":"Mesh","uuid":"two","geometry":"geometry","material":"material"}]}]},"geometries":[{"uuid":"geometry","type":"BufferGeometry","data":{"attributes":{"position":{"type":"Float32Array","itemSize":3,"array":[0,0,0,1,0,0,0,1,0]}}}}],"materials":[{"uuid":"material","type":"MeshBasicMaterial","color":16711680}]})");
    auto state=F::object();state.set("calls",0);
    const auto callback=JSValue::function(+[](const JSValue& self,const F::Args& a,const F::Args& c)->JSValue {auto state=c[0];state.set("receiver",self.isUndefined());state.set("object",a[0]);state.set("calls",state.get("calls").toNumber()+1);return {};},{state});
    const auto root=F::call(loader,"parse",{json,callback});
    REQUIRE(root.get("isScene").strictEquals(true));REQUIRE(state.get("calls").strictEquals(1));REQUIRE(state.get("receiver").toBoolean());REQUIRE(state.get("object").sameValue(root));
    const auto group=root.get("children").get(JSValue(0));REQUIRE(group.get("parent").sameValue(root));
    const auto first=group.get("children").get(JSValue(0)),second=group.get("children").get(JSValue(1));
    REQUIRE(first.get("parent").sameValue(group));REQUIRE(first.get("geometry").sameValue(second.get("geometry")));REQUIRE(first.get("material").sameValue(second.get("material")));
    REQUIRE(first.get("geometry").get("attributes").get("position").get("array").get("length").strictEquals(9));
    const auto repeated=F::call(loader,"parse",{json});REQUIRE_FALSE(repeated.sameValue(root));
    REQUIRE_FALSE(repeated.get("children").get(JSValue(0)).get("children").get(JSValue(0)).get("geometry").sameValue(first.get("geometry")));
}

TEST_CASE("ObjectLoader parseAsync suspends and rejects through intrinsic async functions", "[loaders][object-loader][managed][async]") {
    namespace F=ObjectLoaderNativeFixtures;
    const auto ctor=nativeObjectLoaderConstructor(),loader=ctor.construct();
    REQUIRE(ctor.get("prototype").get("parseAsync").getPrototype().sameValue(nativeAsyncFunctionPrototype()));
    REQUIRE_FALSE(ctor.get("prototype").get("parseAsync").isConstructible());
    const auto json=Animation_detail::parseJSON(R"({"object":{"type":"Group","uuid":"async","children":[{"type":"Bone","uuid":"bone"}]}})");
    const auto promise=F::call(loader,"parseAsync",{json});REQUIRE(isNativePromise(promise));
    const auto object=F::await(promise);REQUIRE(object.get("uuid").strictEquals("async"));REQUIRE(object.get("children").get(JSValue(0)).get("parent").sameValue(object));
    auto reason=F::object();reason.set("tag","reject");
    auto overridden=loader;overridden.set("parseImagesAsync",JSValue::function(+[](const JSValue&,const F::Args&,const F::Args& c)->JSValue {return rejectNativePromise(c[0]);},{reason}));
    bool rejected=false;try {F::await(F::call(loader,"parseAsync",{json}));}catch(const JSThrownValue& error){rejected=true;REQUIRE(error.value.sameValue(reason));}REQUIRE(rejected);
}

TEST_CASE("ObjectLoader dispatch observes getters reentry and image callback temporal dead zone", "[loaders][object-loader][managed]") {
    namespace F=ObjectLoaderNativeFixtures;
    for(const auto* variant:{"normal","reenter","getterThrow","syncImages"}) {
        CAPTURE(variant);auto request=F::object();request.set("op","dispatch");request.set("variant",variant);if(std::string(variant)=="syncImages")request.set("onLoad",true);
        const auto result=F::run(request);
        if(std::string(variant)=="syncImages")REQUIRE(result.get("error").get("name").strictEquals("ReferenceError"));
        else if(std::string(variant)=="getterThrow")REQUIRE(result.get("error").get("raw").strictEquals("geometries"));
        else {REQUIRE(result.get("value").get("tag").strictEquals("outer"));REQUIRE(result.get("events").get("length").toNumber()>(std::string(variant)=="reenter"?40:20));}
    }
}

TEST_CASE("ObjectLoader inline texture sources retain shared typed pixels and version semantics", "[loaders][object-loader][managed][textures]") {
    namespace F=ObjectLoaderNativeFixtures;
    const auto loader=nativeObjectLoaderConstructor().construct();
    const auto images=Animation_detail::parseJSON(R"([{"uuid":"source","url":{"type":"Uint8Array","data":[255,0,0,255],"width":1,"height":1}}])");
    const auto sources=F::call(loader,"parseImages",{images});
    const auto textures=F::call(loader,"parseTextures",{Animation_detail::parseJSON(R"([{"uuid":"one","image":"source","offset":[0.25,0.5],"wrap":[1000,1002]},{"uuid":"two","image":"source"}])"),sources});
    const auto first=textures.get("one"),second=textures.get("two");
    REQUIRE(first.get("isDataTexture").strictEquals(true));REQUIRE(first.get("source").sameValue(second.get("source")));REQUIRE(first.get("source").sameValue(sources.get("source")));
    REQUIRE(first.get("image").get("data").instanceOf(nativeGlobalBinding("Uint8Array")));REQUIRE(first.get("image").get("data").get("0").strictEquals(255));
    REQUIRE(first.get("version").strictEquals(1));REQUIRE(first.get("offset").get("x").strictEquals(0.25));REQUIRE(first.get("wrapS").strictEquals(1000));REQUIRE(first.get("wrapT").strictEquals(1002));
}

TEST_CASE("ObjectLoader shape deserialization executes curve and path source layers", "[loaders][object-loader][managed][shapes]") {
    namespace F=ObjectLoaderNativeFixtures;
    const auto loader=nativeObjectLoaderConstructor().construct();
    const auto json=Animation_detail::parseJSON(R"([{"uuid":"triangle","arcLengthDivisions":200,"autoClose":false,"currentPoint":[0,0],"holes":[],"curves":[{"type":"LineCurve","arcLengthDivisions":200,"v1":[0,0],"v2":[1,0]},{"type":"LineCurve","arcLengthDivisions":200,"v1":[1,0],"v2":[0,1]},{"type":"LineCurve","arcLengthDivisions":200,"v1":[0,1],"v2":[0,0]}]}])");
    const auto shape=F::call(loader,"parseShapes",{json}).get("triangle");REQUIRE(shape.get("uuid").strictEquals("triangle"));REQUIRE(shape.get("curves").get("length").strictEquals(3));
    const auto points=F::call(shape,"extractPoints",{4});REQUIRE(points.get("holes").get("length").strictEquals(0));REQUIRE(points.get("shape").get("length").strictEquals(4));
    REQUIRE(points.get("shape").get(JSValue(1)).get("x").strictEquals(1));REQUIRE(points.get("shape").get(JSValue(2)).get("y").strictEquals(1));
}

TEST_CASE("ObjectLoader custom geometry dispatch preserves getters reentry and assignment errors", "[loaders][object-loader][managed]") {
    namespace F=ObjectLoaderNativeFixtures;
    for(const auto* variant:{"custom","reenter","unknown","throw"}) {
        CAPTURE(variant);auto request=F::object();request.set("op","geometryDispatch");request.set("variant",variant);request.set("uuid","outer");
        const auto result=F::run(request),events=result.get("events");
        if(std::string(variant)=="unknown") {
            REQUIRE(result.get("error").get("name").strictEquals("TypeError"));
            const auto last=events.get(JSValue(events.length()-1));REQUIRE(last.get(JSValue(0)).strictEquals("get"));REQUIRE(last.get(JSValue(2)).strictEquals("uuid"));
        } else if(std::string(variant)=="throw")REQUIRE(result.get("error").get("raw").strictEquals("custom geometry"));
        else {
            REQUIRE(result.get("value").get("outer").get("marker").strictEquals("outer"));
            REQUIRE(result.get("value").get("outer").get("userData").get("payload").strictEquals(17));
            bool reentered=false;for(unsigned i=0;i<events.length();++i) {const auto event=events.get(JSValue(i));if(event.get(JSValue(0)).strictEquals("reenter"))reentered=true;}
            REQUIRE(reentered==(std::string(variant)=="reenter"));
        }
    }
}

TEST_CASE("ObjectLoader material cache converts assignment keys after parse and right hand lookups", "[loaders][object-loader][managed]") {
    namespace F=ObjectLoaderNativeFixtures;
    for(const auto* variant:{"normal","parseThrow","keyThrow"}) {
        CAPTURE(variant);auto request=F::object();request.set("op","materialDispatch");request.set("variant",variant);
        const auto result=F::run(request);const auto events=result.get("events").toJSON()->dump();
        if(std::string(variant)=="normal") {
            REQUIRE(result.get("value").get("material").get("marker").strictEquals(17));
            REQUIRE(events==R"(["uuid",["key",1],"uuid","get:parse",["parse",true],["key",2],"uuid","uuid",["key",3],["key",4]])");
        } else {
            REQUIRE(result.get("error").get("raw").strictEquals(std::string(variant)=="parseThrow"?"material parse":"material key"));
            REQUIRE(events==(std::string(variant)=="parseThrow"?R"(["uuid",["key",1],"uuid","get:parse",["parse",true]])":R"(["uuid",["key",1],"uuid","get:parse",["parse",true],["key",2]])"));
        }
    }
}
