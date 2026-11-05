#include "nodes/lighting/SpotLightNode.h"

#include "math/Color.h"
#include "math/JSNumberMath.h"
#include "nodes/core/Node.h"
#include "nodes/core/UniformNode.h"

#include "catch.hpp"

TEST_CASE("SpotLightNode exposes the pinned spot node shape") {
    auto light = JSValue::object(nullptr);
    const auto module = nativeSpotLightNodeModule();
    const auto ctor = module.get("default");
    REQUIRE(ctor.isCallable());
    REQUIRE(ctor.get("name").strictEquals("SpotLightNode"));
    // constructor( light = null ) has a default, so Function.length is 0.
    REQUIRE(ctor.get("length").strictEquals(0));
    REQUIRE(ctor.get("type").strictEquals("SpotLightNode"));

    const auto node = ctor.construct({light});
    REQUIRE(node.get("type").strictEquals("SpotLightNode"));
    REQUIRE(node.get("isAnalyticLightNode").strictEquals(true));
    REQUIRE(node.get("light").sameValue(light));
    // Five uniform nodes in the render group: four scalars and the colour.
    for (const char* name : {"coneCosNode", "penumbraCosNode", "cutoffDistanceNode",
                             "decayExponentNode", "colorNode"}) {
        REQUIRE(node.get(name).get("type").strictEquals("UniformNode"));
        REQUIRE(node.get(name).get("groupNode").get("name").strictEquals("render"));
    }
    // The method arities the source declares.
    REQUIRE(node.get("getSpotAttenuation").get("length").strictEquals(2));
    REQUIRE(node.get("getLightCoord").get("length").strictEquals(1));
    REQUIRE(node.get("setupDirect").get("length").strictEquals(1));
}

TEST_CASE("SpotLightNode update writes the four cone uniforms and the colour") {
    auto light = JSValue::object(nullptr);
    auto color = nativeColorConstructor().construct();
    color.set("r", 0.25); color.set("g", 0.5); color.set("b", 1.0);
    light.set("color", color);
    light.set("intensity", 2);
    light.set("angle", 0.5);
    light.set("penumbra", 0.25);
    light.set("distance", 12.5);
    light.set("decay", 1.5);

    const auto node = nativeSpotLightNodeConstructor().construct({light});
    node.get("update").call(node, {JSValue::object(nullptr)});

    // Math.cos is the pinned fdlibm implementation rather than the platform libm.
    REQUIRE(node.get("coneCosNode").get("value").toNumber() ==
            Approx(JSNumberMath::cos(0.5)).epsilon(1e-15));
    REQUIRE(node.get("penumbraCosNode").get("value").toNumber() ==
            Approx(JSNumberMath::cos(0.5 * (1 - 0.25))).epsilon(1e-15));
    // The other two pass the light's own values through unchanged.
    REQUIRE(node.get("cutoffDistanceNode").get("value").toNumber() == Approx(12.5));
    REQUIRE(node.get("decayExponentNode").get("value").toNumber() == Approx(1.5));
    // super.update: colour scaled by intensity.
    REQUIRE(node.get("color").get("r").toNumber() == Approx(0.5));
    REQUIRE(node.get("color").get("g").toNumber() == Approx(1.0));
    REQUIRE(node.get("color").get("b").toNumber() == Approx(2.0));
}

TEST_CASE("SpotLightNode getSpotAttenuation builds a smoothed step from the two cone cosines") {
    auto light = JSValue::object(nullptr);
    const auto node = nativeSpotLightNodeConstructor().construct({light});
    const auto angleCos = nativeUniformFactory().call({}, {JSValue(0.5)});
    const auto attenuation = node.get("getSpotAttenuation").call(node, {JSValue::object(nullptr), angleCos});
    // A node is produced, and it is neither of the cone uniforms nor the angle it was given: the
    // smoothstep wraps them. Its unbuilt type is not asserted, because the source computes that
    // from the operands only when a builder asks, which is the same in three.js.
    REQUIRE(attenuation.get("isNode").strictEquals(true));
    REQUIRE_FALSE(attenuation.sameValue(node.get("coneCosNode")));
    REQUIRE_FALSE(attenuation.sameValue(node.get("penumbraCosNode")));
    REQUIRE_FALSE(attenuation.sameValue(angleCos));
    // Two calls produce two nodes; nothing here caches the step.
    const auto again = node.get("getSpotAttenuation").call(node, {JSValue::object(nullptr), angleCos});
    REQUIRE_FALSE(again.sameValue(attenuation));
}
