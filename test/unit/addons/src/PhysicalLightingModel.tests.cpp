#include "nodes/functions/PhysicalLightingModel.h"
#include "nodes/core/LightingModel.h"
#include "nodes/core/NodeBuilder.h"
#include "nodes/core/StackNode.h"
#include "nodes/tsl/TSLBase.h"
#include "../../utils/parity/PhysicalLightingModelScenarios.h"
#include "catch.hpp"

namespace {
using namespace PhysicalLightingScenarios;
JSValue generated(const JSValue& result,const char* output="float") {
    auto renderer=object(),debug=object(),diagnostics=object();diagnostics.set("keywords",false);debug.set("diagnostics",diagnostics);renderer.set("debug",debug);renderer.set("coordinateSystem",2001);
    auto b=nativeNodeBuilderConstructor().construct({nullptr,renderer,nullptr});b.set("shaderStage","fragment");JSValue snippet;
    for(const auto* stage:{"setup","analyze","generate"}){b.set("buildStage",stage);snippet=call(result,"build",{b,output});}
    return snippet;
}
}

TEST_CASE("PhysicalLightingModel preserves raw feature values and constructor order") {
    const auto custom=object(),ctor=nativePhysicalLightingModelConstructor();
    const auto model=ctor.construct({JSValue{},nullptr,0,1,"",custom,false,true});
    REQUIRE(model.get("clearcoat").strictEquals(false));
    REQUIRE(model.get("sheen").isNull());
    REQUIRE(model.get("iridescence").strictEquals(0));
    REQUIRE(model.get("anisotropy").strictEquals(1));
    REQUIRE(model.get("transmission").strictEquals(""));
    REQUIRE(model.get("dispersion").sameValue(custom));
    REQUIRE(model.get("retroreflection").strictEquals(false));
    REQUIRE(model.get("diffuseRoughness").strictEquals(true));
    REQUIRE(ctor.get("length").strictEquals(0));
    REQUIRE(ctor.get("prototype").get("computeMultiscattering").get("length").strictEquals(4));
    REQUIRE(ctor.getPrototype().sameValue(nativeLightingModelConstructor()));
    REQUIRE(ctor.get("prototype").getPrototype().sameValue(nativeLightingModelConstructor().get("prototype")));
    const std::vector<std::string> expected={"clearcoat","sheen","iridescence","anisotropy","transmission","dispersion","retroreflection","diffuseRoughness","clearcoatRadiance","clearcoatSpecularDirect","clearcoatSpecularIndirect","sheenSpecularDirect","sheenSpecularIndirect","iridescenceFresnel","iridescenceF0Dielectric","iridescenceF0Metallic","dfg","eonDirectionalAlbedo","multiScatteringCompensation","singleScatteringDielectric","multiScatteringDielectric"};
    REQUIRE(model.keys()==expected);
    for(std::size_t i=8;i<expected.size();++i)REQUIRE(model.get(expected[i]).isNull());
}

TEST_CASE("PhysicalLightingModel initializes feature caches before real LightingModel start") {
    StackScope scope;const auto events=array(),ctx=context(),b=builder(ctx,events);
    const auto model=ready(flags(255),b,events);
    REQUIRE(events.length()==3);
    REQUIRE(events.get("0").strictEquals("getLightNodes"));
    REQUIRE(events.get("1").get("0").strictEquals("setupLights"));
    REQUIRE(events.get("1").get("1").strictEquals(true));
    REQUIRE(events.get("2").strictEquals("indirect"));
    REQUIRE(model.get("clearcoatSpecularDirect").get("name").strictEquals("clearcoatSpecularDirect"));
    REQUIRE(model.get("sheenSpecularIndirect").get("name").strictEquals("sheenSpecularIndirect"));
    REQUIRE(model.get("dfg").get("name").strictEquals("dfg"));
    REQUIRE(model.get("multiScatteringCompensation").get("name").strictEquals("multiScatteringCompensation"));
    REQUIRE_FALSE(model.get("iridescenceF0Dielectric").isNull());
    REQUIRE_FALSE(model.get("eonDirectionalAlbedo").isNull());
    REQUIRE(ctx.get("backdropAlpha").sameValue(nativePhysicalLightingModelHelper("transmission")));
    REQUIRE(ctx.get("backdrop").get("node").get("rawInputs").get("14").sameValue(nativePhysicalLightingModelHelper("dispersion")));
}

