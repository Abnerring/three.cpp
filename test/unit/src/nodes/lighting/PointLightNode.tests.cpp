#include "nodes/lighting/PointLightNode.h"

#include "math/Color.h"
#include "nodes/core/Node.h"
#include "nodes/tsl/TSLBase.h"

#include "catch.hpp"

#include <stdexcept>

namespace {
JSValue makeLight(double distance, double decay) {
    auto light = JSValue::object(nullptr);
    auto color = nativeColorConstructor().construct();
    color.set("r", 0.25); color.set("g", 0.5); color.set("b", 0.75);
    light.set("color", color);
    light.set("intensity", 2);
    light.set("distance", distance);
    light.set("decay", decay);
    return light;
}
}  // namespace

TEST_CASE("PointLightNode exposes the pinned point node shape and its two default uniforms") {
    const auto light = makeLight(7, 1.5);
    const auto module = nativePointLightNodeModule();
    const auto ctor = module.get("default");
    REQUIRE(ctor.get("name").strictEquals("PointLightNode"));
    REQUIRE(ctor.get("length").strictEquals(0));
    REQUIRE(ctor.get("type").strictEquals("PointLightNode"));

    const auto node = ctor.construct({light});
    REQUIRE(node.get("type").strictEquals("PointLightNode"));
    REQUIRE(node.get("isAnalyticLightNode").strictEquals(true));
    // The cutoff starts at zero but the decay exponent starts at two, which is not the same default.
    REQUIRE(node.get("cutoffDistanceNode").get("value").toNumber() == Approx(0));
    REQUIRE(node.get("decayExponentNode").get("value").toNumber() == Approx(2));
    for (const char* name : {"cutoffDistanceNode", "decayExponentNode"}) {
        REQUIRE(node.get(name).get("type").strictEquals("UniformNode"));
        REQUIRE(node.get(name).get("groupNode").get("name").strictEquals("render"));
    }
    REQUIRE(node.get("setupDirect").get("length").strictEquals(1));
    REQUIRE(node.get("setupShadowNode").get("length").strictEquals(0));
}

TEST_CASE("PointLightNode update copies the light distance and decay onto the uniforms") {
    const auto light = makeLight(7, 1.5);
    const auto node = nativePointLightNodeConstructor().construct({light});
    node.get("update").call(node, {JSValue::object(nullptr)});
    REQUIRE(node.get("cutoffDistanceNode").get("value").toNumber() == Approx(7));
    REQUIRE(node.get("decayExponentNode").get("value").toNumber() == Approx(1.5));
    // super.update still scales the colour by intensity.
    REQUIRE(node.get("color").get("r").toNumber() == Approx(0.5));
    REQUIRE(node.get("color").get("g").toNumber() == Approx(1.0));
    REQUIRE(node.get("color").get("b").toNumber() == Approx(1.5));
}

TEST_CASE("PointLightNode exports the directPointLight factory") {
    const auto factory = nativeDirectPointLight();
    REQUIRE(factory.isCallable());
    REQUIRE(factory.get("name").strictEquals("directPointLight"));
    // A single destructured parameter, so Function.length is 1.
    REQUIRE(factory.get("length").strictEquals(1));

    // The factory is a node function: given nodes it answers nodes, one of each name.
    auto inputs = JSValue::object(nullptr);
    inputs.set("color", nativeTSLBaseBinding("vec3").call({}, {JSValue(1), JSValue(1), JSValue(1)}));
    inputs.set("lightVector", nativeTSLBaseBinding("vec3").call({}, {JSValue(1), JSValue(2), JSValue(3)}));
    inputs.set("cutoffDistance", nativeTSLBaseBinding("float").call({}, {JSValue(10)}));
    inputs.set("decayExponent", nativeTSLBaseBinding("float").call({}, {JSValue(2)}));
    const auto result = factory.call({}, {inputs});
    REQUIRE(result.get("lightDirection").get("isNode").strictEquals(true));
    REQUIRE(result.get("lightColor").get("isNode").strictEquals(true));
    // The two are different nodes: one is the normalised direction, the other the attenuated colour.
    REQUIRE_FALSE(result.get("lightDirection").sameValue(result.get("lightColor")));
}

TEST_CASE("PointLightNode refuses to set up a shadow it cannot build") {
    const auto light = makeLight(7, 1.5);
    const auto node = nativePointLightNodeConstructor().construct({light});
    // PointShadowNode is not ported, so this must fail loudly rather than answer with nothing.
    REQUIRE_THROWS_AS(node.get("setupShadowNode").call(node, {}), std::runtime_error);
}
