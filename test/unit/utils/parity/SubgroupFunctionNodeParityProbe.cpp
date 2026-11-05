#include "nodes/gpgpu/SubgroupFunctionNode.h"

#include "core/JSValue.h"

#include <iostream>
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
            const auto text = (args.empty() ? JSValue("") : args[0]).toString();
            if (text == "float" || text == "int" || text == "uint" || text == "bool") return JSValue(1.0);
            if (!text.empty() && text.back() >= '2' && text.back() <= '4') {
                return JSValue(static_cast<double>(text.back() - '0'));
            }
            return JSValue(0.0);
        },
        {}, "getTypeLength", 1));
    return builder;
}

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

int main() {
    try {
        const auto module = nativeSubgroupFunctionNodeModule();
        const auto ctor = module.get("default");
        const auto typeB = typeBuilder();

        const auto floatNode = nodeStub("float", "a");
        const auto vec4Node = nodeStub("vec4", "b");
        auto add = ctor.construct({JSValue("subgroupAdd"), floatNode, vec4Node});
        const auto inputType = add.get("getInputType").call(add, {typeB});
        const auto generatedType = add.get("generateNodeType").call(add, {typeB});

        const auto matrixNode = nodeStub("mat4", "m");
        auto mixed = ctor.construct({JSValue("subgroupAdd"), matrixNode, floatNode});
        const auto mixedInputType = mixed.get("getInputType").call(mixed, {typeB});
        const auto elect = ctor.construct({JSValue("subgroupElect"), floatNode, JSValue(nullptr)});
        const auto electType = elect.get("generateNodeType").call(elect, {typeB});
        const auto ballot = ctor.construct({JSValue("subgroupBallot"), floatNode, JSValue(nullptr)});
        const auto ballotType = ballot.get("generateNodeType").call(ballot, {typeB});

        auto formatted = JSValue::object(nullptr);
        auto electNode = ctor.construct({JSValue("subgroupElect"), JSValue(nullptr), JSValue(nullptr)});
        auto electBuilder = generateBuilder(formatted);
        const auto electSnippet =
            electNode.get("generate").call(electNode, {electBuilder, JSValue("")});

        const auto a = nodeStub("float", "a");
        auto addNode = ctor.construct({JSValue("subgroupAdd"), a, JSValue(nullptr)});
        auto addBuilder = generateBuilder(formatted);
        const auto addSnippet = addNode.get("generate").call(addNode, {addBuilder, JSValue("")});

        const auto bFloat = nodeStub("float", "b");
        auto broadcastNode = ctor.construct({JSValue("subgroupBroadcast"), a, bFloat});
        auto broadcastBuilder = generateBuilder(formatted);
        const auto broadcastSnippet =
            broadcastNode.get("generate").call(broadcastNode, {broadcastBuilder, JSValue("")});

        auto shuffleNode = ctor.construct({JSValue("subgroupShuffleXor"), a, bFloat});
        auto shuffleBuilder = generateBuilder(formatted);
        const auto shuffleSnippet =
            shuffleNode.get("generate").call(shuffleNode, {shuffleBuilder, JSValue("")});

        auto data = JSValue::object(nullptr);
        addNode.get("serialize").call(addNode, {data});
        const auto serializedMethod = data.get("method");
        const auto target = ctor.construct({JSValue("subgroupAny")});
        auto restore = JSValue::object(nullptr);
        restore.set("method", JSValue("subgroupBallot"));
        target.get("deserialize").call(target, {restore});
        const auto deserializedMethod = target.get("method");

        auto output = JSValue::object(nullptr);
        output.set("name", ctor.get("name"));
        output.set("length", ctor.get("length"));
        output.set("staticType", ctor.get("type"));
        output.set("type", add.get("type"));
        output.set("method", add.get("method"));
        output.set("aNull", add.get("aNode").isNull());
        output.set("constants", [&]() {
            auto constants = JSValue::object(nullptr);
            for (const auto* name : {"SUBGROUP_ELECT", "SUBGROUP_BALLOT", "SUBGROUP_ADD",
                                     "SUBGROUP_INCLUSIVE_ADD", "SUBGROUP_EXCLUSIVE_ADD",
                                     "SUBGROUP_MUL", "SUBGROUP_INCLUSIVE_MUL",
                                     "SUBGROUP_EXCLUSIVE_MUL", "SUBGROUP_AND", "SUBGROUP_OR",
                                     "SUBGROUP_XOR", "SUBGROUP_MIN", "SUBGROUP_MAX", "SUBGROUP_ALL",
                                     "SUBGROUP_ANY", "SUBGROUP_BROADCAST_FIRST", "QUAD_SWAP_X",
                                     "QUAD_SWAP_Y", "QUAD_SWAP_DIAGONAL", "SUBGROUP_BROADCAST",
                                     "SUBGROUP_SHUFFLE", "SUBGROUP_SHUFFLE_XOR",
                                     "SUBGROUP_SHUFFLE_UP", "SUBGROUP_SHUFFLE_DOWN",
                                     "QUAD_BROADCAST"}) {
                constants.set(name, ctor.get(name));
            }
            return constants;
        }());
        output.set("inputType", inputType);
        output.set("generatedType", generatedType);
        output.set("mixedInputType", mixedInputType);
        output.set("electType", electType);
        output.set("ballotType", ballotType);
        output.set("electSnippet", electSnippet);
        output.set("addSnippet", addSnippet);
        output.set("broadcastSnippet", broadcastSnippet);
        output.set("shuffleSnippet", shuffleSnippet);
        output.set("serializedMethod", serializedMethod);
        output.set("deserializedMethod", deserializedMethod);
        output.set("arityTable", [&]() {
            auto table = JSValue::object(nullptr);
            for (const auto* name : {"subgroupElect", "subgroupBallot", "subgroupAdd",
                                     "subgroupBroadcast", "subgroupShuffleDown", "quadBroadcast"}) {
                table.set(name, module.get(name).get("length"));
            }
            return table;
        }());
        output.set("electCallable", module.get("subgroupElect").isCallable());
        output.set("source", ctor.get("toString").call(ctor));
        std::cout << output.toJSON()->dump() << '\n';
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "exception: " << error.what() << '\n';
        return 1;
    }
}
