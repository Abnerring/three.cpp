#include "loaders/SPZLoader.h"

#include "catch.hpp"

#include <cmath>
#include <cstdint>

TEST_CASE("SPZLoader decodes a ten bit quaternion component") {
    // Zero magnitude is zero, whatever the sign.
    REQUIRE(SPZLoader::quatComponent(0) == 0.0);
    REQUIRE(SPZLoader::quatComponent(512) == 0.0);
    // All ones in the magnitude is the largest component, one over root two.
    REQUIRE(SPZLoader::quatComponent(511) == Approx(0.7071067811865476).epsilon(1e-12));
    // Bit nine negates it.
    REQUIRE(SPZLoader::quatComponent(511 | 512) == Approx(-0.7071067811865476).epsilon(1e-12));
    // Half the magnitude is half the largest.
    REQUIRE(SPZLoader::quatComponent(255) == Approx(0.7071067811865476 * 255.0 / 511.0).epsilon(1e-12));
}

TEST_CASE("SPZLoader reconstructs the identity quaternion") {
    // All three fields zero with the largest index naming w.
    const auto q = SPZLoader::readSmallestThreeQuaternion(3u << 30);
    REQUIRE(q[0] == 0.0);
    REQUIRE(q[1] == 0.0);
    REQUIRE(q[2] == 0.0);
    // The omitted component follows from unit length.
    REQUIRE(q[3] == Approx(1.0).epsilon(1e-12));
}

TEST_CASE("SPZLoader fills the remaining components in descending order") {
    // The largest is index three, so the low field lands on index two.
    const std::uint32_t packed = (3u << 30) | 511u;
    const auto q = SPZLoader::readSmallestThreeQuaternion(packed);
    REQUIRE(q[0] == 0.0);
    REQUIRE(q[1] == 0.0);
    REQUIRE(q[2] == Approx(0.7071067811865476).epsilon(1e-12));
    // The omitted w is the same magnitude here.
    REQUIRE(q[3] == Approx(0.7071067811865476).epsilon(1e-12));
}

TEST_CASE("SPZLoader puts the high field on the lowest index") {
    // The middle field is set, so with the largest at three it lands on index one.
    const std::uint32_t packed = (3u << 30) | (511u << 10);
    const auto q = SPZLoader::readSmallestThreeQuaternion(packed);
    REQUIRE(q[0] == 0.0);
    REQUIRE(q[1] == Approx(0.7071067811865476).epsilon(1e-12));
    REQUIRE(q[2] == 0.0);
}

TEST_CASE("SPZLoader omits the named component") {
    // With the largest at zero, x is the reconstructed one and the fields fill y z w.
    const std::uint32_t packed = (0u << 30) | 511u;
    const auto q = SPZLoader::readSmallestThreeQuaternion(packed);
    // The low field goes to the highest remaining index, which is three.
    REQUIRE(q[3] == Approx(0.7071067811865476).epsilon(1e-12));
    REQUIRE(q[0] == Approx(0.7071067811865476).epsilon(1e-12));
}

TEST_CASE("SPZLoader keeps the reconstructed component real") {
    // Three full magnitude components leave nothing for the fourth.
    const std::uint32_t packed = (3u << 30) | 511u | (511u << 10) | (511u << 20);
    const auto q = SPZLoader::readSmallestThreeQuaternion(packed);
    REQUIRE(q[3] >= 0.0);
}

TEST_CASE("SPZLoader scales a Gaussian size from its byte") {
    // The table is an exponential with sixteen steps per unit of exponent.
    REQUIRE(SPZLoader::scaleFromByte(0) == Approx(std::exp(-10.0)).epsilon(1e-12));
    REQUIRE(SPZLoader::scaleFromByte(160) == Approx(1.0).epsilon(1e-12));
    REQUIRE(SPZLoader::scaleFromByte(255) == Approx(std::exp(255.0 / 16.0 - 10.0)).epsilon(1e-12));
}