TEST_CASE("PhysicalLightingModel start uses strict booleans for optional features") {
    StackScope scope;const auto events=array(),ctx=context(),b=builder(ctx,events);
    const auto model=ready({1,1,1,1,1,1,1,1},b,events);
    for(const auto* key:{"clearcoatRadiance","clearcoatSpecularDirect","clearcoatSpecularIndirect","sheenSpecularDirect","sheenSpecularIndirect","iridescenceFresnel","iridescenceF0Dielectric","iridescenceF0Metallic","eonDirectionalAlbedo"})REQUIRE(model.get(key).isNull());
    REQUIRE(ctx.get("backdrop").isUndefined());
    REQUIRE_FALSE(model.get("dfg").isNull());
    REQUIRE_FALSE(model.get("singleScatteringDielectric").isNull());
    REQUIRE(events.get("2").strictEquals("indirect"));
}

TEST_CASE("PhysicalLightingModel indirect dispatch stops on exceptions and supports reentry") {
    const auto stopped=indirectOrder("indirectSpecular");
    REQUIRE(stopped.get("events").length()==2);
    REQUIRE(stopped.get("result").get("0").strictEquals("throw"));
    REQUIRE(stopped.get("result").get("1").strictEquals("indirectSpecular"));
    const auto reentered=indirectOrder("",true);
    REQUIRE(reentered.get("events").length()==6);
    REQUIRE(reentered.get("events").get("0").get("0").strictEquals("indirectDiffuse"));
    REQUIRE(reentered.get("events").get("1").get("0").strictEquals("indirectDiffuse"));
    REQUIRE(reentered.get("events").get("3").get("0").strictEquals("ambientOcclusion"));
    REQUIRE(reentered.get("events").get("5").get("0").strictEquals("ambientOcclusion"));
}

TEST_CASE("PhysicalLightingModel roughness transmission helper builds the pinned native shader") {
    StackScope scope;const auto fn=nativePhysicalLightingModelHelper("applyIorToRoughness");
    const auto inputs=array({node("float",{.35}),node("float",{1.4})});
    const auto result=fn.get("shaderNode").get("jsFunc").call({}, {inputs});
    REQUIRE(result.get("type").strictEquals("VarNode"));
    REQUIRE(result.get("node").get("type").strictEquals("OperatorNode"));
    REQUIRE(result.get("node").get("op").strictEquals("*"));
    REQUIRE(generated(result).strictEquals("( 0.35 * clamp( ( ( 1.4 * 2.0 ) - 2.0 ), 0.0, 1.0 ) )"));
    const auto layout=fn.get("shaderNode").get("layout").get("inputs");
    REQUIRE(layout.length()==2);
    REQUIRE(layout.get("0").get("name").strictEquals("roughness"));
    REQUIRE(layout.get("1").get("name").strictEquals("ior"));
}

TEST_CASE("PhysicalLightingModel sheen integration generates native math including energy normalization") {
    StackScope scope;auto inputs=object();inputs.set("normal",node("vec3",{0.,0.,1.}));inputs.set("viewDir",node("vec3",{.1,.2,.9}));inputs.set("roughness",node("float",{.4}));
    const auto result=helperBody("IBLSheenBRDF").call({}, {inputs});
    const auto snippet=generated(result).toString();
    REQUIRE(snippet.find("-1.9362")!=std::string::npos);
    REQUIRE(snippet.find("0.8469")!=std::string::npos);
    REQUIRE(snippet.find("0.1255")!=std::string::npos);
    REQUIRE(snippet.find("clamp( exp(")==0);
    REQUIRE(snippet.find("dot( vec3( 0.0, 0.0, 1.0 ), vec3( 0.1, 0.2, 0.9 ) )")!=std::string::npos);
}

