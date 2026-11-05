#include "environments/ColorEnvironment.h"

#include "constants.h"
#include "core/EventDispatcher.h"
#include "geometries/SphereGeometry.h"
#include "materials/MeshBasicMaterial.h"
#include "objects/Mesh.h"

#include "catch.hpp"

#include <memory>

TEST_CASE("ColorEnvironment builds a unit sphere with a basic material") {
    ColorEnvironment environment;
    REQUIRE(environment.name == "ColorEnvironment");
    REQUIRE(environment.children.size() == 1);

    const auto mesh = std::dynamic_pointer_cast<Mesh>(environment.children[0]);
    REQUIRE(mesh != nullptr);
    REQUIRE(mesh->type == "Mesh");

    const auto geometry = std::dynamic_pointer_cast<SphereGeometry>(mesh->geometry);
    REQUIRE(geometry != nullptr);
    REQUIRE(geometry->parameters.radius == Approx(1));
    REQUIRE(geometry->parameters.widthSegments == Approx(16));
    REQUIRE(geometry->parameters.heightSegments == Approx(16));

    const auto material = std::dynamic_pointer_cast<MeshBasicMaterial>(mesh->material);
    REQUIRE(material != nullptr);
    REQUIRE(material->type == "MeshBasicMaterial");
    // The material faces the inside of the sphere.
    REQUIRE(material->side == BackSide);
    REQUIRE(material->color.getHex() == 0xffffff);
}

TEST_CASE("ColorEnvironment passes the colour through and disposes its child") {
    ColorEnvironment coloured(0x336699);
    const auto mesh = std::dynamic_pointer_cast<Mesh>(coloured.children[0]);
    REQUIRE(mesh != nullptr);
    const auto material = std::dynamic_pointer_cast<MeshBasicMaterial>(mesh->material);
    REQUIRE(material != nullptr);
    REQUIRE(material->color.getHex() == 0x336699);

    // Both the geometry and the material dispatch their dispose event once per call.
    int geometryEvents = 0;
    int materialEvents = 0;
    mesh->geometry->addEventListener("dispose", [&geometryEvents](Event&) { ++geometryEvents; });
    mesh->material->addEventListener("dispose", [&materialEvents](Event&) { ++materialEvents; });
    coloured.dispose();
    REQUIRE(geometryEvents == 1);
    REQUIRE(materialEvents == 1);
    coloured.dispose();
    REQUIRE(geometryEvents == 2);
    REQUIRE(materialEvents == 2);
}
