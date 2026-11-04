#include "nodes/accessors/VelocityNode.h"
#include "nodes/tsl/TSLBase.h"

#include "catch.hpp"

TEST_CASE("VelocityNode exports the immutable velocity node") {
    const auto module = nativeVelocityNodeModule();
    REQUIRE(module.get("default").sameValue(nativeVelocityNodeConstructor()));
    REQUIRE(module.get("velocity").get("type").strictEquals("VelocityNode"));
    REQUIRE(module.get("velocity").get("nodeType").strictEquals("vec2"));
}

TEST_CASE("VelocityNode initializes motion uniforms and update modes") {
    const auto node = nativeVelocityNodeConstructor().construct();
    REQUIRE(node.get("type").strictEquals("VelocityNode"));
    REQUIRE(node.get("nodeType").strictEquals("vec2"));
    REQUIRE(node.get("projectionMatrix").isNull());
    REQUIRE(node.get("updateType").strictEquals("object"));
    REQUIRE(node.get("updateAfterType").strictEquals("object"));
    for (const auto* key : {"previousModelWorldMatrix", "currentProjectionMatrix", "previousProjectionMatrix", "previousCameraViewMatrix"}) {
        REQUIRE(node.get(key).get("type").strictEquals("UniformNode"));
        REQUIRE(node.get(key).get("value").isObject());
    }
    REQUIRE(node.get("currentProjectionMatrix").get("groupNode").get("name").strictEquals("render"));
    REQUIRE(node.get("previousProjectionMatrix").get("groupNode").get("name").strictEquals("render"));
    REQUIRE(node.get("previousCameraViewMatrix").get("groupNode").get("name").strictEquals("render"));
    REQUIRE(node.get("setProjectionMatrix").isCallable());
    REQUIRE(node.get("update").isCallable());
    REQUIRE(node.get("updateAfter").isCallable());
    REQUIRE(node.get("setup").isCallable());
    const auto setup = node.get("setup").call(node);
    REQUIRE(setup.get("isNode").strictEquals(true));
}