TEST_CASE("PhysicalLightingModel spectral helper uses the pinned matrix and expression grouping") {
    StackScope scope;const auto result=helperBody("evalSensitivity").call({}, {node("float",{500.}),node("vec3",{0.,1.,2.})});
    REQUIRE(result.get("type").strictEquals("VarNode"));
    REQUIRE(result.get("node").get("type").strictEquals("OperatorNode"));
    REQUIRE(result.get("node").get("op").strictEquals("*"));
    REQUIRE(result.get("node").get("aNode").sameValue(nativePhysicalLightingModelHelper("XYZ_TO_REC709")));
    REQUIRE(result.get("node").get("bNode").get("node").get("type").strictEquals("OperatorNode"));
    REQUIRE(result.get("node").get("bNode").get("node").get("op").strictEquals("/"));
}

TEST_CASE("PhysicalLightingModel all hooks and private If Loop bodies produce complete observable graphs") {
    for(const auto* name:{"direct","directRectArea","indirectDiffuse","indirectSpecular","ambientOcclusion","finish","indirect"}){
        const auto graph=lighting(255,name);
        REQUIRE(graph.get("objects").length()>10);
    }
    for(const auto* name:{"volumeAttenuation","evalIridescence","getVolumeTransmissionRay","getTransmissionSample","getIBLVolumeRefraction"}){
        const auto graph=helper(name,1);
        REQUIRE(graph.get("objects").length()>5);
    }
}

TEST_CASE("PhysicalLightingModel array helper destructuring stops after iterator completion") {
    for(const auto& name:arrayHelperNames()){
        const auto empty=iterableHelper(name,"empty"),one=iterableHelper(name,"one");
        REQUIRE(empty.get("events").length()==5);
        REQUIRE(empty.get("events").get("3").get("0").strictEquals("next"));
        REQUIRE(empty.get("events").get("4").get("0").strictEquals("done"));
        REQUIRE(one.get("events").length()==8);
        REQUIRE(one.get("events").get("6").get("0").strictEquals("next"));
        REQUIRE(one.get("events").get("7").get("0").strictEquals("done"));
        REQUIRE(empty.get("result").get("0").strictEquals("error"));
        if(name=="getTransmissionSample")REQUIRE(one.get("result").strictEquals("success"));
        else REQUIRE(one.get("result").get("0").strictEquals("error"));
    }
}

TEST_CASE("PhysicalLightingModel array helper destructuring closes before body and preserves close errors") {
    for(const auto& name:arrayHelperNames()){
        for(const auto* mode:{"exact","extra"}){
            const auto row=iterableHelper(name,mode),events=row.get("events");
            REQUIRE(row.get("result").strictEquals("success"));
            REQUIRE(events.get(JSValue(events.length()-2)).strictEquals("get:return"));
            REQUIRE(events.get(JSValue(events.length()-1)).get("0").strictEquals("return"));
            REQUIRE(events.get(JSValue(events.length()-1)).get("1").strictEquals(true));
            REQUIRE(row.get("graph").get("objects").length()>5);
        }
        const auto getter=iterableHelper(name,"returnGetThrow"),call=iterableHelper(name,"returnThrow"),primitive=iterableHelper(name,"returnPrimitive");
        REQUIRE(getter.get("result").get("1").strictEquals("return:get"));
        REQUIRE(call.get("result").get("1").strictEquals("return:call"));
        REQUIRE(primitive.get("result").get("1").strictEquals("TypeError"));
    }
}

TEST_CASE("PhysicalLightingModel array helpers preserve next done and value exceptions without closing") {
    for(const auto& name:arrayHelperNames())for(const auto* mode:{"nextThrow","doneThrow","valueThrow"}){
        const auto row=iterableHelper(name,mode),events=row.get("events");
        REQUIRE(row.get("result").get("0").strictEquals("throw"));
        REQUIRE(events.length()==(std::string(mode)=="nextThrow"?4u:std::string(mode)=="doneThrow"?5u:6u));
        for(unsigned i=0;i<events.length();++i)REQUIRE_FALSE(events.get(JSValue(i)).strictEquals("get:return"));
    }
}
