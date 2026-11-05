#include "materials/nodes/SpriteNodeMaterial.h"

#include "materials/nodes/NodeMaterial.h"
#include "nodes/core/Node.h"

#include "catch.hpp"

namespace {
JSValue builder(bool perspective, const JSValue& object) {
    auto camera = JSValue::object(nullptr);
    camera.set("isPerspectiveCamera", perspective);
    auto value = JSValue::object(nullptr);
    value.set("object", object);
    value.set("camera", camera);
    return value;
}
}  // namespace

TEST_CASE("SpriteNodeMaterial applies the sprite defaults and its own fields") {
    const auto module = nativeSpriteNodeMaterialModule();
    const auto ctor = module.get("default");
    REQUIRE(ctor.get("name").strictEquals("SpriteNodeMaterial"));
    REQUIRE(ctor.get("length").strictEquals(1));
    REQUIRE(ctor.get("type").strictEquals("SpriteNodeMaterial"));

    const auto material = ctor.construct();
    REQUIRE(material.get("type").strictEquals("SpriteNodeMaterial"));
    REQUIRE(material.get("isSpriteNodeMaterial").strictEquals(true));
    REQUIRE(material.get("isNodeMaterial").strictEquals(true));
    // The constructor writes these before the defaults are applied.
    REQUIRE(material.get("_useSizeAttenuation").strictEquals(true));
    REQUIRE(material.get("positionNode").isNull());
    REQUIRE(material.get("rotationNode").isNull());
    REQUIRE(material.get("scaleNode").isNull());
    REQUIRE(material.get("transparent").strictEquals(true));
    REQUIRE(material.get("setupPositionView").get("length").strictEquals(1));
}

TEST_CASE("SpriteNodeMaterial sizeAttenuation only touches needsUpdate when it changes") {
    const auto material = nativeSpriteNodeMaterialConstructor().construct();
    REQUIRE(material.get("sizeAttenuation").strictEquals(true));
    const auto versionBefore = material.get("version").toNumber();

    // Assigning the same value leaves the version alone.
    auto sameHandle = material;
    sameHandle.set("sizeAttenuation", true);
    REQUIRE(material.get("version").toNumber() == Approx(versionBefore));

    // A different value flips the flag and bumps the version.
    sameHandle.set("sizeAttenuation", false);
    REQUIRE(material.get("_useSizeAttenuation").strictEquals(false));
    REQUIRE(material.get("sizeAttenuation").strictEquals(false));
    REQUIRE(material.get("version").toNumber() > versionBefore);
}

TEST_CASE("SpriteNodeMaterial setupPositionView builds the aligned sprite position") {
    const auto material = nativeSpriteNodeMaterialConstructor().construct();
    // A non perspective camera with no centre keeps the graph on its main path.
    auto emptyObject = JSValue::object(nullptr);
    REQUIRE_NOTHROW(material.get("setupPositionView").call(material, {builder(false, emptyObject)}));
    const auto position = material.get("setupPositionView").call(material, {builder(false, emptyObject)});
    REQUIRE(position.get("isNode").strictEquals(true));

    // The perspective branch multiplies the scale by the negated view depth.
    auto perspectiveMaterial = nativeSpriteNodeMaterialConstructor().construct();
    auto perspectiveHandle = perspectiveMaterial;
    perspectiveHandle.set("sizeAttenuation", false);
    REQUIRE_NOTHROW(
        perspectiveMaterial.get("setupPositionView").call(perspectiveMaterial, {builder(true, emptyObject)}));
}
