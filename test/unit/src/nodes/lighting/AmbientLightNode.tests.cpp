#include "nodes/lighting/AmbientLightNode.h"

#include "nodes/core/Node.h"

#include "catch.hpp"

#include <vector>

namespace {
using ArgList = std::vector<JSValue>;
}

TEST_CASE("AmbientLightNode exposes the pinned ambient node shape") {
    const auto module = nativeAmbientLightNodeModule();
    const auto ctor = module.get("default");
    REQUIRE(ctor.isCallable());
    REQUIRE(ctor.get("name").strictEquals("AmbientLightNode"));
    // constructor( light = null ) has a default, so Function.length is 0.
    REQUIRE(ctor.get("length").strictEquals(0));
    REQUIRE(ctor.get("type").strictEquals("AmbientLightNode"));
    // The constructor keeps the light it was made with and inherits the analytic node fields.
    const auto node = ctor.construct();
    REQUIRE(node.get("type").strictEquals("AmbientLightNode"));
    REQUIRE(node.get("light").isNull());
    REQUIRE(node.get("isAnalyticLightNode").strictEquals(true));
    REQUIRE(node.get("colorNode").get("isNode").strictEquals(true));
}

TEST_CASE("AmbientLightNode setup adds its colour to the context irradiance") {
    const auto node = nativeAmbientLightNodeConstructor().construct();
    const auto colorNode = node.get("colorNode");

    // A recording irradiance object: addAssign must be reached with exactly this.colorNode.
    auto recorded = JSValue::object(nullptr);
    auto addAssign = JSValue::function(
        +[](const JSValue&, const ArgList& args, const ArgList& captures) -> JSValue {
            // JSValue is a handle: copying it keeps the same heap object as the capture, which is
            // the recording object created below.
            auto target = captures[0];
            target.set("called", true);
            target.set("argument", args.empty() ? JSValue() : args[0]);
            return {};
        },
        {recorded}, "", 1);
    auto irradiance = JSValue::object(nullptr);
    irradiance.set("addAssign", addAssign);
    auto context = JSValue::object(nullptr);
    context.set("irradiance", irradiance);
    auto builder = JSValue::object(nullptr);
    builder.set("context", context);

    node.get("setup").call(node, {builder});

    REQUIRE(recorded.get("called").strictEquals(true));
    REQUIRE(recorded.get("argument").sameValue(colorNode));
}
