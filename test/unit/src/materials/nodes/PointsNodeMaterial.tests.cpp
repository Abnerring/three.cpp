#include "materials/nodes/PointsNodeMaterial.h"

#include "materials/nodes/SpriteNodeMaterial.h"
#include "nodes/core/Node.h"

#include "catch.hpp"

TEST_CASE("PointsNodeMaterial applies the points defaults on top of the sprite fields") {
    const auto module = nativePointsNodeMaterialModule();
    const auto ctor = module.get("default");
    REQUIRE(ctor.get("name").strictEquals("PointsNodeMaterial"));
    REQUIRE(ctor.get("length").strictEquals(1));
    REQUIRE(ctor.get("type").strictEquals("PointsNodeMaterial"));

    const auto material = ctor.construct();
    REQUIRE(material.get("type").strictEquals("PointsNodeMaterial"));
    REQUIRE(material.get("isPointsNodeMaterial").strictEquals(true));
    // It inherits the sprite fields, then the points defaults are applied over them.
    REQUIRE(material.get("isSpriteNodeMaterial").strictEquals(true));
    REQUIRE(material.get("isNodeMaterial").strictEquals(true));
    REQUIRE(material.get("sizeNode").isNull());
    REQUIRE(material.get("positionNode").isNull());
    // PointsMaterial carries a size and size attenuation, and the node material picks them up.
    REQUIRE(material.get("size").toNumber() == Approx(1));
    REQUIRE(material.get("sizeAttenuation").strictEquals(true));
    REQUIRE(material.get("setupPositionView").get("length").strictEquals(0));
    REQUIRE(material.get("setupVertex").get("length").strictEquals(1));
    REQUIRE(material.get("setupVertexSprite").get("length").strictEquals(1));
}

TEST_CASE("PointsNodeMaterial alphaToCoverage only touches needsUpdate when it changes") {
    const auto material = nativePointsNodeMaterialConstructor().construct();
    const auto versionBefore = material.get("version").toNumber();

    // The base material already initialises the flag to false, so assigning false is not a change and the
    // version stays put. This is what the pinned module does as well.
    REQUIRE(material.get("_useAlphaToCoverage").strictEquals(false));
    auto handle = material;
    handle.set("alphaToCoverage", false);
    const auto versionAfterFirst = material.get("version").toNumber();
    REQUIRE(versionAfterFirst == Approx(versionBefore));

    // Reassigning the same value leaves the version alone.
    handle.set("alphaToCoverage", false);
    REQUIRE(material.get("version").toNumber() == Approx(versionAfterFirst));

    // A real change flips the flag and bumps the version once.
    handle.set("alphaToCoverage", true);
    REQUIRE(material.get("_useAlphaToCoverage").strictEquals(true));
    REQUIRE(material.get("alphaToCoverage").strictEquals(true));
    REQUIRE(material.get("version").toNumber() > versionAfterFirst);
}

TEST_CASE("PointsNodeMaterial setupPositionView builds the view position through the model view matrix") {
    const auto material = nativePointsNodeMaterialConstructor().construct();
    REQUIRE_NOTHROW(material.get("setupPositionView").call(material, {}));
    const auto position = material.get("setupPositionView").call(material, {});
    REQUIRE(position.get("isNode").strictEquals(true));
}
