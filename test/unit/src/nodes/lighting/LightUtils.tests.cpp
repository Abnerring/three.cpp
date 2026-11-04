#include "nodes/lighting/LightUtils.h"
#include "nodes/tsl/TSLBase.h"

#include "catch.hpp"

TEST_CASE("LightUtils exports the pinned getDistanceAttenuation Fn") {
    const auto module = nativeLightUtilsModule();
    const auto function = nativeGetDistanceAttenuation();
    REQUIRE(module.get("getDistanceAttenuation").sameValue(function));
    REQUIRE(function.get("isFn").toBoolean());
    REQUIRE(function.get("isNode").toBoolean());

    const auto layout = function.get("shaderNode").get("layout");
    REQUIRE(layout.get("type").strictEquals("float"));
    REQUIRE(layout.get("inputs").get("0").get("name").strictEquals("lightDistance"));
    REQUIRE(layout.get("inputs").get("0").get("type").strictEquals("float"));
    REQUIRE(layout.get("inputs").get("1").get("name").strictEquals("cutoffDistance"));
    REQUIRE(layout.get("inputs").get("2").get("name").strictEquals("decayExponent"));
}

TEST_CASE("LightUtils builds the distance attenuation result shape") {
    auto inputs = JSValue::object();
    inputs.set("lightDistance", nativeTSLBaseBinding("float").call({}, {2.0}));
    inputs.set("cutoffDistance", nativeTSLBaseBinding("float").call({}, {10.0}));
    inputs.set("decayExponent", nativeTSLBaseBinding("float").call({}, {2.0}));

    const auto result = nativeGetDistanceAttenuation().call({}, {inputs});
    REQUIRE(result.get("type").strictEquals("VarNode"));
    REQUIRE(result.get("node").get("type").strictEquals("Node"));
}
