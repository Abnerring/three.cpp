#include "loaders/SPZLoader.h"

#include "catch.hpp"

#include <cmath>
#include <cstdint>
#include <vector>

TEST_CASE("SPZLoader reads half floats") {
    // One is zero sign, exponent fifteen and a zero fraction.
    REQUIRE(SPZLoader::fromHalfFloat(0x3c00) == 1.0);
    // Two doubles the exponent.
    REQUIRE(SPZLoader::fromHalfFloat(0x4000) == 2.0);
    // The sign bit negates.
    REQUIRE(SPZLoader::fromHalfFloat(0xbc00) == -1.0);
    // Zero is all zero bits.
    REQUIRE(SPZLoader::fromHalfFloat(0) == 0.0);
    // A half is a half.
    REQUIRE(SPZLoader::fromHalfFloat(0x3800) == 0.5);
}

TEST_CASE("SPZLoader reads version one centres as half floats") {
    // Two splats of three half floats each, all ones.
    const std::vector<std::uint8_t> bytes{0x00, 0x3c, 0x00, 0x3c, 0x00, 0x3c,
                                          0x00, 0x40, 0x00, 0x40, 0x00, 0x40};
    const auto centers = SPZLoader::readCenters(bytes, 0, 2, 1, 0);
    REQUIRE(centers.size() == 6);
    REQUIRE(centers[0] == 1.0);
    REQUIRE(centers[2] == 1.0);
    // The second splat is twos.
    REQUIRE(centers[3] == 2.0);
}

TEST_CASE("SPZLoader reads fixed point centres") {
    // One splat of three int24 values, each 0x000100 which is two hundred and fifty six.
    const std::vector<std::uint8_t> bytes{0x00, 0x01, 0x00, 0x00, 0x01, 0x00, 0x00, 0x01, 0x00};
    const auto centers = SPZLoader::readCenters(bytes, 0, 1, 2, 8);
    REQUIRE(centers.size() == 3);
    // Two hundred and fifty six over two to the eight is one.
    REQUIRE(centers[0] == 1.0);
    REQUIRE(centers[2] == 1.0);
}

TEST_CASE("SPZLoader scales fixed point centres by the fractional bits") {
    const std::vector<std::uint8_t> bytes{0x00, 0x01, 0x00, 0x00, 0x01, 0x00, 0x00, 0x01, 0x00};
    // One fractional bit halves the value.
    const auto centers = SPZLoader::readCenters(bytes, 0, 1, 2, 9);
    REQUIRE(centers[0] == 0.5);
}

TEST_CASE("SPZLoader reads negative fixed point centres") {
    // All ones is minus one, which over two to the eighth stays very small.
    const std::vector<std::uint8_t> bytes{0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
    const auto centers = SPZLoader::readCenters(bytes, 0, 1, 2, 0);
    REQUIRE(centers[0] == -1.0);
}

TEST_CASE("SPZLoader stops when the bytes run out") {
    // Two splats declared but only one splat of bytes present.
    const std::vector<std::uint8_t> bytes{0x00, 0x3c, 0x00, 0x3c, 0x00, 0x3c};
    const auto centers = SPZLoader::readCenters(bytes, 0, 2, 1, 0);
    REQUIRE(centers.size() == 3);
}

TEST_CASE("SPZLoader maps a colour byte through the scale") {
    // The midpoint is close to the middle of the range, and the extremes are further out
    // because the scale is larger than one.
    const double middle = SPZLoader::colorLut(128);
    REQUIRE(middle > 127.0);
    const double low = SPZLoader::colorLut(0);
    const double high = SPZLoader::colorLut(255);
    REQUIRE(low < 0.0);
    REQUIRE(high > 255.0);
    // The mapping is linear, so the midpoint sits near the average of the extremes.
    REQUIRE(middle == Approx((low + high) / 2.0).margin(1.0));
}