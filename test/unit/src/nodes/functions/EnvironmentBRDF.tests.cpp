#include "nodes/functions/BSDF/EnvironmentBRDF.h"
#include "nodes/tsl/TSLBase.h"

#include "catch.hpp"

TEST_CASE("EnvironmentBRDF exports the pinned Fn") {
    const auto module = nativeEnvironmentBRDFModule();
    const auto function = nativeEnvironmentBRDF();
    REQUIRE(module.get("default").sameValue(function));
    REQUIRE(function.get("isFn").toBoolean());
}

TEST_CASE("EnvironmentBRDF builds a vec3 result from DFG coefficients") {
    auto inputs = JSValue::object();
    inputs.set("dotNV", nativeTSLBaseBinding("float").call({}, {0.8}));
    inputs.set("specularColor", nativeTSLBaseBinding("vec3").call({}, {0.5, 0.4, 0.3}));
    inputs.set("specularF90", nativeTSLBaseBinding("float").call({}, {1.0}));
    inputs.set("roughness", nativeTSLBaseBinding("float").call({}, {0.35}));
    const auto result = nativeEnvironmentBRDF().call({}, {inputs});
    REQUIRE(result.get("type").strictEquals("VarNode"));
    REQUIRE(result.get("node").get("type").strictEquals("Node"));
}
