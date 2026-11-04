#include "nodes/functions/BSDF/D_GGX.h"
#include "nodes/tsl/TSLBase.h"

#include "catch.hpp"

TEST_CASE("D_GGX exports the pinned typed Fn") {
    const auto module = nativeDGGXModule();
    const auto function = nativeDGGX();
    REQUIRE(module.get("default").sameValue(function));
    REQUIRE(function.get("isFn").toBoolean());
    REQUIRE(function.get("shaderNode").get("layout").get("type").strictEquals("float"));
}

TEST_CASE("D_GGX builds a float result from alpha and dotNH") {
    auto inputs = JSValue::object();
    inputs.set("alpha", nativeTSLBaseBinding("float").call({}, {0.5}));
    inputs.set("dotNH", nativeTSLBaseBinding("float").call({}, {0.75}));
    const auto result = nativeDGGX().call({}, {inputs});
    REQUIRE(result.get("type").strictEquals("VarNode"));
    REQUIRE(result.get("node").get("type").strictEquals("Node"));
}
