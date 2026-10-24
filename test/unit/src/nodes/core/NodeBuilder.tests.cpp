#include "nodes/core/NodeBuilder.h"
#include "nodes/core/Node.h"
#include "nodes/core/NodeCache.h"
#include "nodes/core/NodeVar.h"
#include "nodes/core/NodeVarying.h"
#include "nodes/core/NodeUniform.h"
#include "renderers/common/BindGroup.h"
#include "nodes/code/FunctionNode.h"
#include "nodes/core/ParameterNode.h"
#include "renderers/common/nodes/NodeUniform.h"
#include "core/JSSet.h"
#include "core/JSWeakMap.h"
#include "catch.hpp"

namespace {
using Args = std::vector<JSValue>;
JSValue invoke(const JSValue& self, const char* name, const Args& args = {}) {
    return self.get(name).call(self, args);
}
JSValue renderer() {
    auto r = JSValue::object(), debug = JSValue::object(), diagnostics = JSValue::object();
    diagnostics.set("keywords", false); debug.set("diagnostics", diagnostics); r.set("debug", debug);
    return r;
}
JSValue builder() {
    auto value = NodeBuilder(nullptr, renderer(), nullptr).object();
    invoke(value, "setShaderStage", {"fragment"}); return value;
}
JSValue constant() {
    auto n = nativeNodeConstructor().construct({"float"});
    n.set("generate", JSValue::function([](const JSValue&, const Args&, const Args&) { return JSValue("4.0"); }, {}, "generate", 1));
    return n;
}
}
TEST_CASE("NodeBuilder owns live constructor state and real dependency objects", "[NodeBuilder]") {
    auto first = NodeBuilder(nullptr, renderer(), nullptr).object();
    auto second = NodeBuilder(nullptr, renderer(), nullptr).object();
    REQUIRE(second.get("id").toNumber() == first.get("id").toNumber() + 1);
    REQUIRE(first.getPrototype().strictEquals(NodeBuilder::prototype()));
    REQUIRE(first.get("cache").strictEquals(first.get("globalCache")));
    REQUIRE(first.get("cache").getPrototype().strictEquals(NodeCache::prototype()));
    REQUIRE(first.get("stack").get("isStackNode").strictEquals(true));
    REQUIRE(isNativeSet(first.get("nodes"))); REQUIRE(isNativeSet(first.get("sequentialNodes")));
    REQUIRE(isNativeWeakMap(first.get("flowsData")));
    REQUIRE_FALSE(first.get("stack").strictEquals(second.get("stack")));
    REQUIRE_FALSE(first.get("flowNodes").get("fragment").strictEquals(second.get("flowNodes").get("fragment")));
    const auto id = *first.getOwnPropertyDescriptor("id");
    REQUIRE_FALSE(id.enumerable.value_or(false)); REQUIRE_FALSE(id.writable.value_or(false)); REQUIRE_FALSE(id.configurable.value_or(false));
    const auto method = *NodeBuilder::prototype().getOwnPropertyDescriptor("getDataFromNode");
    REQUIRE_FALSE(method.enumerable.value_or(false)); REQUIRE(method.writable.value_or(false)); REQUIRE(method.configurable.value_or(false));
}
TEST_CASE("NodeBuilder keeps local global and inherited cache identities", "[NodeBuilder]") {
    auto b = builder(), local = constant(), global = constant(); global.set("global", true);
    const auto data = invoke(b, "getDataFromNode", {local});
    REQUIRE(invoke(b, "getDataFromNode", {local}).strictEquals(data));
    REQUIRE_FALSE(invoke(b, "getDataFromNode", {local, "vertex"}).strictEquals(data));
    const auto globalData = invoke(b, "getDataFromNode", {global});
    const auto oldCache = b.get("cache");
    const auto nested = invoke(b, "getCacheFromNode", {local});
    REQUIRE(nested.get("parent").strictEquals(oldCache));
    invoke(b, "setCache", {nested});
    REQUIRE(invoke(b, "getDataFromNode", {local}).strictEquals(data));
    REQUIRE(invoke(b, "getDataFromNode", {global}).strictEquals(globalData));
    invoke(b, "setCache", {nativeNodeCacheConstructor().construct()});
    REQUIRE_FALSE(invoke(b, "getDataFromNode", {local}).strictEquals(data));
    REQUIRE(invoke(b, "getDataFromNode", {global}).strictEquals(globalData));
    REQUIRE(invoke(b, "getCacheFromNode", {local, false}).get("parent").isNull());
}
TEST_CASE("NodeBuilder isolates sub-build data and preserves shared properties", "[NodeBuilder]") {
    auto b = builder(), n = constant();
    const auto base = invoke(b, "getDataFromNode", {n});
    auto any = invoke(b, "getDataFromNode", {n, "any"}); auto subBuilds = createNativeSet();
    invoke(subBuilds, "add", {"outer"}); invoke(subBuilds, "add", {"inner"}); any.set("subBuilds", subBuilds);
    invoke(b, "addSubBuild", {"outer"});
    const auto outer = invoke(b, "getDataFromNode", {n}); REQUIRE_FALSE(outer.strictEquals(base));
    REQUIRE(outer.get("subBuilds").strictEquals(subBuilds));
    invoke(b, "addSubBuild", {"inner"});
    const auto inner = invoke(b, "getDataFromNode", {n}); REQUIRE_FALSE(inner.strictEquals(outer));
    REQUIRE(invoke(b, "getSubBuildProperty", {"variable", subBuilds}).strictEquals("inner_variable"));
    REQUIRE(invoke(b, "removeSubBuild").strictEquals("inner"));
    REQUIRE(invoke(b, "getDataFromNode", {n}).strictEquals(outer));
    invoke(b, "removeSubBuild"); REQUIRE(invoke(b, "getDataFromNode", {n}).strictEquals(base));
}
TEST_CASE("NodeBuilder usage traverses nested write caches and shared nodes", "[NodeBuilder]") {
    auto b = builder(), n = constant();
    REQUIRE(invoke(b, "increaseUsage", {n}).strictEquals(1));
    REQUIRE_FALSE(invoke(b, "hasWriteUsage", {n}).toBoolean());
    auto context = b.get("context"); context.set("assign", true);
    REQUIRE(invoke(b, "increaseUsage", {n}).strictEquals(2));
    REQUIRE(invoke(b, "hasWriteUsage", {n}).toBoolean());
    auto data = invoke(b, "getDataFromNode", {n});
    REQUIRE(data.get("readUsageCount").strictEquals(1)); REQUIRE(data.get("writeUsageCount").strictEquals(1));
    auto replacement = JSValue::object(), stage = JSValue::object(), nested = JSValue::object(), written = JSValue::object();
    written.set("writeUsageCount", 1); nested.set("branch", written); stage.set("subBuildsCache", nested); replacement.set("vertex", stage);
    invoke(b.get("cache"), "setData", {n, replacement}); REQUIRE(invoke(b, "hasWriteUsage", {n}).toBoolean());
}
TEST_CASE("NodeBuilder declarations reuse identities and rename collisions", "[NodeBuilder]") {
    auto b = builder(), n = constant(), other = constant();
    const auto first = invoke(b, "getVarFromNode", {n, "shared", "float"});
    REQUIRE(first.getPrototype().strictEquals(nativeNodeVarConstructor().get("prototype")));
    REQUIRE(invoke(b, "getVarFromNode", {n}).strictEquals(first));
    const auto second = invoke(b, "getVarFromNode", {other, "shared", "vec3"});
    REQUIRE(second.get("name").strictEquals("shared_1"));
    REQUIRE(invoke(b, "getVars", {"fragment"}).strictEquals("float shared;\n\tvec3 shared_1;"));
    const auto uniform = invoke(b, "getUniformFromNode", {n, "float"});
    REQUIRE(uniform.getPrototype().strictEquals(nativeNodeUniformConstructor().get("prototype")));
    auto uniformValue = uniform; uniformValue.set("value", 17); REQUIRE(n.get("value").strictEquals(17));
    const auto varying = invoke(b, "getVaryingFromNode", {n});
    REQUIRE(varying.getPrototype().strictEquals(nativeNodeVaryingConstructor().get("prototype")));
    REQUIRE(varying.get("isNodeVar").strictEquals(true)); REQUIRE(varying.get("interpolationType").isNull());
}
TEST_CASE("NodeBuilder real Node build generates and reuses a cached variable", "[NodeBuilder]") {
    auto b = builder(), n = constant();
    invoke(b, "setBuildStage", {"setup"}); invoke(n, "build", {b, "float"});
    invoke(b, "setBuildStage", {"analyze"}); invoke(n, "build", {b, "float"}); invoke(n, "build", {b, "float"});
    invoke(b, "setBuildStage", {"generate"});
    const auto result = invoke(n, "build", {b, "float"}); REQUIRE(result.strictEquals("nodeConst0"));
    REQUIRE(b.get("flow").get("code").strictEquals("\tfloat nodeConst0 = 4.0;\n"));
    REQUIRE(invoke(n, "build", {b, "vec3"}).strictEquals("vec3( nodeConst0 )"));
    REQUIRE(b.get("flow").get("code").strictEquals("\tfloat nodeConst0 = 4.0;\n"));
    REQUIRE(b.get("chaining").length() == 0);
}
TEST_CASE("NodeBuilder flow stages restore live state on normal completion", "[NodeBuilder]") {
    auto b = builder(), n = constant();
    auto previousFlow = b.get("flow"), previousVars = b.get("vars"), previousCache = b.get("cache"), previousStack = b.get("stack");
    b.set("buildStage", "before"); const auto result = invoke(b, "flowStagesNode", {n, "vec3"});
    REQUIRE(result.get("result").strictEquals("vec3( 4.0 )"));
    REQUIRE(b.get("flow").strictEquals(previousFlow)); REQUIRE(b.get("vars").strictEquals(previousVars));
    REQUIRE(b.get("cache").strictEquals(previousCache)); REQUIRE(b.get("stack").strictEquals(previousStack));
    REQUIRE(b.get("buildStage").strictEquals("before"));
    b.set("buildStage", "generate"); const auto cross = invoke(b, "flowNodeFromShaderStage", {"vertex", n, "float", "position"});
    REQUIRE(cross.get("result").strictEquals("4.0"));
    REQUIRE(b.get("flowCode").get("vertex").strictEquals("\tposition = 4.0;\n"));
    REQUIRE(b.get("shaderStage").strictEquals("fragment")); REQUIRE(b.get("cache").strictEquals(previousCache));
}
TEST_CASE("NodeBuilder bind groups deduplicate identities across stages and cache results", "[NodeBuilder]") {
    auto b = builder(), groupNode = JSValue::object(); groupNode.set("shared", false); groupNode.set("order", 2);
    auto a = JSValue::object(), other = JSValue::object(); a.set("groupNode", groupNode); other.set("groupNode", groupNode);
    const auto fragment = invoke(b, "getBindGroupArray", {"frame", "fragment"});
    const auto vertex = invoke(b, "getBindGroupArray", {"frame", "vertex"});
    invoke(fragment, "push", {a, a}); invoke(vertex, "push", {other, a});
    const auto groups = invoke(b, "getBindings"); REQUIRE(groups.length() == 1);
    const auto group = groups.get("0"); REQUIRE(group.getPrototype().strictEquals(BindGroup::prototype()));
    REQUIRE(group.get("name").strictEquals("frame")); REQUIRE(group.get("bindings").length() == 2);
    REQUIRE(group.get("bindings").get("0").strictEquals(a)); REQUIRE(group.get("bindings").get("1").strictEquals(other));
    REQUIRE(invoke(b, "getBindings").strictEquals(groups));
    invoke(vertex, "push", {JSValue::object()}); REQUIRE(group.get("bindings").length() == 2);
}
TEST_CASE("NodeBuilder uses renderer shared groups only for one shared group node", "[NodeBuilder]") {
    auto b = builder(), node = JSValue::object(), a = JSValue::object(), otherNode = JSValue::object(), other = JSValue::object();
    node.set("shared", true); a.set("groupNode", node); otherNode.set("shared", true); other.set("groupNode", otherNode);
    auto manager = JSValue::object(), expected = JSValue::object();
    manager.set("getSharedBindGroup", JSValue::function([](const JSValue&, const Args&, const Args& captures) { return captures[0]; }, {expected}, "getSharedBindGroup", 2));
    auto r = b.get("renderer"); r.set("_bindings", manager);
    const auto values = JSValue::array(); invoke(values, "push", {a, a});
    REQUIRE(invoke(b, "_getBindGroup", {"frame", values}).strictEquals(expected));
    invoke(values, "push", {other});
    const auto local = invoke(b, "_getBindGroup", {"frame", values});
    REQUIRE(local.getPrototype().strictEquals(BindGroup::prototype())); REQUIRE(local.get("bindings").strictEquals(values));
}
TEST_CASE("NodeBuilder sorts bind groups by node order then updates live indices", "[NodeBuilder]") {
    auto b = builder();
    for (const auto& [name, order] : {std::pair{"late", 4}, {"early", -2}, {"same", 4}}) {
        auto node = JSValue::object(), binding = JSValue::object(); node.set("shared", false); node.set("order", order); binding.set("groupNode", node);
        invoke(invoke(b, "getBindGroupArray", {name, "fragment"}), "push", {binding});
    }
    REQUIRE(invoke(b, "sortBindingGroups").isUndefined()); const auto groups = b.get("bindGroups");
    REQUIRE(groups.get("0").get("name").strictEquals("early")); REQUIRE(groups.get("1").get("name").strictEquals("late")); REQUIRE(groups.get("2").get("name").strictEquals("same"));
    REQUIRE(b.get("bindingsIndexes").get("early").get("group").strictEquals(0));
    REQUIRE(b.get("bindingsIndexes").get("late").get("group").strictEquals(1));
    REQUIRE(b.get("bindingsIndexes").get("same").get("group").strictEquals(2));
}
TEST_CASE("NodeBuilder function cache shares by backend and shader identity", "[NodeBuilder]") {
    auto first = builder(), second = builder(), backend = JSValue::object(), shader = JSValue::object(), calls = JSValue::array();
    for (const auto& b : {first, second}) {
        auto r = b.get("renderer"); r.set("backend", backend);
        auto mutableBuilder = b;
        mutableBuilder.set("buildFunctionCode", JSValue::function([](const JSValue& self, const Args&, const Args& captures) {
            invoke(captures[0], "push", {self.get("currentFunctionNode")}); return JSValue("float cached() { return 1.0; }");
        }, {calls}, "buildFunctionCode", 1));
    }
    const auto function = invoke(first, "buildFunctionNode", {shader});
    REQUIRE(function.getPrototype().strictEquals(FunctionNode::prototype()));
    REQUIRE(invoke(second, "buildFunctionNode", {shader}).strictEquals(function));
    REQUIRE(first.get("currentFunctionNode").isNull()); REQUIRE(calls.length() == 1);
    REQUIRE(function.get("code").strictEquals("float cached() { return 1.0; }"));
    auto r = second.get("renderer"); r.set("backend", JSValue::object());
    REQUIRE_FALSE(invoke(second, "buildFunctionNode", {shader}).strictEquals(function)); REQUIRE(calls.length() == 2);
}
TEST_CASE("NodeBuilder failed function construction preserves the upstream current function", "[NodeBuilder]") {
    auto b = builder(), shader = JSValue::object(), r = b.get("renderer"); r.set("backend", JSValue::object());
    b.set("buildFunctionCode", JSValue::function([](const JSValue&, const Args&, const Args&) -> JSValue { throw JSRangeError("build"); }, {}, "buildFunctionCode", 1));
    REQUIRE_THROWS_AS(invoke(b, "buildFunctionNode", {shader}), JSRangeError);
    const auto failed = b.get("currentFunctionNode"); REQUIRE(failed.getPrototype().strictEquals(FunctionNode::prototype()));
    b.set("buildFunctionCode", JSValue::function([](const JSValue&, const Args&, const Args&) { return JSValue("ok"); }, {}, "buildFunctionCode", 1));
    const auto retry = invoke(b, "buildFunctionNode", {shader}); REQUIRE_FALSE(retry.strictEquals(failed));
    REQUIRE(b.get("currentFunctionNode").strictEquals(failed)); REQUIRE(retry.get("code").strictEquals("ok"));
}
TEST_CASE("NodeBuilder constructs live renderer uniforms for every supported type", "[NodeBuilder]") {
    auto b = builder(), source = JSValue::object(); source.set("name", "weight"); source.set("type", "float"); source.set("value", 1);
    for (const auto* type : {"float", "int", "uint", "vec2", "ivec2", "uvec2", "vec3", "ivec3", "uvec3", "vec4", "ivec4", "uvec4", "color", "mat2", "mat3", "mat4"}) {
        const auto uniform = invoke(b, "getNodeUniform", {source, type});
        REQUIRE(uniform.get("nodeUniform").strictEquals(source)); REQUIRE(invoke(uniform, "getValue").strictEquals(1));
        source.set("value", 2); REQUIRE(invoke(uniform, "getValue").strictEquals(2));
        source.set("value", 1);
    }
    REQUIRE_THROWS(invoke(b, "getNodeUniform", {source, "bool"}));
}
TEST_CASE("NodeBuilder flows a real shader parameter through all build stages", "[NodeBuilder]") {
    auto b = builder(), shader = JSValue::object(), layout = JSValue::object(), input = JSValue::object(), inputs = JSValue::array(), captured = JSValue::object();
    input.set("type", "float"); input.set("name", "weight"); invoke(inputs, "push", {input}); layout.set("inputs", inputs); layout.set("type", "float"); shader.set("layout", layout);
    shader.set("call", JSValue::function([](const JSValue& self, const Args& args, const Args& captures) {
        auto captured = captures[0]; captured.set("layoutWasNull", self.get("layout").isNull()); captured.set("inputs", args[0]); return args[0].get("weight");
    }, {captured}, "call", 1));
    const auto previous = b.get("flow"), result = invoke(b, "flowShaderNode", {shader});
    REQUIRE(result.get("result").strictEquals("weight")); REQUIRE(captured.get("layoutWasNull").strictEquals(true));
    REQUIRE(shader.get("layout").strictEquals(layout)); REQUIRE(b.get("flow").strictEquals(previous));
    const auto parameter = captured.get("inputs").get("weight"); REQUIRE(parameter.getPrototype().strictEquals(ParameterNode::prototype()));
    REQUIRE(parameter.get("placeholderNode").isNull()); REQUIRE(parameter.get("varying").strictEquals(false));
    auto iterator = captured.get("inputs").get(JSValue::wellKnownSymbol("iterator")).call(captured.get("inputs"));
    REQUIRE(iterator.get("next").call(iterator).get("value").strictEquals(parameter));
    REQUIRE(iterator.get("next").call(iterator).get("done").strictEquals(true));
}
TEST_CASE("NodeBuilder shader call failure leaves the upstream null layout", "[NodeBuilder]") {
    auto b = builder(), shader = JSValue::object(), layout = JSValue::object(); layout.set("inputs", JSValue::array()); layout.set("type", "float"); shader.set("layout", layout);
    shader.set("call", JSValue::function([](const JSValue&, const Args&, const Args&) -> JSValue { throw JSRangeError("shader"); }, {}, "call", 1));
    REQUIRE_THROWS_AS(invoke(b, "flowShaderNode", {shader}), JSRangeError); REQUIRE(shader.get("layout").isNull());
}
