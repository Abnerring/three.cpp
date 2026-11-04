#include "nodes/accessors/SceneProperties.h"

#include "catch.hpp"

TEST_CASE("SceneProperties exports render uniforms") {
    const auto module = nativeScenePropertiesModule();
    for (const auto* name : {"backgroundBlurriness", "backgroundIntensity", "backgroundRotation"}) {
        const auto node = module.get(name);
        REQUIRE(node.get("type").strictEquals("UniformNode"));
        REQUIRE(node.get("updateType").strictEquals("render"));
        REQUIRE(node.get("groupNode").get("name").strictEquals("render"));
        REQUIRE(node.get("update").isCallable());
    }
    REQUIRE(module.get("backgroundBlurriness").get("value").strictEquals(0));
    REQUIRE(module.get("backgroundIntensity").get("value").strictEquals(1));
    REQUIRE(module.get("backgroundRotation").get("value").get("elements").get("length").strictEquals(16));
}

