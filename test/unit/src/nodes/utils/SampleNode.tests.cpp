#include "nodes/utils/SampleNode.h"

#include "nodes/core/Node.h"
#include "nodes/tsl/TSLBase.h"

#include "catch.hpp"

#include <vector>

namespace {
using ArgList = std::vector<JSValue>;

// A callback that counts its calls and remembers the uv node it was handed.
JSValue countingCallback(const JSValue& state) {
    return JSValue::function(
        +[](const JSValue&, const ArgList& args, const ArgList& captures) -> JSValue {
            auto holder = captures[0];
            holder.set("count", JSValue(holder.get("count").toNumber() + 1));
            holder.set("lastArg", args.size() > 0 ? args[0] : JSValue{});
            return captures[1];
        },
        {state, nativeTSLBaseBinding("float").call({}, {JSValue(2.0)})}, "callback", 1);
}
}  // namespace

TEST_CASE("SampleNode keeps the pinned shape and is not cacheable") {
    const auto module = nativeSampleNodeModule();
    const auto ctor = module.get("default");
    REQUIRE(ctor.get("name").strictEquals("SampleNode"));
    // constructor( callback, uvNode = null ) has one required parameter.
    REQUIRE(ctor.get("length").strictEquals(1));
    REQUIRE(ctor.get("type").strictEquals("SampleNode"));

    auto state = JSValue::object(nullptr);
    state.set("count", 0);
    const auto node = ctor.construct({countingCallback(state)});
    REQUIRE(node.get("isSampleNode").strictEquals(true));
    REQUIRE(node.get("uvNode").isNull());
    REQUIRE(node.get("callback").isCallable());
    REQUIRE(node.get("isCacheable").call(node, {}).strictEquals(false));
    // The module also exports the factory, whose second parameter has a default.
    const auto factory = nativeSampleFunction();
    REQUIRE(factory.isCallable());
    // Only the uv parameter is defaulted, so the factory length is one.\n    REQUIRE(factory.get("length").strictEquals(1));
    REQUIRE(factory.get("name").strictEquals("sample"));
}

TEST_CASE("SampleNode sample delegates to the callback and setup passes the uv node") {
    auto state = JSValue::object(nullptr);
    state.set("count", 0);
    const auto node = nativeSampleNodeConstructor().construct({countingCallback(state)});

    auto uvArgument = nativeTSLBaseBinding("float").call({}, {JSValue(0.5)});
    const auto sampled = node.get("sample").call(node, {uvArgument});
    REQUIRE(state.get("count").toNumber() == Approx(1));
    // The callback receives exactly the uv it was given, and its answer is passed through.
    REQUIRE(state.get("lastArg").sameValue(uvArgument));
    REQUIRE(sampled.get("isNode").strictEquals(true));

    const auto set = node.get("setup").call(node, {});
    REQUIRE(state.get("count").toNumber() == Approx(2));
    // setup samples the default uv accessor, so what the callback sees is a node again.
    REQUIRE(state.get("lastArg").get("isNode").strictEquals(true));
    REQUIRE(set.get("isNode").strictEquals(true));
}

TEST_CASE("SampleNode factory wraps a node object and keeps the callback") {
    auto state = JSValue::object(nullptr);
    state.set("count", 0);
    const auto callback = countingCallback(state);
    const auto node = nativeSampleFunction().call({}, {callback});
    REQUIRE(node.get("isSampleNode").strictEquals(true));
    REQUIRE(node.get("callback").sameValue(callback));
    node.get("sample").call(node, {nativeTSLBaseBinding("float").call({}, {JSValue(1.0)})});
    REQUIRE(state.get("count").toNumber() == Approx(1));
}
