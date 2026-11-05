#include "nodes/lighting/HemisphereLightNode.h"

#include "math/Color.h"
#include "nodes/core/Node.h"

#include "catch.hpp"

#include <vector>

namespace {
using ArgList = std::vector<JSValue>;
}

TEST_CASE("HemisphereLightNode exposes the pinned hemisphere node shape") {
    // A plain object is a valid weak-map key for the light accessors, while null is not, so the
    // node is built with an object here rather than with the default.
    auto light = JSValue::object(nullptr);
    const auto module = nativeHemisphereLightNodeModule();
    const auto ctor = module.get("default");
    REQUIRE(ctor.isCallable());
    REQUIRE(ctor.get("name").strictEquals("HemisphereLightNode"));
    REQUIRE(ctor.get("length").strictEquals(0));
    REQUIRE(ctor.get("type").strictEquals("HemisphereLightNode"));

    const auto node = ctor.construct({light});
    REQUIRE(node.get("type").strictEquals("HemisphereLightNode"));
    REQUIRE(node.get("isAnalyticLightNode").strictEquals(true));
    REQUIRE(node.get("light").sameValue(light));
    // The three fields the constructor adds.
    REQUIRE(node.get("lightPositionNode").get("type").strictEquals("UniformNode"));
    REQUIRE(node.get("lightPositionNode").get("groupNode").get("name").strictEquals("render"));
    REQUIRE(node.get("lightDirectionNode").get("isNode").strictEquals(true));
    REQUIRE(node.get("groundColorNode").get("type").strictEquals("UniformNode"));
    REQUIRE(node.get("groundColorNode").get("groupNode").get("name").strictEquals("render"));
}

TEST_CASE("HemisphereLightNode update scales the colour and the ground colour by intensity") {
    auto light = JSValue::object(nullptr);
    auto color = nativeColorConstructor().construct();
    color.set("r", 0.25); color.set("g", 0.5); color.set("b", 0.75);
    auto ground = nativeColorConstructor().construct();
    ground.set("r", 1.0); ground.set("g", 0.5); ground.set("b", 0.0);
    light.set("color", color);
    light.set("groundColor", ground);
    light.set("intensity", 2);

    const auto node = nativeHemisphereLightNodeConstructor().construct({light});
    node.get("update").call(node, {JSValue::object(nullptr)});

    // super.update copies the light colour and multiplies by intensity.
    const auto nodeColor = node.get("color");
    REQUIRE(nodeColor.get("r").toNumber() == Approx(0.5));
    REQUIRE(nodeColor.get("g").toNumber() == Approx(1.0));
    REQUIRE(nodeColor.get("b").toNumber() == Approx(1.5));
    // The ground colour is scaled the same way, and it is a different object from the light's.
    const auto nodeGround = node.get("groundColorNode").get("value");
    REQUIRE(nodeGround.get("r").toNumber() == Approx(2.0));
    REQUIRE(nodeGround.get("g").toNumber() == Approx(1.0));
    REQUIRE(nodeGround.get("b").toNumber() == Approx(0.0));
    REQUIRE_FALSE(nodeGround.sameValue(ground));
    // The position node is pointed at the light it belongs to.
    REQUIRE(node.get("lightPositionNode").get("object3d").sameValue(light));
}

TEST_CASE("HemisphereLightNode setup adds a mixed irradiance to the context") {
    auto light = JSValue::object(nullptr);
    const auto node = nativeHemisphereLightNodeConstructor().construct({light});

    auto recorded = JSValue::object(nullptr);
    auto addAssign = JSValue::function(
        +[](const JSValue&, const ArgList& args, const ArgList& captures) -> JSValue {
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
    // mix( groundColorNode, colorNode, weight ) produces a node rather than a number.
    REQUIRE(recorded.get("argument").get("isNode").strictEquals(true));
}
