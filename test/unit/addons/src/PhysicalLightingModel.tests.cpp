#include "nodes/functions/PhysicalLightingModel.h"

#include "core/JSValue.h"

#include "catch.hpp"

TEST_CASE("PhysicalLightingModel applies the feature switches") {
    const auto model = nativePhysicalLightingModelConstructor().construct();
    REQUIRE_FALSE(model.get("clearcoat").toBoolean());
    REQUIRE_FALSE(model.get("sheen").toBoolean());
    REQUIRE_FALSE(model.get("iridescence").toBoolean());
    REQUIRE_FALSE(model.get("anisotropy").toBoolean());
    REQUIRE_FALSE(model.get("transmission").toBoolean());
    REQUIRE_FALSE(model.get("dispersion").toBoolean());
    REQUIRE_FALSE(model.get("retroreflection").toBoolean());
    REQUIRE_FALSE(model.get("diffuseRoughness").toBoolean());
    REQUIRE(model.get("dfg").isNull());
    REQUIRE(model.get("clearcoatRadiance").isNull());
    REQUIRE(model.get("multiScatteringCompensation").isNull());
}

TEST_CASE("PhysicalLightingModel takes the flags from the constructor") {
    const auto model = nativePhysicalLightingModelConstructor().construct(
        {true, true, true, true, true, true, true, true});
    REQUIRE(model.get("clearcoat").toBoolean());
    REQUIRE(model.get("sheen").toBoolean());
    REQUIRE(model.get("iridescence").toBoolean());
    REQUIRE(model.get("anisotropy").toBoolean());
    REQUIRE(model.get("transmission").toBoolean());
    REQUIRE(model.get("dispersion").toBoolean());
    REQUIRE(model.get("retroreflection").toBoolean());
    REQUIRE(model.get("diffuseRoughness").toBoolean());
}