#include "nodes/lighting/AnalyticLightNode.h"

#include "nodes/accessors/Position.h"
#include "nodes/core/Node.h"
#include "../../../utils/parity/AnalyticLightNodeScenarios.h"

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

TEST_CASE("AnalyticLightNode preserves custom light colors and shadow disposal listener identity") {
    using namespace AnalyticLightScenarios;
    const auto constructors=constructorCases();
    REQUIRE(constructors.get(JSValue(1)).get("colorTag").strictEquals("custom"));
    for(int i:{0,2,3,4}) {
        REQUIRE(constructors.get(JSValue(i)).get("uniform").strictEquals(true));
        REQUIRE(constructors.get(JSValue(i)).get("group").strictEquals("render"));
    }
    const auto lifecycle=listenerLifecycle();
    REQUIRE(lifecycle.get("listenerSame").strictEquals(true));
    REQUIRE(lifecycle.get("events").toJSON()->dump()=="[\"add:dispose\",\"shadow:lexical-self\",\"remove:dispose:same\",\"node:dispose\"]");
    REQUIRE(lifecycle.get("result").strictEquals("undefined"));
}

TEST_CASE("AnalyticLightNode shadow disposal preserves interrupted and reentrant states") {
    using namespace AnalyticLightScenarios;
    const auto rows=disposalCases();
    REQUIRE(rows.get(JSValue(0)).get("state").get("colorNode").strictEquals("base"));
    REQUIRE(rows.get(JSValue(0)).get("state").get("baseColorNode").strictEquals("null"));
    REQUIRE(rows.get(JSValue(0)).get("state").get("shadowNode").strictEquals("null"));
    const auto failed=rows.get(JSValue(1));
    REQUIRE(failed.get("result").strictEquals("dispose-error"));
    REQUIRE(failed.get("state").get("shadowNode").strictEquals("shadow"));
    REQUIRE(failed.get("state").get("baseColorNode").strictEquals("base"));
    REQUIRE(failed.get("state").get("shadowColorNode").strictEquals("shaded"));
    const auto reentered=rows.get(JSValue(2));
    REQUIRE(reentered.get("state").get("colorNode").strictEquals("reentrant-base"));
    REQUIRE(reentered.get("state").get("shadowColorNode").strictEquals("null"));
    REQUIRE(rows.get(JSValue(3)).get("events").get("length").strictEquals(0));
}

TEST_CASE("AnalyticLightNode distinguishes disabled shadows custom null and cached shadow colors") {
    using namespace AnalyticLightScenarios;
    const auto rows=shadowCases();
    REQUIRE(rows.get(JSValue(0)).get("events").get("length").strictEquals(0));
    REQUIRE(rows.get(JSValue(1)).get("customSame").strictEquals(true));
    REQUIRE(rows.get(JSValue(1)).get("state").get("shadowNode").strictEquals("node"));
    REQUIRE(rows.get(JSValue(2)).get("state").get("colorNode").strictEquals("cached"));
    REQUIRE(rows.get(JSValue(2)).get("events").get("length").strictEquals(0));
    REQUIRE(rows.get(JSValue(7)).get("events").get("length").strictEquals(0));
    REQUIRE(rows.get(JSValue(8)).get("state").get("colorNode").strictEquals("shaded"));
    REQUIRE(rows.get(JSValue(9)).get("state").get("shadowNode").strictEquals("null"));
    REQUIRE(rows.get(JSValue(9)).get("events").toJSON()->dump()=="[\"mul:self\",\"null\"]");
    REQUIRE(rows.get(JSValue(10)).get("state").get("shadowNode").strictEquals("node"));
    const auto generated=defaultFactory();
    REQUIRE(generated.get("isShadowNode").strictEquals(true));
    REQUIRE(generated.get("lightSame").strictEquals(true));
    REQUIRE(generated.get("shadowSame").strictEquals(true));
}

