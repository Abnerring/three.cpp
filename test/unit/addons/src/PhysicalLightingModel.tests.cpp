#include "nodes/functions/PhysicalLightingModel.h"

#include "core/JSValue.h"

#include "catch.hpp"

#include "nodes/tsl/TSLBase.h"

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
TEST_CASE("PhysicalLightingModel module helpers build graphs") {
    const auto roughness = nativeTSLBaseBinding("float").call({}, {JSValue(0.5)});
    const auto ior = nativeTSLBaseBinding("float").call({}, {JSValue(1.5)});
    const auto adjusted = nativePhysicalLightingModelHelper("applyIorToRoughness").call({}, {roughness, ior});
    REQUIRE(adjusted.toBoolean());
    REQUIRE(adjusted.get("isNode").toBoolean());

    const auto transmissionDistance = nativeTSLBaseBinding("float").call({}, {JSValue(1.0)});
    const auto attenuationColor = nativeTSLBaseBinding("vec3").call(
        {}, {JSValue(1.0), JSValue(1.0), JSValue(1.0)});
    const auto attenuationDistance = nativeTSLBaseBinding("float").call({}, {JSValue(0.0)});
    const auto attenuation = nativePhysicalLightingModelHelper("volumeAttenuation").call(
        {}, {transmissionDistance, attenuationColor, attenuationDistance});
    REQUIRE(attenuation.get("isNode").toBoolean());

    const auto normal = nativeTSLBaseBinding("vec3").call({}, {JSValue(0.0), JSValue(0.0), JSValue(1.0)});
    const auto view = nativeTSLBaseBinding("vec3").call({}, {JSValue(0.0), JSValue(0.0), JSValue(1.0)});
    const auto thickness = nativeTSLBaseBinding("float").call({}, {JSValue(0.1)});
    const auto matrix = nativeTSLBaseBinding("mat4").call({}, {});
    const auto ray = nativePhysicalLightingModelHelper("getVolumeTransmissionRay").call(
        {}, {normal, view, thickness, ior, matrix});
    REQUIRE(ray.get("isNode").toBoolean());
}