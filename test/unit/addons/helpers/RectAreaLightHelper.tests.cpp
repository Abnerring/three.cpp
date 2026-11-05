#include "helpers/RectAreaLightHelper.h"

#include "lights/RectAreaLight.h"
#include "materials/LineBasicMaterial.h"
#include "materials/MeshBasicMaterial.h"
#include "objects/Mesh.h"

#include "catch.hpp"

#include <algorithm>
#include <memory>

TEST_CASE("RectAreaLightHelper builds the outline and the filled quad") {
    auto light = std::make_shared<RectAreaLight>(Color(0xff0000), 2, 4, 6);
    RectAreaLightHelper helper(light);

    REQUIRE(helper.type == "RectAreaLightHelper");
    REQUIRE(helper.geometry->getAttribute("position")->getCount() == Approx(5));
    REQUIRE(helper.children.size() == 1);
    const auto fill = std::dynamic_pointer_cast<Mesh>(helper.children[0]);
    REQUIRE(fill != nullptr);
    REQUIRE(fill->geometry->getAttribute("position")->getCount() == Approx(6));
    const auto fillMaterial = std::dynamic_pointer_cast<MeshBasicMaterial>(fill->material);
    REQUIRE(fillMaterial != nullptr);
    REQUIRE(fillMaterial->side == BackSide);
    REQUIRE(fillMaterial->fog == false);
    const auto outlineMaterial = std::dynamic_pointer_cast<LineBasicMaterial>(helper.material);
    REQUIRE(outlineMaterial != nullptr);
    REQUIRE(outlineMaterial->fog == false);
}

TEST_CASE("RectAreaLightHelper scales to the light and derives the colour") {
    auto light = std::make_shared<RectAreaLight>(Color(0xff0000), 2, 4, 6);
    light->matrixWorld.makeTranslation(1, 2, 3);
    RectAreaLightHelper helper(light);
    helper.updateMatrixWorld();

    // scale = ( width / 2, height / 2, 1 )
    REQUIRE(helper.scale.x == Approx(2));
    REQUIRE(helper.scale.y == Approx(3));
    REQUIRE(helper.scale.z == Approx(1));

    // The red channel of (1, 0, 0) * 2 is normalised back to one.
    const auto outlineMaterial = std::dynamic_pointer_cast<LineBasicMaterial>(helper.material);
    REQUIRE(outlineMaterial->color.getHex() == 0xff0000);
    const auto fill = std::dynamic_pointer_cast<Mesh>(helper.children[0]);
    REQUIRE(std::dynamic_pointer_cast<MeshBasicMaterial>(fill->material)->color.getHex() == 0xff0000);

    // The helper sits at the light world position and copies its matrix to the fill.
    REQUIRE(helper.matrixWorld.elements()[12] == Approx(1));
    REQUIRE(helper.matrixWorld.elements()[13] == Approx(2));
    REQUIRE(helper.matrixWorld.elements()[14] == Approx(3));
    const auto helperElements = helper.matrixWorld.elements();
    const auto fillElements = fill->matrixWorld.elements();
    REQUIRE(std::equal(helperElements.begin(), helperElements.end(), fillElements.begin()));
}

TEST_CASE("RectAreaLightHelper prefers the explicit colour") {
    auto light = std::make_shared<RectAreaLight>(Color(0xff0000), 2, 4, 6);
    RectAreaLightHelper helper(light, Color(0x00ff00));
    helper.updateMatrixWorld();

    const auto outlineMaterial = std::dynamic_pointer_cast<LineBasicMaterial>(helper.material);
    REQUIRE(outlineMaterial->color.getHex() == 0x00ff00);
    const auto fill = std::dynamic_pointer_cast<Mesh>(helper.children[0]);
    REQUIRE(std::dynamic_pointer_cast<MeshBasicMaterial>(fill->material)->color.getHex() == 0x00ff00);
}
