#include "objects/GroundedSkybox.h"

#include "geometries/SphereGeometry.h"
#include "materials/MeshBasicMaterial.h"
#include "textures/Texture.h"

#include "catch.hpp"

#include <memory>
#include <stdexcept>

TEST_CASE("GroundedSkybox validates its arguments") {
    auto texture = std::make_shared<Texture>();
    REQUIRE_THROWS_AS(GroundedSkybox(texture, 0, 100), std::runtime_error);
    REQUIRE_THROWS_AS(GroundedSkybox(texture, 15, 0), std::runtime_error);
    REQUIRE_THROWS_AS(GroundedSkybox(texture, 15, 100, 0), std::runtime_error);
}

TEST_CASE("GroundedSkybox builds the mirrored, flattened sphere") {
    auto texture = std::make_shared<Texture>();
    GroundedSkybox skybox(texture, 15, 100, 4);

    const auto geometry = std::dynamic_pointer_cast<SphereGeometry>(skybox.geometry);
    REQUIRE(geometry != nullptr);
    REQUIRE(geometry->parameters.radius == Approx(100));
    REQUIRE(geometry->parameters.widthSegments == Approx(8));
    REQUIRE(geometry->parameters.heightSegments == Approx(4));

    const auto material = std::dynamic_pointer_cast<MeshBasicMaterial>(skybox.material);
    REQUIRE(material != nullptr);
    REQUIRE(material->map == texture);
    REQUIRE(material->depthWrite == false);

    // The sphere is mirrored on z, so the bottom hemisphere reaches y = -radius and is flattened to -height.
    const auto position = skybox.geometry->getAttribute("position");
    REQUIRE(position != nullptr);
    double minimumY = 1e30;
    for (double i = 0; i < position->getCount(); ++i) {
        const double y = position->getY(i);
        if (y < minimumY) minimumY = y;
    }
    REQUIRE(minimumY == Approx(-15));
    REQUIRE(position->getCount() == Approx(9 * 5));
}
