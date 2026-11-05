#include "nodes/gpgpu/ComputeBuiltinNode.h"

#include "nodes/core/Node.h"

#include "catch.hpp"

namespace {
JSValue computeBuilder(JSValue& formatted, bool hasBuiltin) {
    auto builder = JSValue::object(nullptr);
    builder.set("shaderStage", JSValue("compute"));
    builder.set("format", JSValue::function(
        +[](const JSValue&, const std::vector<JSValue>&, const std::vector<JSValue>& captures) -> JSValue {
            auto holder = captures[0];
            holder.set("value", JSValue(true));
            return JSValue("formatted");
        },
        {formatted}, "format", 3));
    builder.set("generateConst", JSValue::function(
        +[](const JSValue&, const std::vector<JSValue>&, const std::vector<JSValue>&) -> JSValue {
            return JSValue("const");
        },
        {}, "generateConst", 1));
    builder.set("getDataFromNode", JSValue::function(
        +[](const JSValue&, const std::vector<JSValue>&, const std::vector<JSValue>&) -> JSValue {
            return JSValue::object(nullptr);
        },
        {}, "getDataFromNode", 1));
    builder.set("getNodeType", JSValue::function(
        +[](const JSValue&, const std::vector<JSValue>&, const std::vector<JSValue>&) -> JSValue {
            return JSValue("uvec3");
        },
        {}, "getNodeType", 2));
    builder.set("hasBuiltin", JSValue::function(
        +[](const JSValue&, const std::vector<JSValue>&, const std::vector<JSValue>& captures) -> JSValue {
            return captures[0];
        },
        {JSValue(hasBuiltin)}, "hasBuiltin", 1));
    return builder;
}
}  // namespace

TEST_CASE("ComputeBuiltinNode keeps its built-in name and node type") {
    const auto module = nativeComputeBuiltinNodeModule();
    const auto ctor = module.get("default");
    REQUIRE(ctor.get("name").strictEquals("ComputeBuiltinNode"));
    REQUIRE(ctor.get("length").strictEquals(2));
    REQUIRE(ctor.get("type").strictEquals("ComputeBuiltinNode"));

    const auto builtin = ctor.construct({JSValue("globalId"), JSValue("uvec3")});
    REQUIRE(builtin.get("type").strictEquals("ComputeBuiltinNode"));
    REQUIRE(builtin.get("isNode").strictEquals(true));
    REQUIRE(builtin.get("_builtinName").strictEquals("globalId"));
    // The second constructor parameter is passed to the node constructor, so it lands in nodeType.
    REQUIRE(builtin.get("nodeType").strictEquals("uvec3"));
    REQUIRE(builtin.get("isCacheable").call(builtin, {}).strictEquals(false));
    REQUIRE(builtin.get("getBuiltinName").call(builtin, {}).strictEquals("globalId"));
    REQUIRE(builtin.get("generateNodeType").call(builtin, {}).strictEquals("uvec3"));

    // setBuiltinName answers the node itself and replaces the name.
    const auto renamed = builtin.get("setBuiltinName").call(builtin, {JSValue("localId")});
    REQUIRE(renamed.sameValue(builtin));
    REQUIRE(builtin.get("_builtinName").strictEquals("localId"));
}

TEST_CASE("ComputeBuiltinNode hashes, checks and generates through the builder") {
    const auto builtin =
        nativeComputeBuiltinNodeConstructor().construct({JSValue("workgroupId"), JSValue("uvec3")});
    auto formatted = JSValue::object(nullptr);
    formatted.set("value", false);
    const auto builder = computeBuilder(formatted, true);
    REQUIRE(builtin.get("getHash").call(builtin, {builder}).strictEquals("workgroupId"));
    REQUIRE(builtin.get("hasBuiltin").call(builtin, {builder}).strictEquals(true));
    // In the compute stage the built-in name is formatted through the builder.
    REQUIRE(builtin.get("generate").call(builtin, {builder, JSValue("uvec3")}).strictEquals("formatted"));
    REQUIRE(formatted.get("value").strictEquals(true));

    // Outside the compute stage it answers a constant instead and warns.
    auto outputStage = JSValue::object(nullptr);
    outputStage.set("shaderStage", JSValue("fragment"));
    outputStage.set("getDataFromNode", JSValue::function(
        +[](const JSValue&, const std::vector<JSValue>&, const std::vector<JSValue>&) -> JSValue { return JSValue::object(nullptr); },
        {}, "getDataFromNode", 1));
    outputStage.set("getNodeType", JSValue::function(
        +[](const JSValue&, const std::vector<JSValue>&, const std::vector<JSValue>&) -> JSValue { return JSValue("uvec3"); },
        {}, "getNodeType", 2));
    outputStage.set("generateConst", JSValue::function(
        +[](const JSValue&, const std::vector<JSValue>&, const std::vector<JSValue>&) -> JSValue {
            return JSValue("const");
        },
        {}, "generateConst", 1));
    REQUIRE(builtin.get("generate").call(builtin, {outputStage, JSValue("uvec3")}).strictEquals("const"));
}

TEST_CASE("ComputeBuiltinNode serialises its built-in name and global flag") {
    const auto builtin =
        nativeComputeBuiltinNodeConstructor().construct({JSValue("numWorkgroups"), JSValue("uvec3")});
    auto node = builtin;
    node.set("global", true);
    auto data = JSValue::object(nullptr);
    builtin.get("serialize").call(builtin, {data});
    REQUIRE(data.get("global").strictEquals(true));
    REQUIRE(data.get("_builtinName").strictEquals("numWorkgroups"));

    // Deserialising restores both fields.
    const auto target =
        nativeComputeBuiltinNodeConstructor().construct({JSValue("localId"), JSValue("vec3")});
    auto restore = JSValue::object(nullptr);
    restore.set("global", false);
    restore.set("_builtinName", JSValue("globalId"));
    target.get("deserialize").call(target, {restore});
    REQUIRE(target.get("global").strictEquals(false));
    REQUIRE(target.get("_builtinName").strictEquals("globalId"));
}

TEST_CASE("ComputeBuiltinNode module exposes the five builtins") {
    const auto module = nativeComputeBuiltinNodeModule();
    const auto globalId = module.get("globalId");
    REQUIRE(globalId.get("_builtinName").strictEquals("globalId"));
    REQUIRE(globalId.get("nodeType").strictEquals("uvec3"));
    REQUIRE(module.get("numWorkgroups").get("nodeType").strictEquals("uvec3"));
    REQUIRE(module.get("workgroupId").get("nodeType").strictEquals("uvec3"));
    REQUIRE(module.get("localId").get("nodeType").strictEquals("uvec3"));
    REQUIRE(module.get("subgroupSize").get("nodeType").strictEquals("uint"));
    // They are shared module level nodes, not fresh ones.
    REQUIRE(module.get("globalId").sameValue(globalId));
}
