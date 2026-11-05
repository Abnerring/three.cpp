#include "nodes/functions/BSDF/EnvironmentBRDF.h"
#include "../../../utils/parity/BSDFLookupFixtures.h"
#include "catch.hpp"
using namespace BSDFLookupFixtures;
TEST_CASE("EnvironmentBRDF retains an untyped inline Fn with pinned source") {
    const auto value=nativeEnvironmentBRDF(),shader=value.get("shaderNode"),callback=shader.get("jsFunc");
    REQUIRE(nativeEnvironmentBRDFModule().get("default").sameValue(value));REQUIRE(value.get("isFn").strictEquals(true));
    REQUIRE(shader.get("nodeType").isNull());REQUIRE(shader.get("layout").isNull());REQUIRE(callback.get("length").strictEquals(1));
    REQUIRE(call(callback,"toString").toString().find("const { dotNV, specularColor, specularF90, roughness } = inputs;")!=std::string::npos);
}
TEST_CASE("EnvironmentBRDF destructures four inputs before any math and stops at the throwing getter") {
    const auto ok=observed(true,"");REQUIRE(ok.get("result").strictEquals("success"));
    REQUIRE(ok.get("events").toJSON()->dump()=="[\"dotNV\",\"specularColor\",\"specularF90\",\"roughness\"]");
    const std::vector<std::string> order={"dotNV","specularColor","specularF90","roughness"};
    for(unsigned i=0;i<order.size();++i){const auto row=observed(true,order[i]);REQUIRE(row.get("events").length()==i+1);REQUIRE(row.get("result").get("0").strictEquals("throw"));REQUIRE(row.get("result").get("1").strictEquals(order[i]));}
    REQUIRE_THROWS_AS(body(true).call({}, {nullptr}),JSTypeError);REQUIRE_THROWS_AS(body(true).call({}, {JSValue{}}),JSTypeError);
}
TEST_CASE("EnvironmentBRDF builds specularColor times DFG x plus specularF90 times DFG y") {
    StackScope scope;const auto in=inputs(true),result=body(true).call({}, {in});
    REQUIRE(result.get("type").strictEquals("VarNode"));const auto add=result.get("node");REQUIRE(add.get("op").strictEquals("+"));
    const auto left=add.get("aNode").get("node"),right=add.get("bNode").get("node");
    REQUIRE(left.get("op").strictEquals("*"));REQUIRE(right.get("op").strictEquals("*"));
    REQUIRE(left.get("aNode").sameValue(in.get("specularColor")));REQUIRE(right.get("aNode").sameValue(in.get("specularF90")));
    const auto x=left.get("bNode"),y=right.get("bNode");REQUIRE(x.get("components").strictEquals("x"));REQUIRE(y.get("components").strictEquals("y"));
    REQUIRE(x.get("node").sameValue(y.get("node")));
    const auto callNode=walkFind(result,[](const JSValue& v){return v.get("isShaderCallNodeInternal").strictEquals(true)&&v.get("shaderNode").sameValue(nativeDFGLUT().get("shaderNode"));});
    const auto dfgInputs=callNode.get("rawInputs").get("0");REQUIRE(dfgInputs.get("dotNV").sameValue(in.get("dotNV")));REQUIRE(dfgInputs.get("roughness").sameValue(in.get("roughness")));
    const auto actual=body(false).call({}, {dfgInputs});REQUIRE(texture(actual).get("name").strictEquals("DFG_LUT"));
    REQUIRE(graph(true).get("objects").length()>20);
}
