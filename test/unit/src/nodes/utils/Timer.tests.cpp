#include "nodes/utils/Timer.h"

#include "catch.hpp"

TEST_CASE("Timer exports render group uniforms") {
    const auto module = nativeTimerModule();
    for (const auto& name : {"time", "deltaTime", "frameId"}) {
        const auto node = module.get(name);
        REQUIRE(node.get("type").strictEquals("UniformNode"));
        REQUIRE(node.get("isUniformNode").strictEquals(true));
        REQUIRE(node.get("groupNode").get("type").strictEquals("UniformGroupNode"));
        REQUIRE(node.get("groupNode").get("name").strictEquals("render"));
        REQUIRE(node.get("updateType").strictEquals("render"));
        REQUIRE(node.get("value").strictEquals(0));
    }
}

TEST_CASE("Timer frameId preserves uint node type") {
    REQUIRE(nativeTimerTime().get("nodeType").isNull());
    REQUIRE(nativeTimerDeltaTime().get("nodeType").isNull());
    REQUIRE(nativeTimerFrameId().get("nodeType").strictEquals("uint"));
    REQUIRE(nativeTimerModule().get("time").sameValue(nativeTimerTime()));
}
