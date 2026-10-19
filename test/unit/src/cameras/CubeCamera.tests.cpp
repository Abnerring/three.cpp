#include "catch.hpp"
#include "cameras/CubeCamera.h"
#include "cameras/PerspectiveCamera.h"
#include "core/JSValue.h"
#include <stdexcept>

// The rig is six perspective cameras at a negative field of view sharing the cube layers, and the coordinate
// system decides which way each face points.
TEST_CASE("CubeCamera builds six faces and orients them for the coordinate system") {
    const auto realm = JSValue::object();
    static_cast<void>(realm);
    auto renderTarget = JSValue::object();
    CubeCamera cube(0.5, 500.0, renderTarget);
    REQUIRE(cube.type == "CubeCamera");
    REQUIRE(cube.children.size() == 6);
    REQUIRE(cube.activeMipmapLevel == 0.0);

    const auto first = std::dynamic_pointer_cast<PerspectiveCamera>(cube.children[0]);
    REQUIRE(first);
    REQUIRE(first->fov == -90.0);
    REQUIRE(first->aspect == 1.0);
    REQUIRE(first->near == 0.5);
    REQUIRE(first->far == 500.0);

    cube.coordinateSystem = JSValue(WebGLCoordinateSystem);
    cube.updateCoordinateSystem();
    REQUIRE(cube.children.size() == 6);
    Vector3 direction;
    auto face = [&](int index) {
        const auto camera = std::dynamic_pointer_cast<PerspectiveCamera>(cube.children[index]);
        return camera->getWorldDirection(direction);
    };
    REQUIRE(face(0).x == 1.0);          // positive x
    REQUIRE(face(1).x == -1.0);         // negative x
    REQUIRE(face(2).y == 1.0);          // positive y
    REQUIRE(face(3).y == -1.0);         // negative y
    REQUIRE(face(4).z == 1.0);          // positive z
    REQUIRE(face(5).z == -1.0);         // negative z

    // Rendering the faces needs a renderer, which is refused by name rather than silently skipped.
    REQUIRE_THROWS_AS(cube.update(), std::runtime_error);
    REQUIRE_THROWS_AS(cube.clear(), std::runtime_error);
}