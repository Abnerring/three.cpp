#include "nodes/functions/BSDF/BRDF_EON.h"
#include "nodes/tsl/TSLBase.h"

#include "catch.hpp"

TEST_CASE("BRDF_EON exports the pinned Fn") {
    const auto module = nativeBRDFEONModule();
    const auto function = nativeBRDFEON();
    REQUIRE(module.get("default").sameValue(function));
    REQUIRE(function.get("isFn").toBoolean());
}

TEST_CASE("BRDF_EON builds a vec3 result") {
    auto inputs = JSValue::object();
    inputs.set("lightDirection", nativeTSLBaseBinding("vec3").call({}, {0.2, 0.7, 0.6}));
    inputs.set("diffuseColor", nativeTSLBaseBinding("vec3").call({}, {0.5, 0.4, 0.3}));
    inputs.set("roughness", nativeTSLBaseBinding("float").call({}, {0.35}));
    inputs.set("normalView", nativeTSLBaseBinding("vec3").call({}, {0.0, 0.0, 1.0}));
    inputs.set("viewDirection", nativeTSLBaseBinding("vec3").call({}, {0.1, 0.2, 0.97}));
    const auto result = nativeBRDFEON().call({}, {inputs});
    REQUIRE(result.get("type").strictEquals("VarNode"));
    REQUIRE(result.get("node").get("type").strictEquals("Node"));
}
