#include "nodes/functions/BSDF/V_GGX_SmithCorrelated_Anisotropic.h"
#include "nodes/tsl/TSLBase.h"

#include "catch.hpp"

TEST_CASE("V_GGX_SmithCorrelated_Anisotropic exports the pinned typed Fn") {
    const auto module = nativeVGGXSmithCorrelatedAnisotropicModule();
    const auto function = nativeVGGXSmithCorrelatedAnisotropic();
    REQUIRE(module.get("default").sameValue(function));
    REQUIRE(function.get("isFn").toBoolean());
    REQUIRE(function.get("shaderNode").get("layout").get("type").strictEquals("float"));
}

TEST_CASE("V_GGX_SmithCorrelated_Anisotropic builds a float result") {
    auto inputs = JSValue::object();
    for (const auto* name : {"alphaT", "alphaB", "dotTV", "dotBV", "dotTL", "dotBL", "dotNV", "dotNL"})
        inputs.set(name, nativeTSLBaseBinding("float").call({}, {0.5}));
    const auto result = nativeVGGXSmithCorrelatedAnisotropic().call({}, {inputs});
    REQUIRE(result.get("type").strictEquals("VarNode"));
    REQUIRE(result.get("node").get("type").strictEquals("Node"));
}
