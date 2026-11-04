#include "nodes/functions/BSDF/BRDF_Sheen.h"
#include "nodes/tsl/TSLBase.h"

#include "catch.hpp"

TEST_CASE("BRDF_Sheen exports the pinned Fn") {
    const auto module = nativeBRDFSheenModule();
    const auto function = nativeBRDFSheen();
    REQUIRE(module.get("default").sameValue(function));
    REQUIRE(function.get("isFn").toBoolean());
}

TEST_CASE("BRDF_Sheen builds a scalar sheen result") {
    auto inputs = JSValue::object();
    inputs.set("lightDirection", nativeTSLBaseBinding("vec3").call({}, {1.0, 0.0, 0.0}));
    const auto result = nativeBRDFSheen().call({}, {inputs});
    REQUIRE(result.get("type").strictEquals("VarNode"));
    REQUIRE(result.get("node").get("type").strictEquals("Node"));
}
