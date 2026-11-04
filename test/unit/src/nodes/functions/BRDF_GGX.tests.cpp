#include "nodes/functions/BSDF/BRDF_GGX.h"
#include "nodes/tsl/TSLBase.h"

#include "catch.hpp"

namespace {
JSValue inputs(bool anisotropic = false) {
    auto value = JSValue::object();
    value.set("lightDirection", nativeTSLBaseBinding("vec3").call({}, {1.0, 0.0, 0.0}));
    value.set("f0", nativeTSLBaseBinding("vec3").call({}, {0.04, 0.05, 0.06}));
    value.set("f90", nativeTSLBaseBinding("float").call({}, {1.0}));
    value.set("roughness", nativeTSLBaseBinding("float").call({}, {0.5}));
    value.set("f", nativeTSLBaseBinding("vec3").call({}, {0.2, 0.3, 0.4}));
    if (anisotropic) {
        value.set("USE_IRIDESCENCE", true);
        value.set("USE_ANISOTROPY", true);
    }
    return value;
}
}

TEST_CASE("BRDF_GGX exports the pinned Fn") {
    const auto module = nativeBRDFGGXModule();
    const auto function = nativeBRDFGGX();
    REQUIRE(module.get("default").sameValue(function));
    REQUIRE(function.get("isFn").toBoolean());
}

TEST_CASE("BRDF_GGX builds isotropic and anisotropic results") {
    const auto isotropic = nativeBRDFGGX().call({}, {inputs()});
    REQUIRE(isotropic.get("type").strictEquals("VarNode"));
    REQUIRE(isotropic.get("node").get("type").strictEquals("Node"));

    const auto anisotropic = nativeBRDFGGX().call({}, {inputs(true)});
    REQUIRE(anisotropic.get("type").strictEquals("VarNode"));
    REQUIRE(anisotropic.get("node").get("type").strictEquals("Node"));
}
