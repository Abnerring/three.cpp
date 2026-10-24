#include "nodes/core/NodeParser.h"
#include "nodes/core/NodeFunction.h"
#include "nodes/core/NodeFunctionInput.h"
#include "nodes/parsers/GLSLNodeParser.h"
#include "nodes/parsers/GLSLNodeFunction.h"
#include "renderers/webgpu/nodes/WGSLNodeParser.h"
#include "renderers/webgpu/nodes/WGSLNodeFunction.h"
#include "nodes/code/FunctionNode.h"
#include "catch.hpp"

namespace {
using Args = std::vector<JSValue>;
JSValue call(const JSValue& self, const char* key, const Args& args = {}) { return self.get(key).call(self, args); }
}

TEST_CASE("Shader parsers use actual NodeParser and NodeFunction prototype chains", "[nodes][ShaderParsers]") {
    const auto base = nativeNodeParserConstructor(), function = nativeNodeFunctionConstructor();
    for (const auto& parser : {nativeGLSLNodeParserConstructor(), nativeWGSLNodeParserConstructor()}) {
        REQUIRE(parser.getPrototype().sameValue(base)); REQUIRE(parser.get("prototype").getPrototype().sameValue(base.get("prototype")));
        REQUIRE(parser.get("length").strictEquals(0)); REQUIRE(parser.get("prototype").get("parseFunction").get("length").strictEquals(1));
        REQUIRE_THROWS_AS(parser.call(), JSTypeError);
    }
    for (const auto& ctor : {nativeGLSLNodeFunctionConstructor(), nativeWGSLNodeFunctionConstructor()}) {
        REQUIRE(ctor.getPrototype().sameValue(function)); REQUIRE(ctor.get("prototype").getPrototype().sameValue(function.get("prototype")));
        REQUIRE(ctor.get("length").strictEquals(1)); REQUIRE(ctor.get("prototype").get("getCode").get("length").strictEquals(0));
    }
}

TEST_CASE("GLSL pragma separates header and preserves the original function body", "[nodes][ShaderParsers]") {
    const auto f = GLSLNodeParser().parseFunction("\nfloat helper(){return 0.;}\n#pragma main\n// comment\n/* comment */ highp vec3 paint(vec3 p)\n{ return p; }\n");
    REQUIRE(f.get("type").strictEquals("vec3")); REQUIRE(f.get("name").strictEquals("paint")); REQUIRE(f.get("precision").strictEquals("highp"));
    REQUIRE(f.get("headerCode").strictEquals("float helper(){return 0.;}\n")); REQUIRE(f.get("blockCode").strictEquals("\n{ return p; }"));
    REQUIRE(call(f, "getCode", {"newName"}).strictEquals("float helper(){return 0.;}\nhighp vec3 newName ( vec3 p )\n{ return p; }"));
}

TEST_CASE("GLSL parameter parsing retains qualifiers and the upstream array syntax", "[nodes][ShaderParsers]") {
    const auto f = GLSLNodeFunction("void modify(const in vec3 p, out float[4] samples, inout uint count) {}").object(), inputs = f.get("inputs");
    REQUIRE(inputs.length() == 3); const auto p = inputs.get("0"), samples = inputs.get("1"), count = inputs.get("2");
    REQUIRE(p.get("isConst").strictEquals(true)); REQUIRE(p.get("qualifier").strictEquals("in")); REQUIRE(p.get("count").isNull());
    REQUIRE(samples.get("type").strictEquals("float")); REQUIRE(samples.get("count").strictEquals(4)); REQUIRE(samples.get("name").strictEquals("samples"));
    REQUIRE(count.get("type").strictEquals("uint")); REQUIRE(count.get("qualifier").strictEquals("inout"));
    REQUIRE(p.getPrototype().sameValue(nativeNodeFunctionInputConstructor().get("prototype")));
    REQUIRE_THROWS_AS(GLSLNodeFunction("void modify(float samples[4]){}"), JSTypeError);
}

TEST_CASE("GLSL interfaces and invalid code follow the pinned source behavior", "[nodes][ShaderParsers]") {
    REQUIRE(GLSLNodeFunction("float interfaceFn()").getCode().strictEquals(""));
    REQUIRE(GLSLNodeFunction("float interfaceFn()  ").getCode().strictEquals(""));
    REQUIRE(GLSLNodeFunction("float f(float 0xff a){}").object().get("inputs").get("0").get("count").strictEquals(255));
    REQUIRE(GLSLNodeFunction("float f(float 1e3 a){}").object().get("inputs").get("0").get("count").strictEquals(1));
    REQUIRE_THROWS_AS(GLSLNodeFunction("invalid shader"), std::runtime_error);
    REQUIRE_THROWS_AS(GLSLNodeFunction(nullptr), JSTypeError);
    REQUIRE_THROWS_AS(GLSLNodeFunction("float f(const){}"), JSTypeError);
}

