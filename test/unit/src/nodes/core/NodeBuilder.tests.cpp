#include "nodes/core/NodeBuilder.h"
#include "nodes/core/Node.h"
#include "nodes/core/NodeCache.h"
#include "nodes/core/NodeVar.h"
#include "nodes/core/NodeVarying.h"
#include "nodes/core/NodeUniform.h"
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
