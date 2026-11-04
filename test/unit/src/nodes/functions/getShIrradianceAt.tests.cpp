#include "nodes/functions/material/getShIrradianceAt.h"
#include "nodes/accessors/UniformArrayNode.h"
#include "nodes/tsl/TSLBase.h"

#include "catch.hpp"

TEST_CASE("getShIrradianceAt exports the pinned Fn") {
    const auto module = nativeGetShIrradianceAtModule();
    const auto function = nativeGetShIrradianceAt();
    REQUIRE(module.get("default").sameValue(function));
    REQUIRE(function.get("isFn").toBoolean());
}

TEST_CASE("getShIrradianceAt builds a vec3 result") {
    auto coefficients = JSValue::array();
    for (int i = 0; i < 9; ++i)
        coefficients.set(JSValue(i), nativeTSLBaseBinding("vec3").call({}, {i + 1.0, i + 2.0, i + 3.0}));
    UniformArrayNode shCoefficients(coefficients, "vec3");
    auto inputs = JSValue::array();
    inputs.set(JSValue(0), nativeTSLBaseBinding("vec3").call({}, {1.0, 0.0, 0.0}));
    inputs.set(JSValue(1), static_cast<JSValue>(shCoefficients));
    const auto result = nativeGetShIrradianceAt().call({}, {inputs});
    REQUIRE(result.get("type").strictEquals("VarNode"));
    REQUIRE(result.get("node").get("type").strictEquals("Node"));
}
