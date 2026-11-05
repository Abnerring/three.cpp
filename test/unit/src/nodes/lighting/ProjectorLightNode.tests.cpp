#include "nodes/lighting/ProjectorLightNode.h"

#include "math/Color.h"
#include "math/JSNumberMath.h"
#include "nodes/core/Node.h"

#include "catch.hpp"

#include <algorithm>

namespace {
// The light the projector's update reads. A plain object is enough: the light accessors key a weak
// map on it and the update only reads fields.
JSValue makeLight(double angle, double penumbra, const JSValue& aspect, const JSValue& map) {
    auto light = JSValue::object(nullptr);
    auto color = nativeColorConstructor().construct();
    color.set("r", 0.25); color.set("g", 0.5); color.set("b", 0.75);
    light.set("color", color);
    light.set("intensity", 2);
    light.set("angle", angle);
    light.set("penumbra", penumbra);
    light.set("aspect", aspect);
    light.set("map", map);
    light.set("shadow", JSValue::object(nullptr));
    return light;
}
}  // namespace

TEST_CASE("ProjectorLightNode exposes the pinned projector shape") {
    const auto light = makeLight(0.5, 0.25, nullptr, nullptr);
    const auto module = nativeProjectorLightNodeModule();
    const auto ctor = module.get("default");
    REQUIRE(ctor.isCallable());
    REQUIRE(ctor.get("name").strictEquals("ProjectorLightNode"));
    REQUIRE(ctor.get("length").strictEquals(0));
    REQUIRE(ctor.get("type").strictEquals("ProjectorLightNode"));

    const auto node = ctor.construct({light});
    REQUIRE(node.get("type").strictEquals("ProjectorLightNode"));
    REQUIRE(node.get("isAnalyticLightNode").strictEquals(true));
    REQUIRE(node.get("getSpotAttenuation").get("length").strictEquals(1));
    REQUIRE(node.get("update").get("length").strictEquals(1));
}

TEST_CASE("ProjectorLightNode update clamps the penumbra cosine and derives a unit aspect") {
    const auto light = makeLight(0.5, 0.25, nullptr, nullptr);
    const auto node = nativeProjectorLightNodeConstructor().construct({light});
    node.get("update").call(node, {JSValue::object(nullptr)});

    // super.update writes the plain cone cosine and the colour scaled by intensity.
    REQUIRE(node.get("coneCosNode").get("value").toNumber() ==
            Approx(JSNumberMath::cos(0.5)).epsilon(1e-15));
    REQUIRE(node.get("color").get("r").toNumber() == Approx(0.5));
    // The projector replaces the penumbra cosine with a value clamped below one.
    const auto expected = std::min(JSNumberMath::cos(0.5 * (1 - 0.25)), 0.99999);
    REQUIRE(node.get("penumbraCosNode").get("value").toNumber() == Approx(expected).epsilon(1e-15));
    REQUIRE(node.get("penumbraCosNode").get("value").toNumber() <= 0.99999);
    // With no aspect and no map the shadow aspect falls back to one.
    REQUIRE(light.get("shadow").get("aspect").toNumber() == Approx(1.0));
}

TEST_CASE("ProjectorLightNode update takes the aspect from the map or from the light") {
    auto map = JSValue::object(nullptr);
    map.set("width", 4);
    map.set("height", 2);
    const auto withMap = makeLight(0.5, 0.25, nullptr, map);
    const auto nodeWithMap = nativeProjectorLightNodeConstructor().construct({withMap});
    nodeWithMap.get("update").call(nodeWithMap, {JSValue::object(nullptr)});
    REQUIRE(withMap.get("shadow").get("aspect").toNumber() == Approx(2.0));

    const auto explicitAspect = makeLight(0.5, 0.25, JSValue(1.5), nullptr);
    const auto nodeWithAspect = nativeProjectorLightNodeConstructor().construct({explicitAspect});
    nodeWithAspect.get("update").call(nodeWithAspect, {JSValue::object(nullptr)});
    REQUIRE(explicitAspect.get("shadow").get("aspect").toNumber() == Approx(1.5));
}

TEST_CASE("ProjectorLightNode clamps a wide penumbra cosine to the fixed ceiling") {
    // A penumbra of zero and a tiny angle give a cosine above the ceiling, so the clamp is the value
    // that survives. cos( 0.1 ) is about 0.995 and is below the ceiling, so the angle has to be much
    // smaller for the clamp to be reached at all.
    const auto light = makeLight(0.001, 0.0, nullptr, nullptr);
    const auto node = nativeProjectorLightNodeConstructor().construct({light});
    node.get("update").call(node, {JSValue::object(nullptr)});
    REQUIRE(JSNumberMath::cos(0.001) > 0.99999);
    REQUIRE(node.get("penumbraCosNode").get("value").toNumber() == Approx(0.99999));
}
