#include "nodes/functions/material/getParallaxCorrectNormal.h"
#include "nodes/tsl/TSLBase.h"

#include "catch.hpp"

TEST_CASE("getParallaxCorrectNormal exports the pinned Fn") {
    const auto module = nativeGetParallaxCorrectNormalModule();
    const auto function = nativeGetParallaxCorrectNormal();
    REQUIRE(module.get("default").sameValue(function));
    REQUIRE(function.get("isFn").toBoolean());
}

TEST_CASE("getParallaxCorrectNormal builds a vec3 result") {
    auto inputs = JSValue::array();
    inputs.set(JSValue(0), nativeTSLBaseBinding("vec3").call({}, {1.0, 2.0, 3.0}));
    inputs.set(JSValue(1), nativeTSLBaseBinding("vec3").call({}, {10.0, 20.0, 30.0}));
    inputs.set(JSValue(2), nativeTSLBaseBinding("vec3").call({}, {0.0, 1.0, 2.0}));
    const auto result = nativeGetParallaxCorrectNormal().call({}, {inputs});
    REQUIRE(result.get("type").strictEquals("VarNode"));
    REQUIRE(result.get("node").get("type").strictEquals("Node"));
}
