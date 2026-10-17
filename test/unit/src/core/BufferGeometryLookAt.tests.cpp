#include "catch.hpp"
#include "core/BufferAttribute.h"
#include "core/BufferGeometry.h"
#include "math/Vector3.h"
#include <cmath>
#include <memory>
#include <vector>

// lookAt routes through a scratch Object3D exactly as upstream does, so looking from the origin towards -Z
// is a half turn about Y: x and z change sign and y is untouched.
//
// Object3D.intersectsFrustum was also added here and then reverted: giving the base class that virtual
// hook changed the vtable and crashed an unrelated suite partway through, so it is left for a round that
// can chase that properly rather than being shipped on top of a green tree that is not green.
TEST_CASE("BufferGeometry.lookAt turns the geometry towards the given point", "[buffergeometry]") {
    auto geometry = std::make_shared<BufferGeometry>();
    auto position = std::make_shared<Float32BufferAttribute>(std::vector<float>{1.0f, 2.0f, 3.0f}, 3);
    geometry->setAttribute("position", position);
    geometry->lookAt(Vector3(0, 0, -1));
    REQUIRE(std::abs(position->getX(0) + 1.0) < 1e-5);   // x flips
    REQUIRE(std::abs(position->getY(0) - 2.0) < 1e-5);   // y is untouched
    REQUIRE(std::abs(position->getZ(0) + 3.0) < 1e-5);   // z flips
}