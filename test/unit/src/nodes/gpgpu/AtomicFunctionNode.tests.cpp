#include "nodes/gpgpu/AtomicFunctionNode.h"

#include "nodes/core/Node.h"

#include "catch.hpp"

#include <string>
#include <vector>

namespace {
using ArgList = std::vector<JSValue>;

JSValue pointerStub() {
    auto pointer = JSValue::object(nullptr);
    pointer.set("getNodeType", JSValue::function(
        +[](const JSValue&, const ArgList&, const ArgList&) -> JSValue { return JSValue("uint"); },
        {}, "getNodeType", 1));
    pointer.set("build", JSValue::function(
        +[](const JSValue&, const ArgList&, const ArgList&) -> JSValue { return JSValue("ptr"); },
        {}, "build", 2));
    return pointer;
}

JSValue valueStub() {
    auto value = JSValue::object(nullptr);
    value.set("build", JSValue::function(
        +[](const JSValue&, const ArgList&, const ArgList&) -> JSValue { return JSValue("val"); },
        {}, "build", 2));
    return value;
}

// A builder stub with everything the generate path asks for; lines records the flow code it receives.
JSValue generateBuilder(const JSValue& parents, JSValue& lines) {
    auto builder = JSValue::object(nullptr);
    builder.set("shaderStage", JSValue("compute"));
    builder.set("getNodeProperties", JSValue::function(
        +[](const JSValue&, const ArgList&, const ArgList& captures) -> JSValue {
            auto properties = JSValue::object(nullptr);
            properties.set("parents", captures[0]);
            return properties;
        },
        {parents}, "getNodeProperties", 1));
    builder.set("getDataFromNode", JSValue::function(
        +[](const JSValue&, const ArgList&, const ArgList&) -> JSValue {
            auto data = JSValue::object(nullptr);
            data.set("nodeType", JSValue("uint"));
            return data;
        },
        {}, "getDataFromNode", 1));
    builder.set("getNodeType", JSValue::function(
        +[](const JSValue&, const ArgList&, const ArgList&) -> JSValue { return JSValue("uint"); },
        {}, "getNodeType", 2));
    builder.set("getMethod", JSValue::function(
        +[](const JSValue&, const ArgList& args, const ArgList&) -> JSValue {
            return args.empty() ? JSValue("atomicAdd") : args[0];
        },
        {}, "getMethod", 2));
    builder.set("getVarFromNode", JSValue::function(
        +[](const JSValue&, const ArgList&, const ArgList&) -> JSValue {
            return JSValue::object(nullptr);
        },
        {}, "getVarFromNode", 6));
    builder.set("getPropertyName", JSValue::function(
        +[](const JSValue&, const ArgList&, const ArgList&) -> JSValue { return JSValue("v0"); },
        {}, "getPropertyName", 1));
    builder.set("generateLetStatement", JSValue::function(
        +[](const JSValue&, const ArgList& args, const ArgList&) -> JSValue {
            const auto type = args.empty() ? JSValue("uint") : args[0];
            const auto name = args.size() < 2 ? JSValue("v0") : args[1];
            return JSValue("let " + type.toString() + " " + name.toString());
        },
        {}, "generateLetStatement", 2));
    builder.set("addLineFlowCode", JSValue::function(
        +[](const JSValue&, const ArgList& args, const ArgList& captures) -> JSValue {
            // The captured value is the lines array itself, so append to it directly.
            auto lines = captures[0];
            const auto snippet = args.empty() ? JSValue("") : args[0];
            lines.set(lines.length(), snippet);
            return {};
        },
        {lines}, "addLineFlowCode", 2));
    return builder;
}
}  // namespace

TEST_CASE("AtomicFunctionNode keeps the method name and its operands") {
    const auto module = nativeAtomicFunctionNodeModule();
    const auto ctor = module.get("default");
    REQUIRE(ctor.get("name").strictEquals("AtomicFunctionNode"));
    REQUIRE(ctor.get("length").strictEquals(3));
    REQUIRE(ctor.get("type").strictEquals("AtomicFunctionNode"));
    // The nine static method name constants.
    REQUIRE(ctor.get("ATOMIC_LOAD").strictEquals("atomicLoad"));
    REQUIRE(ctor.get("ATOMIC_STORE").strictEquals("atomicStore"));
    REQUIRE(ctor.get("ATOMIC_ADD").strictEquals("atomicAdd"));
    REQUIRE(ctor.get("ATOMIC_SUB").strictEquals("atomicSub"));
    REQUIRE(ctor.get("ATOMIC_MAX").strictEquals("atomicMax"));
    REQUIRE(ctor.get("ATOMIC_MIN").strictEquals("atomicMin"));
    REQUIRE(ctor.get("ATOMIC_AND").strictEquals("atomicAnd"));
    REQUIRE(ctor.get("ATOMIC_OR").strictEquals("atomicOr"));
    REQUIRE(ctor.get("ATOMIC_XOR").strictEquals("atomicXor"));

    const auto pointer = pointerStub();
    const auto value = valueStub();
    const auto atomic = ctor.construct({JSValue("atomicAdd"), pointer, value});
    REQUIRE(atomic.get("type").strictEquals("AtomicFunctionNode"));
    REQUIRE(atomic.get("nodeType").strictEquals("uint"));
    REQUIRE(atomic.get("method").strictEquals("atomicAdd"));
    REQUIRE(atomic.get("pointerNode").sameValue(pointer));
    REQUIRE(atomic.get("valueNode").sameValue(value));
    // parents is set to true by the constructor.
    REQUIRE(atomic.get("parents").strictEquals(true));
    REQUIRE(atomic.get("isCacheable").call(atomic, {}).strictEquals(false));
    // getInputType and generateNodeType both answer the pointer node type.
    auto builder = JSValue::object(nullptr);
    REQUIRE(atomic.get("getInputType").call(atomic, {builder}).strictEquals("uint"));
    REQUIRE(atomic.get("generateNodeType").call(atomic, {builder}).strictEquals("uint"));
    REQUIRE(atomic.get("generate").get("length").strictEquals(1));
}

// NOTE: the generate path is implemented faithfully but is NOT exercised yet. Calling it from a test against a
// stub builder raises an access violation inside this runtime, which is recorded as an open issue for a later
// round rather than hidden behind a passing test.

TEST_CASE("AtomicFunctionNode answers the toStack chain method") {
    const auto atomic = nativeAtomicFunctionNodeConstructor().construct(
        {JSValue("atomicAdd"), pointerStub(), valueStub()});
    // toStack is registered through addMethodChaining in TSLCore, so every node carries it.
    const auto toStack = atomic.get("toStack");
    REQUIRE(toStack.isCallable());
    REQUIRE(toStack.call(atomic, {}).sameValue(atomic));
}

TEST_CASE("AtomicFunctionNode module factories build nodes") {
    const auto module = nativeAtomicFunctionNodeModule();
    const auto pointer = pointerStub();
    // atomicLoad goes through nodeProxy and toStack, both of which work now.
    JSValue loaded;
    REQUIRE_NOTHROW(loaded = module.get("atomicLoad").call({}, {pointer}));
    REQUIRE(loaded.get("isNode").strictEquals(true));
    REQUIRE(loaded.get("nodeType").strictEquals("uint"));
    REQUIRE(loaded.get("method").strictEquals("atomicLoad"));
    // The value form carries the value node as well.
    JSValue stored;
    REQUIRE_NOTHROW(stored = module.get("atomicAdd").call({}, {pointer, valueStub()}));
    REQUIRE(stored.get("method").strictEquals("atomicAdd"));
    REQUIRE(stored.get("valueNode").isNull() == false);
}