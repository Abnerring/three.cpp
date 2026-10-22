#include "cameras/PerspectiveCamera.h"
#include "cameras/StereoCamera.h"

#include "catch.hpp"

#include <cmath>

// The expected numbers come from running the pinned StereoCamera.js in node and rounding
// the results to float, so these comparisons are exact at float precision.
TEST_CASE("StereoCamera update builds the off-axis projections upstream builds") {
    StereoCamera stereo;
    PerspectiveCamera camera(50, 16.0 / 9.0, 0.1, 100);
    camera.focus = 10;
    camera.zoom = 2;
    camera.updateMatrixWorld();
    stereo.update(camera);

    const auto f = [](double v) { return static_cast<float>(v); };
    const auto& left = stereo.cameraL->projectionMatrix.elements();
    const auto& right = stereo.cameraR->projectionMatrix.elements();

    REQUIRE(f(left[0]) == 2.4125702381134033f);
    REQUIRE(f(left[5]) == 2.1445069313049316f);
    REQUIRE(f(left[10]) == -1.0020020008087158f);
    REQUIRE(f(left[14]) == -0.20020020008087158f);
    // The off-axis term is symmetric between the eyes.
    REQUIRE(f(left[8]) == 0.007720225024968386f);
    REQUIRE(f(right[8]) == -0.007720225024968386f);

    // The eye matrices are the camera matrix translated by half the eye separation.
    REQUIRE(f(stereo.cameraL->matrix.elements()[12]) == -0.03200000151991844f);
    REQUIRE(f(stereo.cameraR->matrix.elements()[12]) == 0.03200000151991844f);
    REQUIRE(stereo.cameraL->matrixWorldNeedsUpdate);
    REQUIRE(stereo.cameraR->matrixWorldNeedsUpdate);
}

TEST_CASE("StereoCamera keeps the upstream defaults and layer assignment") {
    StereoCamera stereo;
    REQUIRE(stereo.type == "StereoCamera");
    REQUIRE(stereo.aspect == 1.0);
    REQUIRE(stereo.eyeSep == 0.064);
    // Left eye on layer 1, right eye on layer 2, and neither auto updates its matrix.
    REQUIRE(stereo.cameraL->layers.isEnabled(1));
    REQUIRE_FALSE(stereo.cameraL->layers.isEnabled(2));
    REQUIRE(stereo.cameraR->layers.isEnabled(2));
    REQUIRE_FALSE(stereo.cameraR->layers.isEnabled(1));
    REQUIRE_FALSE(stereo.cameraL->matrixAutoUpdate);
    REQUIRE_FALSE(stereo.cameraR->matrixAutoUpdate);
}

TEST_CASE("StereoCamera skips the projection rebuild when nothing changed") {
    StereoCamera stereo;
    PerspectiveCamera camera(50, 16.0 / 9.0, 0.1, 100);
    camera.updateMatrixWorld();
    stereo.update(camera);
    const auto before = stereo.cameraL->projectionMatrix.elements()[0];
    // Moving the camera alone must not touch the cached projection.
    camera.position.set(3, 4, 5);
    camera.updateMatrixWorld();
    stereo.update(camera);
    REQUIRE(stereo.cameraL->projectionMatrix.elements()[0] == before);
    // But the eye matrices do follow the camera.
    REQUIRE(stereo.cameraL->matrixWorldNeedsUpdate);
}
