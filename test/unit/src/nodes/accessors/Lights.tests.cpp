#include "nodes/accessors/Lights.h"

#include "catch.hpp"

TEST_CASE("Lights exports the six light accessors") {
    const auto module = nativeLightsModule();
    for (const auto* name : {"lightPosition", "lightProjectionUV", "lightShadowMatrix",
                             "lightTargetDirection", "lightTargetPosition", "lightViewPosition"}) {
        REQUIRE(module.get(name).isCallable());
        REQUIRE(module.get(name).get("length").strictEquals(1));
        REQUIRE(module.get(name).get("name").strictEquals(name));
    }
}

TEST_CASE("Lights caches per-light uniform nodes") {
    auto light = JSValue::object(nullptr);
    const auto position = nativeLightsBinding("lightPosition").call({}, {light});
    const auto positionAgain = nativeLightsBinding("lightPosition").call({}, {light});
    const auto target = nativeLightsBinding("lightTargetPosition").call({}, {light});
    REQUIRE(position.get("type").strictEquals("UniformNode"));
    REQUIRE(position.sameValue(positionAgain));
    REQUIRE(target.get("type").strictEquals("UniformNode"));
    REQUIRE_FALSE(position.sameValue(target));
    REQUIRE(position.get("groupNode").get("name").strictEquals("render"));
}

TEST_CASE("Lights graph helpers return nodes") {
    auto light = JSValue::object(nullptr);
    for (const auto* name : {"lightProjectionUV", "lightTargetDirection", "lightViewPosition"}) {
        const auto node = nativeLightsBinding(name).call({}, {light});
        REQUIRE(node.get("isNode").strictEquals(true));
    }
}
