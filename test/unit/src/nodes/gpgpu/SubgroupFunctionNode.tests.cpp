#include "nodes/gpgpu/SubgroupFunctionNode.h"

#include "nodes/core/Node.h"

#include "catch.hpp"

#include <string>
#include <vector>

namespace {
using ArgList = std::vector<JSValue>;

JSValue nodeStub(const char* type, const char* snippet) {
    auto node = JSValue::object(nullptr);
    node.set("getNodeType", JSValue::function(
        +[](const JSValue&, const ArgList&, const ArgList& captures) -> JSValue { return captures[0]; },
        {JSValue(type)}, "getNodeType", 1));
    node.set("build", JSValue::function(
        +[](const JSValue&, const ArgList& args, const ArgList& captures) -> JSValue {
            const auto type = args.size() > 1 ? args[1] : JSValue("");
            return JSValue(captures[0].toString() + ":" + type.toString());
        },
        {JSValue(snippet)}, "build", 2));
    return node;
}

JSValue typeBuilder() {
    auto builder = JSValue::object(nullptr);
    builder.set("isMatrix", JSValue::function(
        +[](const JSValue&, const ArgList& args, const ArgList&) -> JSValue {
            const auto type = args.empty() ? JSValue("") : args[0];
            return JSValue(type.toString().rfind("mat", 0) == 0);
        },
        {}, "isMatrix", 1));
    builder.set("getTypeLength", JSValue::function(
        +[](const JSValue&, const ArgList& args, const ArgList&) -> JSValue {
            const auto type = args.empty() ? JSValue("") : args[0];
            const auto text = type.toString();
            if (text == "float" || text == "int" || text == "uint" || text == "bool") return JSValue(1.0);
            if (!text.empty() && text.back() >= '2' && text.back() <= '4') {
                return JSValue(static_cast<double>(text.back() - '0'));
            }
            return JSValue(0.0);
        },
        {}, "getTypeLength", 1));
    return builder;
}

// A builder stub for the generate path; format records the composed snippet.
JSValue generateBuilder(JSValue& formatted) {
    auto builder = JSValue::object(nullptr);
    builder.set("shaderStage", JSValue("compute"));
    builder.set("getDataFromNode", JSValue::function(
        +[](const JSValue&, const ArgList&, const ArgList&) -> JSValue { return JSValue::object(nullptr); },
        {}, "getDataFromNode", 1));
    builder.set("isMatrix", JSValue::function(
        +[](const JSValue&, const ArgList& args, const ArgList&) -> JSValue {
            const auto type = args.empty() ? JSValue("") : args[0];
            return JSValue(type.toString().rfind("mat", 0) == 0);
        },
        {}, "isMatrix", 1));
    builder.set("getTypeLength", JSValue::function(
        +[](const JSValue&, const ArgList& args, const ArgList&) -> JSValue {
            const auto text = (args.empty() ? JSValue("") : args[0]).toString();
            if (text == "float" || text == "int" || text == "uint" || text == "bool") return JSValue(1.0);
            if (!text.empty() && text.back() >= '2' && text.back() <= '4') {
                return JSValue(static_cast<double>(text.back() - '0'));
            }
            return JSValue(0.0);
        },
        {}, "getTypeLength", 1));
    builder.set("getNodeType", JSValue::function(
        +[](const JSValue&, const ArgList&, const ArgList&) -> JSValue { return JSValue("float"); },
        {}, "getNodeType", 2));    builder.set("getMethod", JSValue::function(
        +[](const JSValue&, const ArgList& args, const ArgList&) -> JSValue {
            return args.empty() ? JSValue("method") : args[0];
        },
        {}, "getMethod", 2));
    builder.set("format", JSValue::function(
        +[](const JSValue&, const ArgList& args, const ArgList& captures) -> JSValue {
            const auto snippet = args.empty() ? JSValue("") : args[0];
            auto holder = captures[0];
            holder.set("snippet", snippet);
            return snippet;
        },
        {formatted}, "format", 3));
    return builder;
}
}  // namespace

TEST_CASE("SubgroupFunctionNode exposes the pinned name constants") {
    const auto module = nativeSubgroupFunctionNodeModule();
    const auto ctor = module.get("default");
    REQUIRE(ctor.get("name").strictEquals("SubgroupFunctionNode"));
    REQUIRE(ctor.get("length").strictEquals(1));
    REQUIRE(ctor.get("type").strictEquals("SubgroupFunctionNode"));
    REQUIRE(ctor.get("SUBGROUP_ELECT").strictEquals("subgroupElect"));
    REQUIRE(ctor.get("SUBGROUP_BALLOT").strictEquals("subgroupBallot"));
    REQUIRE(ctor.get("SUBGROUP_ADD").strictEquals("subgroupAdd"));
    REQUIRE(ctor.get("SUBGROUP_SHUFFLE_XOR").strictEquals("subgroupShuffleXor"));
    REQUIRE(ctor.get("QUAD_BROADCAST").strictEquals("quadBroadcast"));
    REQUIRE(ctor.get("QUAD_SWAP_DIAGONAL").strictEquals("quadSwapDiagonal"));

    const auto node = ctor.construct({JSValue("subgroupAdd")});
    REQUIRE(node.get("type").strictEquals("SubgroupFunctionNode"));
    REQUIRE(node.get("method").strictEquals("subgroupAdd"));
    REQUIRE(node.get("aNode").isNull());
    REQUIRE(node.get("bNode").isNull());
    REQUIRE(node.get("getInputType").get("length").strictEquals(1));
    REQUIRE(node.get("setup").get("length").strictEquals(1));
    REQUIRE(node.get("generateNodeType").get("length").strictEquals(1));
    REQUIRE(node.get("generate").get("length").strictEquals(2));
    REQUIRE(node.get("serialize").get("length").strictEquals(1));
    REQUIRE(node.get("deserialize").get("length").strictEquals(1));
}

