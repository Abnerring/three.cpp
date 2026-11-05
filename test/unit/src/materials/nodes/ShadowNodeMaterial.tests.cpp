#include "materials/nodes/ShadowNodeMaterial.h"

#include "materials/nodes/NodeMaterial.h"

#include "catch.hpp"

TEST_CASE("ShadowNodeMaterial applies the shadow material defaults and its own flags") {
    const auto module = nativeShadowNodeMaterialModule();
    const auto ctor = module.get("default");
    REQUIRE(ctor.get("name").strictEquals("ShadowNodeMaterial"));
    REQUIRE(ctor.get("length").strictEquals(1));
    REQUIRE(ctor.get("type").strictEquals("ShadowNodeMaterial"));

    const auto material = ctor.construct();
    REQUIRE(material.get("type").strictEquals("ShadowNodeMaterial"));
    REQUIRE(material.get("isShadowNodeMaterial").strictEquals(true));
    REQUIRE(material.get("isNodeMaterial").strictEquals(true));
    // The constructor sets these before the defaults are applied, as the source does.
    REQUIRE(material.get("lights").strictEquals(true));
    REQUIRE(material.get("transparent").strictEquals(true));
    REQUIRE(material.get("setupLightingModel").get("length").strictEquals(1));
}

TEST_CASE("ShadowNodeMaterial setupLightingModel answers a shadow mask model") {
    const auto material = nativeShadowNodeMaterialConstructor().construct();
    const auto model = material.get("setupLightingModel").call(material, {JSValue::object(nullptr)});
    // The lighting model carries the shadow accumulator its finish and direct hooks work on.
    REQUIRE(model.get("shadowNode").get("isNode").strictEquals(true));
    REQUIRE(model.get("direct").isCallable());
    REQUIRE(model.get("finish").isCallable());
    // Two calls answer two separate models, as the source constructs a new one each time.
    const auto again = material.get("setupLightingModel").call(material, {JSValue::object(nullptr)});
    REQUIRE_FALSE(again.sameValue(model));
}
