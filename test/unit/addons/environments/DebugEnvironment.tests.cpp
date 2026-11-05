#include "environments/DebugEnvironment.h"

#include "constants.h"
#include "core/EventDispatcher.h"
#include "lights/PointLight.h"
#include "materials/MeshLambertMaterial.h"
#include "materials/MeshStandardMaterial.h"
#include "objects/Mesh.h"

#include "catch.hpp"

#include <memory>

TEST_CASE("DebugEnvironment builds the room, the light and three boxes") {
    DebugEnvironment environment;
    REQUIRE(environment.children.size() == 5);

    const auto room = std::dynamic_pointer_cast<Mesh>(environment.children[0]);
    REQUIRE(room != nullptr);
    REQUIRE(room->geometry->getAttribute("uv") == nullptr);
    const auto roomMaterial = std::dynamic_pointer_cast<MeshStandardMaterial>(room->material);
    REQUIRE(roomMaterial != nullptr);
    REQUIRE(roomMaterial->metalness == Approx(0));
    REQUIRE(roomMaterial->side == BackSide);
    REQUIRE(room->scale.x == Approx(10));
    REQUIRE(room->scale.y == Approx(10));
    REQUIRE(room->scale.z == Approx(10));

    const auto mainLight = std::dynamic_pointer_cast<PointLight>(environment.children[1]);
    REQUIRE(mainLight != nullptr);
    REQUIRE(mainLight->intensity == Approx(50));
    REQUIRE(mainLight->distance == Approx(0));
    REQUIRE(mainLight->decay == Approx(2));

    const auto first = std::dynamic_pointer_cast<Mesh>(environment.children[2]);
    REQUIRE(first != nullptr);
    REQUIRE(first->position.x == Approx(-5));
    REQUIRE(first->position.y == Approx(2));
    REQUIRE(first->position.z == Approx(0));
    REQUIRE(first->scale.x == Approx(0.1));
    REQUIRE(first->scale.y == Approx(1));
    const auto firstMaterial = std::dynamic_pointer_cast<MeshLambertMaterial>(first->material);
    REQUIRE(firstMaterial != nullptr);
    REQUIRE(firstMaterial->color.getHex() == 0xff0000);
    REQUIRE(firstMaterial->emissive.getHex() == 0xffffff);
    REQUIRE(firstMaterial->emissiveIntensity == Approx(10));

    const auto second = std::dynamic_pointer_cast<Mesh>(environment.children[3]);
    REQUIRE(second->position.y == Approx(5));
    REQUIRE(second->scale.y == Approx(0.1));
    REQUIRE(std::dynamic_pointer_cast<MeshLambertMaterial>(second->material)->color.getHex() == 0x00ff00);

    const auto third = std::dynamic_pointer_cast<Mesh>(environment.children[4]);
    REQUIRE(third->position.z == Approx(5));
    REQUIRE(third->scale.x == Approx(1.5));
    REQUIRE(third->scale.z == Approx(0.1));
    REQUIRE(std::dynamic_pointer_cast<MeshLambertMaterial>(third->material)->color.getHex() == 0x0000ff);
}

TEST_CASE("DebugEnvironment disposes every shared resource once") {
    DebugEnvironment environment;
    const auto room = std::dynamic_pointer_cast<Mesh>(environment.children[0]);
    const auto first = std::dynamic_pointer_cast<Mesh>(environment.children[2]);
    REQUIRE(room->geometry == first->geometry);

    int geometryEvents = 0;
    int materialEvents = 0;
    room->geometry->addEventListener("dispose", [&geometryEvents](Event&) { ++geometryEvents; });
    for (const auto& child : environment.children) {
        const auto mesh = std::dynamic_pointer_cast<Mesh>(child);
        if (mesh && mesh->material) {
            mesh->material->addEventListener("dispose", [&materialEvents](Event&) { ++materialEvents; });
        }
    }

    environment.dispose();
    // The four meshes share one geometry, and the four materials are distinct.
    REQUIRE(geometryEvents == 1);
    REQUIRE(materialEvents == 4);

    environment.dispose();
    REQUIRE(geometryEvents == 2);
    REQUIRE(materialEvents == 8);
}