TEST_CASE("AnalyticLightNode shadow callbacks preserve receiver evaluation order and exception boundaries") {
    using namespace AnalyticLightScenarios;
    const auto rows=shadowCases();
    const auto overridden=rows.get(JSValue(3));
    REQUIRE(overridden.get("events").get(JSValue(3)).strictEquals("getShadow:args"));
    REQUIRE(overridden.get("state").get("colorNode").strictEquals("override"));
    REQUIRE(overridden.get("state").get("shadowColorNode").strictEquals("shaded"));
    const auto mulFailed=rows.get(JSValue(4));
    REQUIRE(mulFailed.get("result").strictEquals("mul-error"));
    REQUIRE(mulFailed.get("state").get("shadowNode").strictEquals("shadow"));
    REQUIRE(mulFailed.get("state").get("baseColorNode").strictEquals("null"));
    const auto callbackFailed=rows.get(JSValue(5));
    REQUIRE(callbackFailed.get("result").strictEquals("getShadow-error"));
    REQUIRE(callbackFailed.get("state").get("colorNode").strictEquals("color"));
    REQUIRE(callbackFailed.get("state").get("baseColorNode").strictEquals("color"));
    const auto reentered=rows.get(JSValue(6));
    REQUIRE(reentered.get("state").get("colorNode").strictEquals("override"));
    REQUIRE(reentered.get("state").get("shadowNode").strictEquals("null"));
    REQUIRE(reentered.get("state").get("baseColorNode").strictEquals("null"));
}

TEST_CASE("AnalyticLightNode computes both direct light inputs before dispatch and obeys shadow switches") {
    using namespace AnalyticLightScenarios;
    const auto rows=setupCases();
    REQUIRE(rows.get(JSValue(0)).get("events").toJSON()->dump()=="[\"setupDirect\",true,\"color\",\"setupDirectRectArea\",true,\"color\",\"setupDirectLight\",true,\"direct\",\"setupDirectRectAreaLight\",true,\"rect\"]");
    REQUIRE(rows.get(JSValue(1)).get("events").get(JSValue(0)).strictEquals("setupShadow"));
    REQUIRE(rows.get(JSValue(1)).get("state").get("colorNode").strictEquals("shadowed"));
    REQUIRE(rows.get(JSValue(2)).get("events").get(JSValue(0)).strictEquals("setupDirect"));
    REQUIRE(rows.get(JSValue(3)).get("events").get(JSValue(0)).strictEquals("shadow:dispose"));
    REQUIRE(rows.get(JSValue(3)).get("state").get("baseColorNode").strictEquals("base"));
    REQUIRE(rows.get(JSValue(3)).get("state").get("colorNode").strictEquals("base"));
    REQUIRE(rows.get(JSValue(4)).get("result").strictEquals("rect-error"));
    REQUIRE(rows.get(JSValue(4)).get("events").get("length").strictEquals(6));
    REQUIRE(rows.get(JSValue(5)).get("events").get("length").strictEquals(6));
    REQUIRE(rows.get(JSValue(6)).get("result").strictEquals("dispatch-error"));
    REQUIRE(rows.get(JSValue(6)).get("events").get("length").strictEquals(9));
}

TEST_CASE("AnalyticLightNode update invokes multiplyScalar on the copy return value") {
    using namespace AnalyticLightScenarios;
    const auto rows=updateCases();
    REQUIRE(rows.get(JSValue(0)).get("events").toJSON()->dump()=="[\"copy:args\",\"get:intensity\",\"multiply:return-value\",3]");
    REQUIRE(rows.get(JSValue(1)).get("result").strictEquals("TypeError"));
    REQUIRE(rows.get(JSValue(1)).get("events").toJSON()->dump()=="[\"copy:args\",\"get:intensity\"]");
    REQUIRE(rows.get(JSValue(2)).get("result").strictEquals("copy-error"));
    REQUIRE(rows.get(JSValue(2)).get("events").get("length").strictEquals(1));
    REQUIRE(rows.get(JSValue(3)).get("result").strictEquals("TypeError"));
    REQUIRE(rows.get(JSValue(3)).get("events").get("length").strictEquals(0));
    REQUIRE(rows.get(JSValue(4)).get("color").strictEquals("replaced-color"));
    REQUIRE(rows.get(JSValue(4)).get("events").toJSON()->dump()=="[\"copy:args\",\"get:intensity\",\"multiply:return-value\",3]");
    REQUIRE(rows.get(JSValue(5)).get("result").strictEquals("intensity-error"));
    REQUIRE(rows.get(JSValue(5)).get("events").toJSON()->dump()=="[\"copy:args\",\"get:intensity\"]");
    REQUIRE(rows.get(JSValue(6)).get("result").strictEquals("multiply-error"));
    REQUIRE(rows.get(JSValue(6)).get("events").toJSON()->dump()=="[\"copy:args\",\"get:multiplyScalar\"]");
}
