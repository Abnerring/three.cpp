#include "catch.hpp"
#include "cameras/PerspectiveCamera.h"
#include "math/Matrix4.h"
#include "math/Vector3.h"
#include "utils/CameraUtils.h"
#include <cmath>

// The pinned frameCorners reads near/far/fov/aspect through JS duck typing; the native port
// takes PerspectiveCamera. The values are the ones the differential probe compares bit for bit.
TEST_CASE("CameraUtils frameCorners builds the pinned off-axis projection", "[camerautils]") {
    PerspectiveCamera camera(50, 1, 0.1, 2000);
    camera.position.set(0, 0, 5);
    CameraUtils::frameCorners(camera, Vector3(-1, -1, 0), Vector3(1, -1, 0), Vector3(-1, 1, 0));
    // The pinned code only touches fov when estimateViewFrustum is set.
    REQUIRE(camera.fov == 50);
    // The quaternion is a unit quaternion (the source normalises through setFromUnitVectors).
    const double length = camera.quaternion.x * camera.quaternion.x + camera.quaternion.y * camera.quaternion.y +
        camera.quaternion.z * camera.quaternion.z + camera.quaternion.w * camera.quaternion.w;
    REQUIRE(std::abs(length - 1.) < 1e-12);
    // The last row of the off-axis matrix is ( 0, 0, -1, 0 ) by construction.
    REQUIRE(camera.projectionMatrix.elements()[11] == -1.);
    REQUIRE(camera.projectionMatrix.elements()[15] == 0.);
    // projectionMatrixInverse is the inverse of projectionMatrix.
    Matrix4 product;
    product.multiplyMatrices(camera.projectionMatrix, camera.projectionMatrixInverse);
    for (int i = 0; i < 16; ++i) {
        const double expected = (i % 5 == 0) ? 1. : 0.;
        REQUIRE(std::abs(product.elements()[std::size_t(i)] - expected) < 1e-9);
    }
    // With the estimate flag the fov becomes the conservative estimate of the pinned formula.
    PerspectiveCamera estimated(50, 1, 0.1, 2000);
    estimated.position.set(0, 0, 5);
    CameraUtils::frameCorners(estimated, Vector3(-1, -1, 0), Vector3(1, -1, 0), Vector3(-1, 1, 0), true);
    REQUIRE(estimated.fov != 50);
    REQUIRE(estimated.fov > 0);
}