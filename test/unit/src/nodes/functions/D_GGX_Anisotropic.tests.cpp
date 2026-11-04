#include "nodes/functions/BSDF/D_GGX_Anisotropic.h"
#include "nodes/tsl/TSLBase.h"

#include "catch.hpp"

TEST_CASE("D_GGX_Anisotropic exports the pinned typed Fn") {
    const auto module = nativeDGGXAnisotropicModule();
    const auto function = nativeDGGXAnisotropic();
    REQUIRE(module.get("default").sameValue(function));
    REQUIRE(function.get("isFn").toBoolean());
    REQUIRE(function.get("shaderNode").get("layout").get("type").strictEquals("float"));
}

TEST_CASE("D_GGX_Anisotropic builds a float result") {
    auto inputs = JSValue::object();
    for (const auto* name : {"alphaT", "alphaB", "dotNH", "dotTH", "dotBH"})
        inputs.set(name, nativeTSLBaseBinding("float").call({}, {0.5}));
    const auto result = nativeDGGXAnisotropic().call({}, {inputs});
    REQUIRE(result.get("type").strictEquals("VarNode"));
    REQUIRE(result.get("node").get("type").strictEquals("Node"));
}
