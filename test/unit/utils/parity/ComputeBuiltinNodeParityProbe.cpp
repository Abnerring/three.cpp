#include "nodes/gpgpu/ComputeBuiltinNode.h"

#include "core/JSValue.h"

#include <iostream>
#include <string>
#include <vector>

namespace {
using ArgList = std::vector<JSValue>;

JSValue makeBuilder(bool hasBuiltin, JSValue& formatted) {
    auto builder = JSValue::object(nullptr);
    builder.set("shaderStage", JSValue("compute"));
    builder.set("format", JSValue::function(
        +[](const JSValue&, const ArgList&, const ArgList& captures) -> JSValue {
            auto holder = captures[0];
            holder.set("value", JSValue(true));
            return JSValue("formatted");
        },
        {formatted}, "format", 3));
    builder.set("generateConst", JSValue::function(
        +[](const JSValue&, const ArgList&, const ArgList&) -> JSValue { return JSValue("const"); },
        {}, "generateConst", 1));
    builder.set("getDataFromNode", JSValue::function(
        +[](const JSValue&, const ArgList&, const ArgList&) -> JSValue { return JSValue::object(nullptr); },
        {}, "getDataFromNode", 1));
    builder.set("getNodeType", JSValue::function(
        +[](const JSValue&, const ArgList&, const ArgList&) -> JSValue { return JSValue("uvec3"); },
        {}, "getNodeType", 2));
    builder.set("hasBuiltin", JSValue::function(
        +[](const JSValue&, const ArgList&, const ArgList& captures) -> JSValue { return captures[0]; },
        {JSValue(hasBuiltin)}, "hasBuiltin", 1));
    return builder;
}
}  // namespace

int main() {
    try {
        const auto module = nativeComputeBuiltinNodeModule();
        const auto ctor = module.get("default");
        const auto builtin = ctor.construct({JSValue("globalId"), JSValue("uvec3")});

        const auto cacheable = builtin.get("isCacheable").call(builtin, {});
        const auto renamed = builtin.get("setBuiltinName").call(builtin, {JSValue("localId")});
        const auto renamedSame = renamed.sameValue(builtin);
        const auto nameAfterSet = builtin.get("_builtinName");
        // put it back for the hash comparison below
        builtin.get("setBuiltinName").call(builtin, {JSValue("globalId")});

        auto formatted = JSValue::object(nullptr);
        formatted.set("value", false);
        const auto builder = makeBuilder(true, formatted);
        const auto hash = builtin.get("getHash").call(builtin, {builder});
        const auto hasBuiltinValue = builtin.get("hasBuiltin").call(builtin, {builder});
        const auto generated = builtin.get("generate").call(builtin, {builder, JSValue("uvec3")});
        const auto formatWasUsed = formatted.get("value");
        const auto generatedType = builtin.get("generateNodeType").call(builtin, {});

        auto outputStage = JSValue::object(nullptr);
        outputStage.set("shaderStage", JSValue("fragment"));
        outputStage.set("getDataFromNode", JSValue::function(
        +[](const JSValue&, const ArgList&, const ArgList&) -> JSValue { return JSValue::object(nullptr); },
        {}, "getDataFromNode", 1));
    outputStage.set("getNodeType", JSValue::function(
        +[](const JSValue&, const ArgList&, const ArgList&) -> JSValue { return JSValue("uvec3"); },
        {}, "getNodeType", 2));
    outputStage.set("generateConst", JSValue::function(
            +[](const JSValue&, const ArgList&, const ArgList&) -> JSValue { return JSValue("const"); },
            {}, "generateConst", 1));
        const auto generatedOther = builtin.get("generate").call(builtin, {outputStage, JSValue("uvec3")});

        auto data = JSValue::object(nullptr);
        builtin.get("serialize").call(builtin, {data});
        const auto serializedGlobal = data.get("global");
        const auto serializedGlobalUndefined = serializedGlobal.isUndefined();
        const auto serializedBuiltin = data.get("_builtinName");

        const auto target = ctor.construct({JSValue("localId"), JSValue("vec3")});
        auto restore = JSValue::object(nullptr);
        restore.set("global", true);
        restore.set("_builtinName", JSValue("globalId"));
        target.get("deserialize").call(target, {restore});

        auto output = JSValue::object(nullptr);
        output.set("name", ctor.get("name"));
        output.set("length", ctor.get("length"));
        output.set("staticType", ctor.get("type"));
        output.set("type", builtin.get("type"));
        output.set("isNode", builtin.get("isNode"));
        output.set("builtinName", builtin.get("_builtinName"));
        output.set("nodeType", builtin.get("nodeType"));
        output.set("cacheable", cacheable);
        output.set("renamedSame", renamedSame);
        output.set("nameAfterSet", nameAfterSet);
        output.set("hash", hash);
        output.set("hasBuiltinValue", hasBuiltinValue);
        output.set("generated", generated);
        output.set("formatWasUsed", formatWasUsed);
        output.set("generatedType", generatedType);
        output.set("generatedOther", generatedOther);
        output.set("serializedBuiltin", serializedBuiltin);
        output.set("serializedGlobalIsUndefined", serializedGlobalUndefined);
        output.set("deserializedBuiltin", target.get("_builtinName"));
        output.set("deserializedGlobal", target.get("global"));
        // The five module level built-ins.
        const char* names[] = {"numWorkgroups", "workgroupId", "globalId", "localId", "subgroupSize"};
        auto builtins = JSValue::object(nullptr);
        for (const auto* name : names) {
            auto entry = JSValue::object(nullptr);
            entry.set("builtinName", module.get(name).get("_builtinName"));
            entry.set("nodeType", module.get(name).get("nodeType"));
            entry.set("isNode", module.get(name).get("isNode"));
            builtins.set(name, entry);
        }
        output.set("builtins", builtins);
        output.set("constructionLength", builtin.get("setBuiltinName").get("length"));
        output.set("source", ctor.get("toString").call(ctor));
        std::cout << output.toJSON()->dump() << '\n';
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "exception: " << error.what() << '\n';
        return 1;
    }
}