TEST_CASE("WGSL parameter type resolution preserves pointers textures and unknowns", "[nodes][ShaderParsers]") {
    const auto f = WGSLNodeParser().parseFunction("fn sample(p: ptr<storage, f32, read_write>, v: vec3u, t: texture_multisampled_2d<f32>, s: sampler_comparison, x: Custom) -> vec4f {return vec4f();}");
    const auto inputs = f.get("inputs"); REQUIRE(inputs.length() == 5);
    REQUIRE(inputs.get("0").get("type").strictEquals("pointer")); REQUIRE(inputs.get("1").get("type").strictEquals("uvec3"));
    REQUIRE(inputs.get("2").get("type").strictEquals("cubeTexture")); REQUIRE(inputs.get("3").get("type").strictEquals("samplerComparison"));
    REQUIRE(inputs.get("4").get("type").isUndefined()); REQUIRE(f.get("type").strictEquals("vec4")); REQUIRE(f.get("outputType").strictEquals("vec4f"));
    REQUIRE(inputs.get("0").get("count").isNull()); REQUIRE(f.get("precision").strictEquals(""));
}

TEST_CASE("WGSL output spelling and whitespace follow getCode", "[nodes][ShaderParsers]") {
    REQUIRE(WGSLNodeFunction("fn f(a: f32) -> f32 { return a; }\n").getCode("renamed").strictEquals("fn renamed ( a: f32 ) -> f32 { return a; }\n"));
    REQUIRE(WGSLNodeFunction("fn f() -> void {}").getCode().strictEquals("fn f (  )  {}"));
    REQUIRE(WGSLNodeFunction("fn f() -> Custom {}").object().get("type").strictEquals("Custom"));
    REQUIRE_THROWS_AS(WGSLNodeFunction("fn f() {}"), std::runtime_error);
    REQUIRE_THROWS_AS(WGSLNodeFunction("fn f() ->"), std::runtime_error);
}

TEST_CASE("GLSL getCode reads and coerces fields in source order", "[nodes][ShaderParsers]") {
    auto f = GLSLNodeFunction("float f(){}").object(), events = JSValue::array();
    for (const auto* key : {"name", "blockCode", "type", "inputsCode", "headerCode", "precision"}) {
        const auto value = f.get(key); JSPropertyDescriptor d; d.configurable = true;
        d.getter = JSValue::function([](const JSValue&, const Args&, const Args& c) -> JSValue { call(c[0], "push", {c[1]}); return c[2]; }, {events, key, value}); f.defineProperty(key, d);
    }
    REQUIRE(call(f, "getCode").strictEquals("float f (  ){}")); REQUIRE(call(events, "join", {","}).strictEquals("name,blockCode,type,inputsCode,headerCode,precision"));
    events.set("length", 0); REQUIRE(call(f, "getCode", {"renamed"}).strictEquals("float renamed (  ){}"));
    REQUIRE(call(events, "join", {","}).strictEquals("blockCode,type,inputsCode,headerCode,precision"));
}

TEST_CASE("FunctionNode caches actual parsed functions independently of source edits", "[nodes][ShaderParsers]") {
    for (const auto& parser : {GLSLNodeParser().object(), WGSLNodeParser().object()}) {
        const auto glsl = parser.getPrototype().sameValue(GLSLNodeParser::prototype()); auto builder = JSValue::object(), data = JSValue::object(); builder.set("parser", parser);
        builder.set("getDataFromNode", JSValue::function([](const JSValue&, const Args&, const Args& c) -> JSValue { return c[0]; }, {data}));
        auto node = FunctionNode(glsl ? JSValue("float original(float x){return x;}") : JSValue("fn original(x: f32) -> f32 {return x;}")).object();
        const auto first = call(node, "getNodeFunction", {builder}); node.set("code", "not valid shader");
        REQUIRE(call(node, "getNodeFunction", {builder}).sameValue(first)); REQUIRE(call(node, "getInputs", {builder}).sameValue(first.get("inputs")));
        REQUIRE(first.get("name").strictEquals("original")); REQUIRE(first.get("type").strictEquals("float"));
    }
}
