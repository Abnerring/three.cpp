#include "nodes/gpgpu/WorkgroupInfoNode.h"

#include "core/JSValue.h"

#include <iostream>
#include <string>
#include <vector>

namespace {
using ArgList = std::vector<JSValue>;

JSValue recordingBuilder(JSValue& captured, bool computeStage = true) {
    auto builder = JSValue::object(nullptr);
    builder.set("shaderStage", JSValue(computeStage ? "compute" : "fragment"));
    builder.set("getScopedArray", JSValue::function(
        +[](const JSValue&, const ArgList& args, const ArgList& captures) -> JSValue {
            auto holder = captures[0];
            auto record = JSValue::array();
            for (std::size_t i = 0; i < args.size(); ++i) record.set(i, args[i]);
            holder.set("args", record);
            return JSValue("scoped");
        },
        {captured}, "getScopedArray", 5));
    return builder;
}
}  // namespace

int main() {
    try {
        const auto module = nativeWorkgroupInfoNodeModule();
        const auto ctor = module.get("default");
        const auto elementCtor = nativeWorkgroupInfoElementNodeConstructor();

        const auto info = module.get("workgroupArray").call({}, {JSValue("float"), JSValue(4)});
        const auto defaulted = module.get("workgroupArray").call({}, {JSValue("uint")});

        const auto byCtor = ctor.construct({JSValue("Workgroup"), JSValue("int"), JSValue(8)});
        const auto setNameResult = byCtor.get("setName").call(byCtor, {JSValue("myArray")});
        const auto setNameSame = setNameResult.sameValue(byCtor);
        const auto nameAfterSet = byCtor.get("name");
        const auto setScopeResult = byCtor.get("setScope").call(byCtor, {JSValue("Storage")});
        const auto setScopeSame = setScopeResult.sameValue(byCtor);
        const auto inputTypeAfterScope = byCtor.get("getInputType").call(byCtor, {});
        const auto elementType = byCtor.get("getElementType").call(byCtor, {});
        const auto toAtomicResult = byCtor.get("toAtomic").call(byCtor, {});
        const auto toAtomicSame = toAtomicResult.sameValue(byCtor);
        const auto atomicFlag = byCtor.get("isAtomic");
        const auto labelResult = byCtor.get("label").call(byCtor, {JSValue("older")});
        const auto labelSame = labelResult.sameValue(byCtor);
        const auto nameAfterLabel = byCtor.get("name");

        auto index = JSValue::object(nullptr);
        const auto element = byCtor.get("element").call(byCtor, {index});
        const auto elementIndexSame = element.get("indexNode").sameValue(index);

        // generate on the shared module node
        auto captured = JSValue::object(nullptr);
        const auto builder = recordingBuilder(captured);
        const auto generated = info.get("generate").call(info, {builder});
        auto args = captured.get("args");
        const auto generatedName = args.get("0").toString();
        const auto named = nativeWorkgroupInfoNodeConstructor().construct(
            {JSValue("Workgroup"), JSValue("float"), JSValue(4)});
        named.get("setName").call(named, {JSValue("named")});
        named.get("toAtomic").call(named, {});
        auto capturedNamed = JSValue::object(nullptr);
        const auto namedBuilder = recordingBuilder(capturedNamed);
        const auto generatedNamed = named.get("generate").call(named, {namedBuilder});
        auto namedArgs = capturedNamed.get("args");
        auto capturedOther = JSValue::object(nullptr);
        auto otherBuilder = recordingBuilder(capturedOther, false);
        const auto generatedOther = named.get("generate").call(named, {otherBuilder});

        // element generate needs a real array element generation, so it is recorded as raised or not.
        bool elementGenerated = true;
        auto elementBuilder = JSValue::object(nullptr);
        elementBuilder.set("isContextAssign", JSValue::function(
            +[](const JSValue&, const ArgList&, const ArgList&) -> JSValue { return JSValue(true); },
            {}, "isContextAssign", 0));
        try {
            element.get("generate").call(element, {elementBuilder, JSValue("float")});
        } catch (...) {
            elementGenerated = false;
        }

        auto output = JSValue::object(nullptr);
        output.set("name", ctor.get("name"));
        output.set("length", ctor.get("length"));
        output.set("elementName", elementCtor.get("name"));
        output.set("elementLength", elementCtor.get("length"));
        output.set("factoryLength", module.get("workgroupArray").get("length"));
        output.set("type", info.get("type"));
        output.set("isWorkgroupInfoNode", info.get("isWorkgroupInfoNode"));
        output.set("scope", info.get("scope"));
        output.set("bufferType", info.get("bufferType"));
        output.set("bufferCount", info.get("bufferCount"));
        output.set("elementTypeField", info.get("elementType"));
        output.set("nameField", info.get("name"));
        output.set("isAtomic", info.get("isAtomic"));
        output.set("defaultedCount", defaulted.get("bufferCount"));
        output.set("cacheable", info.get("isCacheable").call(info, {}));
        output.set("inputTypeDefault", info.get("getInputType").call(info, {}));
        output.set("setNameSame", setNameSame);
        output.set("nameAfterSet", nameAfterSet);
        output.set("setScopeSame", setScopeSame);
        output.set("inputTypeAfterScope", inputTypeAfterScope);
        output.set("elementType", elementType);
        output.set("toAtomicSame", toAtomicSame);
        output.set("atomicFlag", atomicFlag);
        output.set("labelSame", labelSame);
        output.set("nameAfterLabel", nameAfterLabel);
        output.set("elementFlag", element.get("isWorkgroupInfoElementNode"));
        output.set("elementIsNode", element.get("isNode"));
        output.set("elementIndexSame", elementIndexSame);
        output.set("generated", generated);
        output.set("generatedNamePrefix", generatedName.substr(0, 15));
        output.set("generatedScope", args.get("1"));
        output.set("generatedType", args.get("2"));
        output.set("generatedCount", args.get("3"));
        output.set("generatedAtomic", args.get("4"));
        output.set("generatedNamed", generatedNamed);
        output.set("namedName", namedArgs.get("0"));
        output.set("namedAtomic", namedArgs.get("4"));
        output.set("generatedOther", generatedOther);
        output.set("elementGenerated", elementGenerated);
        output.set("methodLengths", [&]() {
            auto lengths = JSValue::object(nullptr);
            for (const auto* name : {"isCacheable", "setName", "label", "setScope", "setAtomic",
                                     "toAtomic", "getElementType", "getInputType", "element",
                                     "generate"}) {
                lengths.set(name, info.get(name).get("length"));
            }
            return lengths;
        }());
        output.set("source", ctor.get("toString").call(ctor));
        output.set("elementSource", elementCtor.get("toString").call(elementCtor));
        std::cout << output.toJSON()->dump() << '\n';
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "exception: " << error.what() << '\n';
        return 1;
    }
}
