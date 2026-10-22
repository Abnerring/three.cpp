#include "loaders/LDrawLoader.h"

#include "catch.hpp"

#include <array>
#include <cmath>

TEST_CASE("LDrawLoader computes a face normal") {
    // A triangle in the xy plane facing up.
    const std::array<double, 3> v0{0, 0, 0};
    const std::array<double, 3> v1{1, 0, 0};
    const std::array<double, 3> v2{0, 1, 0};
    const auto normal = LDrawLoader::faceNormal(v0, v1, v2);
    REQUIRE(normal[0] == 0.0);
    REQUIRE(normal[1] == 0.0);
    REQUIRE(normal[2] == Approx(1.0).epsilon(1e-12));
}

TEST_CASE("LDrawLoader normalises the face normal") {
    // A larger triangle must give the same unit normal.
    const std::array<double, 3> v0{0, 0, 0};
    const std::array<double, 3> v1{10, 0, 0};
    const std::array<double, 3> v2{0, 10, 0};
    const auto normal = LDrawLoader::faceNormal(v0, v1, v2);
    const double length = std::sqrt(normal[0] * normal[0] + normal[1] * normal[1] +
                                    normal[2] * normal[2]);
    REQUIRE(length == Approx(1.0).epsilon(1e-12));
    REQUIRE(normal[2] == Approx(1.0).epsilon(1e-12));
}

TEST_CASE("LDrawLoader gives a degenerate face no normal") {
    // Three points on a line enclose no area.
    const std::array<double, 3> v0{0, 0, 0};
    const std::array<double, 3> v1{1, 0, 0};
    const std::array<double, 3> v2{2, 0, 0};
    const auto normal = LDrawLoader::faceNormal(v0, v1, v2);
    REQUIRE(normal[0] == 0.0);
    REQUIRE(normal[1] == 0.0);
    REQUIRE(normal[2] == 0.0);
}

TEST_CASE("LDrawLoader smooths a pair of faces within the threshold") {
    // Two faces pointing the same way smooth together.
    REQUIRE(LDrawLoader::isSmoothPair(1.0));
    // Exactly at the threshold still smooths.
    REQUIRE(LDrawLoader::isSmoothPair(LDrawLoader::kHardEdgeCosine));
    // Just below it does not.
    REQUIRE_FALSE(LDrawLoader::isSmoothPair(LDrawLoader::kHardEdgeCosine - 1e-6));
}

TEST_CASE("LDrawLoader smooths a pair facing apart") {
    // The magnitude is compared, so opposite faces are smooth too.
    REQUIRE(LDrawLoader::isSmoothPair(-1.0));
    REQUIRE(LDrawLoader::isSmoothPair(-LDrawLoader::kHardEdgeCosine));
    REQUIRE_FALSE(LDrawLoader::isSmoothPair(-LDrawLoader::kHardEdgeCosine + 1e-6));
}

TEST_CASE("LDrawLoader measures the hard edge threshold") {
    // The constant is a quarter, which is nearer seventy five degrees than the sixty seven and
    // a half the source comment names.
    REQUIRE(LDrawLoader::kHardEdgeCosine == 0.25);
    const double degrees = std::acos(LDrawLoader::kHardEdgeCosine) * 180.0 / 3.14159265358979;
    REQUIRE(degrees == Approx(75.52).margin(0.01));
    // The angle the comment names would be a larger cosine and therefore a stricter test.
    const double commentCosine = std::cos(67.5 * 3.14159265358979 / 180.0);
    REQUIRE(commentCosine > LDrawLoader::kHardEdgeCosine);
}
