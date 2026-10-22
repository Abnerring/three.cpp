#include "loaders/SPZLoader.h"

#include "catch.hpp"

#include <cmath>
#include <cstdint>
#include <vector>

TEST_CASE("SPZLoader reads a signed twenty four bit integer") {
    // All ones sign extends to minus one.
    REQUIRE(SPZLoader::readInt24({0xff, 0xff, 0xff}, 0) == -1);
    // A small positive value.
    REQUIRE(SPZLoader::readInt24({1, 0, 0}, 0) == 1);
    // The most negative twenty four bit value.
    REQUIRE(SPZLoader::readInt24({0x00, 0x00, 0x80}, 0) == -8388608);
    // The largest positive value.
    REQUIRE(SPZLoader::readInt24({0xff, 0xff, 0x7f}, 0) == 8388607);
}

TEST_CASE("SPZLoader reads the int24 at an offset") {
    const std::vector<std::uint8_t> bytes{9, 9, 2, 0, 0};
    REQUIRE(SPZLoader::readInt24(bytes, 2) == 2);
    // A read past the end is zero rather than undefined.
    REQUIRE(SPZLoader::readInt24(bytes, 3) == 0);
}

TEST_CASE("SPZLoader maps a byte to the closed range") {
    // Zero is minus one, one hundred and twenty eight is just above zero because the divisor
    // is a half step above the maximum.
    const auto low = SPZLoader::readXYZQuaternion({0, 0, 0}, 0);
    REQUIRE(low[0] == Approx(-1.0).epsilon(1e-12));
    REQUIRE(low[3] == Approx(0.0).margin(1e-6));

    const auto middle = SPZLoader::readXYZQuaternion({128, 128, 128}, 0);
    REQUIRE(middle[0] == Approx(128.0 / 127.5 - 1.0).epsilon(1e-12));
    // Nearly unit length with three small components.
    const double length = middle[0] * middle[0] + middle[1] * middle[1] + middle[2] * middle[2] +
                          middle[3] * middle[3];
    REQUIRE(length == Approx(1.0).margin(1e-9));
}

TEST_CASE("SPZLoader reconstructs the fourth component") {
    // Pure w with zero xyz: the bytes are the midpoint, so the result is nearly identity.
    const auto identity = SPZLoader::readXYZQuaternion({127, 127, 127}, 0);
    REQUIRE(identity[3] == Approx(1.0).margin(1e-4));
}

TEST_CASE("SPZLoader clamps the fourth component at zero") {
    // Three complete components leave nothing for w, and rounding must not make it imaginary.
    const auto corner = SPZLoader::readXYZQuaternion({255, 0, 0}, 0);
    REQUIRE(corner[0] == Approx(1.0).epsilon(1e-12));
    REQUIRE(corner[3] >= 0.0);
}