#include "loaders/GaussianSplatPLYLoader.h"

#include "catch.hpp"

namespace {
GaussianSplatPLYLoader::Attributes oneSplat() {
    GaussianSplatPLYLoader::Attributes attributes;
    attributes.position = {1, 2, 3};
    // Log scale of zero means a size of one.
    attributes.scale = {0, 0, 0};
    // An identity rotation stored as w x y z.
    attributes.rotation = {1, 0, 0, 0};
    attributes.fDc = {0, 0, 0};
    attributes.opacity = {0};
    return attributes;
}
}  // namespace

TEST_CASE("GaussianSplatPLYLoader converts one splat") {
    const auto converted = GaussianSplatPLYLoader::convertGeometry(oneSplat());
    REQUIRE(converted.count == 1);
    REQUIRE(converted.centers.size() == 3);
    REQUIRE(converted.centers[0] == 1.0);
    REQUIRE(converted.centers[2] == 3.0);
    // Six covariance components and four colour bytes.
    REQUIRE(converted.covariances.size() == 6);
    REQUIRE(converted.colors.size() == 4);
}

TEST_CASE("GaussianSplatPLYLoader raises the log scale") {
    auto attributes = oneSplat();
    // A log scale of zero gives unit size, so the covariance is the identity.
    const auto unit = GaussianSplatPLYLoader::convertGeometry(attributes);
    REQUIRE(unit.covariances[0] == Approx(1.0).margin(1e-6));
    // The diagonal entries are the squared sizes.
    REQUIRE(unit.covariances[3] == Approx(1.0).margin(1e-6));

    // A log scale of the natural log of two doubles the size.
    attributes.scale = {std::log(2.0), 0, 0};
    const auto larger = GaussianSplatPLYLoader::convertGeometry(attributes);
    REQUIRE(larger.covariances[0] == Approx(4.0).margin(1e-6));
}

TEST_CASE("GaussianSplatPLYLoader reads the rotation as w first") {
    auto attributes = oneSplat();
    // A rotation of ninety degrees about z: w and z are both root half.
    const double h = std::sqrt(0.5);
    attributes.rotation = {h, 0, 0, h};
    const auto converted = GaussianSplatPLYLoader::convertGeometry(attributes);
    // The covariance is no longer diagonal because the rotation mixes x and y.
    REQUIRE(converted.covariances[1] == Approx(0.0).margin(1e-6));
    REQUIRE(converted.covariances[0] == Approx(1.0).margin(1e-6));
    REQUIRE(converted.covariances[3] == Approx(1.0).margin(1e-6));
}

TEST_CASE("GaussianSplatPLYLoader makes the colour opaque") {
    const auto converted = GaussianSplatPLYLoader::convertGeometry(oneSplat());
    // A sigmoid of zero is a half, so the alpha reaches its middle value.
    REQUIRE(converted.colors[3] == Approx(128).margin(2));
}

TEST_CASE("GaussianSplatPLYLoader rejects a missing stream") {
    auto attributes = oneSplat();
    attributes.opacity.clear();
    REQUIRE_THROWS_WITH(
        GaussianSplatPLYLoader::convertGeometry(attributes),
        "THREE.GaussianSplatPLYLoader: PLY file requires position, scale, rotation, f_dc and "
        "opacity properties.");
}

TEST_CASE("GaussianSplatPLYLoader rejects a wrong width") {
    auto attributes = oneSplat();
    // A rotation of three components cannot be a quaternion.
    attributes.rotation = {1, 0, 0};
    REQUIRE_THROWS_WITH(
        GaussianSplatPLYLoader::convertGeometry(attributes),
        "THREE.GaussianSplatPLYLoader: Invalid Gaussian splat PLY property itemSize.");
}

TEST_CASE("GaussianSplatPLYLoader rejects mismatched counts") {
    auto attributes = oneSplat();
    // Two splats of scale but only one position.
    attributes.scale = {0, 0, 0, 0, 0, 0};
    REQUIRE_THROWS_WITH(
        GaussianSplatPLYLoader::convertGeometry(attributes),
        "THREE.GaussianSplatPLYLoader: Gaussian splat PLY property counts must match position.");
}
