#include "materials/nodes/VolumeNodeMaterial.h"

#include "materials/nodes/NodeMaterial.h"
#include "nodes/core/Node.h"

#include "catch.hpp"

TEST_CASE("VolumeNodeMaterial writes the pinned fields without a defaults object") {
    const auto module = nativeVolumeNodeMaterialModule();
    const auto ctor = module.get("default");
    REQUIRE(ctor.get("name").strictEquals("VolumeNodeMaterial"));
    REQUIRE(ctor.get("length").strictEquals(1));
    REQUIRE(ctor.get("type").strictEquals("VolumeNodeMaterial"));

    const auto material = ctor.construct();
    REQUIRE(material.get("type").strictEquals("VolumeNodeMaterial"));
    REQUIRE(material.get("isVolumeNodeMaterial").strictEquals(true));
    REQUIRE(material.get("isNodeMaterial").strictEquals(true));
    // The volume fields, written by the constructor itself.
    REQUIRE(material.get("steps").toNumber() == Approx(25));
    REQUIRE(material.get("offsetNode").isNull());
    REQUIRE(material.get("scatteringNode").isNull());
    REQUIRE(material.get("lights").strictEquals(true));
    REQUIRE(material.get("transparent").strictEquals(true));
    // BackSide is one, which is what the constructor writes for side.
    REQUIRE(material.get("side").toNumber() == Approx(1));
    REQUIRE(material.get("depthTest").strictEquals(false));
    REQUIRE(material.get("depthWrite").strictEquals(false));
    // The source method takes no parameter, so its length is zero.
    REQUIRE(material.get("setupLightingModel").get("length").strictEquals(0));
}

TEST_CASE("VolumeNodeMaterial setupLightingModel answers the volumetric model") {
    const auto material = nativeVolumeNodeMaterialConstructor().construct();
    const auto model = material.get("setupLightingModel").call(material, {JSValue::object(nullptr)});
    // The model carries the hooks the volume pipeline drives.
    REQUIRE(model.get("scatteringLight").get("length").strictEquals(2));
    REQUIRE(model.get("direct").get("length").strictEquals(2));
    REQUIRE(model.get("finish").get("length").strictEquals(1));
    // Every call answers a new model.
    const auto again = material.get("setupLightingModel").call(material, {JSValue::object(nullptr)});
    REQUIRE_FALSE(again.sameValue(model));
}

TEST_CASE("VolumeNodeMaterial accepts parameters over its own fields") {
    auto parameters = JSValue::object(nullptr);
    parameters.set("steps", 50);
    parameters.set("transparent", false);
    const auto material = nativeVolumeNodeMaterialConstructor().construct({parameters});
    REQUIRE(material.get("steps").toNumber() == Approx(50));
    REQUIRE(material.get("transparent").strictEquals(false));
}
