#include "nodes/lighting/AnalyticLightNode.h"

#include "nodes/accessors/Position.h"
#include "nodes/core/Node.h"

#include "catch.hpp"

TEST_CASE("AnalyticLightNode keeps the pinned construction shape") {
    auto light = JSValue::object(nullptr);
    light.set("uuid", "light-uuid");
    const auto node = nativeAnalyticLightNodeConstructor().construct({light});
    REQUIRE(node.get("type").strictEquals("AnalyticLightNode"));
    REQUIRE(node.get("isAnalyticLightNode").strictEquals(true));
    REQUIRE(node.get("light").sameValue(light));
    REQUIRE(node.get("colorNode").get("type").strictEquals("UniformNode"));
    REQUIRE(node.get("colorNode").get("groupNode").get("name").strictEquals("render"));
    REQUIRE(node.get("updateType").strictEquals("frame"));
    REQUIRE(node.get("getHash").call(node).strictEquals("light-uuid"));
}

TEST_CASE("AnalyticLightNode getLightVector subtracts the position view from the light view") {
    auto light = JSValue::object(nullptr);
    const auto node = nativeAnalyticLightNodeConstructor().construct({light});

    // No context value: the accessor is used.
    auto builderWithout = JSValue::object(nullptr);
    builderWithout.set("context", JSValue::object(nullptr));
    const auto fallback = node.get("getLightVector").call(node, {builderWithout});
    REQUIRE(fallback.get("isNode").strictEquals(true));

    // A context positionView wins over the accessor; the resulting subtree is a different one.
    const auto contextPosition = nativePositionBinding("positionView");
    auto builderWith = JSValue::object(nullptr);
    auto context = JSValue::object(nullptr);
    context.set("positionView", contextPosition);
    builderWith.set("context", context);
    const auto overridden = node.get("getLightVector").call(node, {builderWith});
    REQUIRE(overridden.get("isNode").strictEquals(true));
    REQUIRE_FALSE(overridden.sameValue(fallback));

    // getLightVector( builder ) takes exactly one parameter.
    REQUIRE(node.get("getLightVector").get("length").strictEquals(1));
}
