#include "nodes/functions/BSDF/V_GGX_SmithCorrelated.h"
#include "nodes/tsl/TSLBase.h"

#include "catch.hpp"

TEST_CASE("V_GGX_SmithCorrelated exports the pinned typed Fn") {
    const auto module = nativeVGGXSmithCorrelatedModule();
    const auto function = nativeVGGXSmithCorrelated();
    REQUIRE(module.get("default").sameValue(function));
    REQUIRE(function.get("isFn").toBoolean());
    REQUIRE(function.get("shaderNode").get("layout").get("type").strictEquals("float"));
}

TEST_CASE("V_GGX_SmithCorrelated builds a float result") {
    auto inputs = JSValue::object();
    inputs.set("alpha", nativeTSLBaseBinding("float").call({}, {0.5}));
    inputs.set("dotNL", nativeTSLBaseBinding("float").call({}, {0.75}));
    inputs.set("dotNV", nativeTSLBaseBinding("float").call({}, {0.65}));
    const auto result = nativeVGGXSmithCorrelated().call({}, {inputs});
    REQUIRE(result.get("type").strictEquals("VarNode"));
    REQUIRE(result.get("node").get("type").strictEquals("Node"));
}
