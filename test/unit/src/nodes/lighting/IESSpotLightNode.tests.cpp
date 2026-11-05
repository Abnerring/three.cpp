#include "nodes/lighting/IESSpotLightNode.h"

#include "nodes/core/Node.h"

#include "catch.hpp"

TEST_CASE("IESSpotLightNode exposes the pinned IES spot shape") {
    auto light = JSValue::object(nullptr);
    const auto module = nativeIESSpotLightNodeModule();
    const auto ctor = module.get("default");
    REQUIRE(ctor.isCallable());
    REQUIRE(ctor.get("name").strictEquals("IESSpotLightNode"));
    // constructor( light = null ) has a default, so Function.length is 0.
    REQUIRE(ctor.get("length").strictEquals(0));
    REQUIRE(ctor.get("type").strictEquals("IESSpotLightNode"));

    const auto node = ctor.construct({light});
    REQUIRE(node.get("type").strictEquals("IESSpotLightNode"));
    REQUIRE(node.get("isAnalyticLightNode").strictEquals(true));
    // The inherited spot uniforms are still there, and the IES node starts absent.
    for (const char* name : {"coneCosNode", "penumbraCosNode", "cutoffDistanceNode",
                             "decayExponentNode", "colorNode"}) {
        REQUIRE(node.get(name).get("type").strictEquals("UniformNode"));
    }
    REQUIRE(node.get("_iesTextureNode").isNull());
    REQUIRE(node.get("getSpotAttenuation").get("length").strictEquals(2));
    REQUIRE(node.get("update").get("length").strictEquals(1));
}

TEST_CASE("IESSpotLightNode falls back to the spot attenuation when there is no IES map") {
    auto light = JSValue::object(nullptr);
    const auto node = nativeIESSpotLightNodeConstructor().construct({light});
    const auto angleCos = node.get("coneCosNode");
    const auto attenuation = node.get("getSpotAttenuation").call(node, {JSValue::object(nullptr), angleCos});
    // The source takes the else branch and returns the parent's smoothstep over the cone cosines.
    REQUIRE(attenuation.get("isNode").strictEquals(true));
    REQUIRE_FALSE(attenuation.sameValue(node.get("coneCosNode")));
    REQUIRE_FALSE(attenuation.sameValue(node.get("penumbraCosNode")));
    // The IES texture node is only created on the map path.
    REQUIRE(node.get("_iesTextureNode").isNull());
}

TEST_CASE("IESSpotLightNode update leaves the IES texture alone when the light has none") {
    auto light = JSValue::object(nullptr);
    auto color = JSValue::object(nullptr);
    light.set("color", color);
    light.set("intensity", 1);
    light.set("iesMap", nullptr);
    const auto node = nativeIESSpotLightNodeConstructor().construct({light});
    // update only refreshes the IES texture; the cone uniforms belong to the parent override, and
    // this override does not call it, so a missing light.iesMap must simply do nothing.
    node.get("update").call(node, {JSValue::object(nullptr)});
    REQUIRE(node.get("_iesTextureNode").isNull());
}