TEST_CASE("SubgroupFunctionNode resolves the input and output types") {
    const auto ctor = nativeSubgroupFunctionNodeConstructor();
    const auto builder = typeBuilder();

    // float is shorter than vec4, so the vec4 operand wins.
    const auto floatNode = nodeStub("float", "a");
    const auto vec4Node = nodeStub("vec4", "b");
    auto add = ctor.construct({JSValue("subgroupAdd"), floatNode, vec4Node});
    REQUIRE(add.get("getInputType").call(add, {builder}).strictEquals("vec4"));
    REQUIRE(add.get("generateNodeType").call(add, {builder}).strictEquals("vec4"));

    // A matrix operand counts as length zero, so the vector operand wins.
    const auto matrixNode = nodeStub("mat4", "m");
    auto mixed = ctor.construct({JSValue("subgroupAdd"), matrixNode, floatNode});
    REQUIRE(mixed.get("getInputType").call(mixed, {builder}).strictEquals("float"));

    // elect and ballot answer fixed types regardless of the input.
    const auto elect = ctor.construct({JSValue("subgroupElect"), floatNode, JSValue(nullptr)});
    REQUIRE(elect.get("generateNodeType").call(elect, {builder}).strictEquals("bool"));
    const auto ballot = ctor.construct({JSValue("subgroupBallot"), floatNode, JSValue(nullptr)});
    REQUIRE(ballot.get("generateNodeType").call(ballot, {builder}).strictEquals("uvec4"));
}

TEST_CASE("SubgroupFunctionNode generate composes the call for each operand shape") {
    const auto ctor = nativeSubgroupFunctionNodeConstructor();
    auto formatted = JSValue::object(nullptr);

    // No operands: the parameter list is empty parentheses.
    auto elect = ctor.construct({JSValue("subgroupElect"), JSValue(nullptr), JSValue(nullptr)});
    auto electBuilder = generateBuilder(formatted);
    REQUIRE(elect.get("generate").call(elect, {electBuilder, JSValue("")}).strictEquals("subgroupElect()"));

    // One operand: built with the input type.
    const auto a = nodeStub("float", "a");
    auto add = ctor.construct({JSValue("subgroupAdd"), a, JSValue(nullptr)});
    auto addBuilder = generateBuilder(formatted);
    REQUIRE(add.get("generate").call(add, {addBuilder, JSValue("")}).strictEquals("subgroupAdd( a:float )"));

    // Broadcast takes a second operand; a float index is built as int.
    const auto bFloat = nodeStub("float", "b");
    auto broadcast = ctor.construct({JSValue("subgroupBroadcast"), a, bFloat});
    auto broadcastBuilder = generateBuilder(formatted);
    REQUIRE(broadcast.get("generate")
                .call(broadcast, {broadcastBuilder, JSValue("")})
                .strictEquals("subgroupBroadcast( a:float, b:int )"));

    // Shuffle offsets are always built as uint.
    auto shuffle = ctor.construct({JSValue("subgroupShuffleXor"), a, bFloat});
    auto shuffleBuilder = generateBuilder(formatted);
    REQUIRE(shuffle.get("generate")
                .call(shuffle, {shuffleBuilder, JSValue("")})
                .strictEquals("subgroupShuffleXor( a:float, b:uint )"));
}

TEST_CASE("SubgroupFunctionNode serialises its method name") {
    const auto ctor = nativeSubgroupFunctionNodeConstructor();
    const auto node = ctor.construct({JSValue("subgroupAll")});
    auto data = JSValue::object(nullptr);
    node.get("serialize").call(node, {data});
    REQUIRE(data.get("method").strictEquals("subgroupAll"));

    const auto target = ctor.construct({JSValue("subgroupAny")});
    auto restore = JSValue::object(nullptr);
    restore.set("method", JSValue("subgroupBallot"));
    target.get("deserialize").call(target, {restore});
    REQUIRE(target.get("method").strictEquals("subgroupBallot"));
}

TEST_CASE("SubgroupFunctionNode module exports the subgroup and quad helpers") {
    const auto module = nativeSubgroupFunctionNodeModule();
    REQUIRE(module.get("subgroupElect").get("length").strictEquals(0));
    REQUIRE(module.get("subgroupBallot").get("length").strictEquals(0));
    REQUIRE(module.get("subgroupAdd").get("length").strictEquals(0));
    REQUIRE(module.get("subgroupBroadcast").get("length").strictEquals(0));
    REQUIRE(module.get("subgroupShuffleDown").get("length").strictEquals(0));
    REQUIRE(module.get("quadBroadcast").get("length").strictEquals(0));
    REQUIRE(module.get("subgroupElect").isCallable());
}
