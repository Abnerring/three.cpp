#include "nodes/gpgpu/WorkgroupInfoNode.h"

#include "nodes/core/Node.h"

#include "catch.hpp"

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

TEST_CASE("WorkgroupInfoNode keeps its scope, buffer type and count") {
    const auto module = nativeWorkgroupInfoNodeModule();
    const auto ctor = module.get("default");
    REQUIRE(ctor.get("name").strictEquals("WorkgroupInfoNode"));
    REQUIRE(ctor.get("length").strictEquals(2));
    REQUIRE(module.get("workgroupArray").get("length").strictEquals(2));
    REQUIRE(nativeWorkgroupInfoElementNodeConstructor().get("name").strictEquals("WorkgroupInfoElementNode"));
    REQUIRE(nativeWorkgroupInfoElementNodeConstructor().get("length").strictEquals(2));

    const auto info = module.get("workgroupArray").call({}, {JSValue("float"), JSValue(4)});
    // The class declares no static type getter, so the type resolves through the base Node class.
    REQUIRE(info.get("type").strictEquals("Node"));
    REQUIRE(info.get("isWorkgroupInfoNode").strictEquals(true));
    REQUIRE(info.get("scope").strictEquals("Workgroup"));
    REQUIRE(info.get("bufferType").strictEquals("float"));
    REQUIRE(info.get("bufferCount").toNumber() == Approx(4));
    REQUIRE(info.get("elementType").strictEquals("float"));
    REQUIRE(info.get("name").strictEquals(""));
    REQUIRE(info.get("isAtomic").strictEquals(false));
    // bufferCount defaults to zero when it is omitted.
    const auto defaulted = module.get("workgroupArray").call({}, {JSValue("uint")});
    REQUIRE(defaulted.get("bufferCount").toNumber() == Approx(0));
    // The hooks that take no builder are zero arity.
    REQUIRE(info.get("isCacheable").get("length").strictEquals(0));
    REQUIRE(info.get("getInputType").get("length").strictEquals(0));
}

TEST_CASE("WorkgroupInfoNode setters answer the node and update the fields") {
    const auto info = nativeWorkgroupInfoNodeConstructor().construct(
        {JSValue("Workgroup"), JSValue("float"), JSValue(8)});
    REQUIRE(info.get("setName").call(info, {JSValue("myArray")}).sameValue(info));
    REQUIRE(info.get("name").strictEquals("myArray"));
    REQUIRE(info.get("setScope").call(info, {JSValue("Storage")}).sameValue(info));
    REQUIRE(info.get("scope").strictEquals("Storage"));
    REQUIRE(info.get("getInputType").call(info, {}).strictEquals("StorageArray"));
    REQUIRE(info.get("getElementType").call(info, {}).strictEquals("float"));
    // toAtomic goes through setAtomic and answers the node.
    REQUIRE(info.get("toAtomic").call(info, {}).sameValue(info));
    REQUIRE(info.get("isAtomic").strictEquals(true));
    // label is deprecated but still renames through setName.
    const auto labelled = info.get("label").call(info, {JSValue("older")});
    REQUIRE(labelled.sameValue(info));
    REQUIRE(info.get("name").strictEquals("older"));
}

TEST_CASE("WorkgroupInfoNode element builds a workgroup element node") {
    const auto info = nativeWorkgroupInfoNodeConstructor().construct(
        {JSValue("Workgroup"), JSValue("int"), JSValue(4)});
    auto index = JSValue::object(nullptr);
    const auto element = info.get("element").call(info, {index});
    REQUIRE(element.get("isWorkgroupInfoElementNode").strictEquals(true));
    REQUIRE(element.get("isNode").strictEquals(true));
    REQUIRE(element.get("indexNode").sameValue(index));
    REQUIRE(element.get("getNodeType").isCallable());
}

TEST_CASE("WorkgroupInfoNode generate asks the builder for a scoped array") {
    const auto info = nativeWorkgroupInfoNodeConstructor().construct(
        {JSValue("Workgroup"), JSValue("float"), JSValue(4)});
    auto captured = JSValue::object(nullptr);
    const auto builder = recordingBuilder(captured);
    // Unnamed uses the scope plus the node id.
    REQUIRE(info.get("generate").call(info, {builder}).strictEquals("scoped"));
    auto args = captured.get("args");
    const auto generatedName = args.get("0").toString();
    REQUIRE(generatedName.find("WorkgroupArray_") == 0);
    REQUIRE(args.get("1").strictEquals("workgroup"));
    REQUIRE(args.get("2").strictEquals("float"));
    REQUIRE(args.get("3").toNumber() == Approx(4));
    REQUIRE(args.get("4").strictEquals(false));

    // A name overrides the generated one, and atomicity is forwarded.
    info.get("setName").call(info, {JSValue("named")});
    info.get("toAtomic").call(info, {});
    auto capturedNamed = JSValue::object(nullptr);
    const auto namedBuilder = recordingBuilder(capturedNamed);
    REQUIRE(info.get("generate").call(info, {namedBuilder}).strictEquals("scoped"));
    auto namedArgs = capturedNamed.get("args");
    REQUIRE(namedArgs.get("0").strictEquals("named"));
    REQUIRE(namedArgs.get("4").strictEquals(true));

    // Outside the compute stage it reports an error but still answers the scoped array.
    auto capturedOther = JSValue::object(nullptr);
    const auto otherBuilder = recordingBuilder(capturedOther, false);
    REQUIRE(info.get("generate").call(info, {otherBuilder}).strictEquals("scoped"));
}
