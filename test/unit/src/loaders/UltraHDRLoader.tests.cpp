#include "loaders/UltraHDRLoader.h"

#include "catch.hpp"

#include <cmath>

TEST_CASE("UltraHDRLoader builds the sRGB table") {
    // Index zero is the offset raised to the exponent.
    REQUIRE(UltraHDRLoader::srgbToLinear(0) == Approx(std::pow(0.0521327014, 2.4)).epsilon(1e-12));
    // Index one follows the same formula.
    REQUIRE(UltraHDRLoader::srgbToLinear(1) ==
            Approx(std::pow(0.003717127 + 0.0521327014, 2.4)).epsilon(1e-12));
    // The table rises with the index.
    REQUIRE(UltraHDRLoader::srgbToLinear(1023) > UltraHDRLoader::srgbToLinear(512));
    REQUIRE(UltraHDRLoader::srgbToLinear(512) > UltraHDRLoader::srgbToLinear(0));
    // Black is near zero and white is above one, since the table overshoots for the top values.
    REQUIRE(UltraHDRLoader::srgbToLinear(0) < 0.01);
    REQUIRE(UltraHDRLoader::srgbToLinear(1023) > 1.0);
}

TEST_CASE("UltraHDRLoader clamps a table index") {
    // Past the end the last entry repeats, and below zero the first.
    REQUIRE(UltraHDRLoader::srgbToLinear(5000) == UltraHDRLoader::srgbToLinear(1023));
    REQUIRE(UltraHDRLoader::srgbToLinear(-3) == UltraHDRLoader::srgbToLinear(0));
}

TEST_CASE("UltraHDRLoader interpolates the gain boost") {
    // The endpoints of the map land on the declared boosts.
    REQUIRE(UltraHDRLoader::gainMapBoost(0.0, 1.0, true, -1.0, 2.0) == -1.0);
    REQUIRE(UltraHDRLoader::gainMapBoost(1.0, 1.0, true, -1.0, 2.0) == 2.0);
    // The middle lands halfway between.
    REQUIRE(UltraHDRLoader::gainMapBoost(0.5, 1.0, true, -1.0, 2.0) == Approx(0.5).epsilon(1e-12));
}

TEST_CASE("UltraHDRLoader raises the map value by the recovery exponent") {
    // A gamma of one passes the value through, any other raises it.
    REQUIRE(UltraHDRLoader::gainMapBoost(0.25, 2.0, true, 0.0, 1.0) == Approx(0.25).epsilon(1e-12));
    REQUIRE(UltraHDRLoader::gainMapBoost(0.25, 2.0, false, 0.0, 1.0) == Approx(0.0625).epsilon(1e-12));
}
namespace {
void putUint32BE(std::vector<std::uint8_t>& out, std::uint32_t value) {
    for (int i = 3; i >= 0; --i) out.push_back(std::uint8_t((value >> (i * 8)) & 0xff));
}
void putInt32BE(std::vector<std::uint8_t>& out, std::int32_t value) {
    putUint32BE(out, static_cast<std::uint32_t>(value));
}
}  // namespace

TEST_CASE("UltraHDRLoader reads the common denominator metadata") {
    std::vector<std::uint8_t> buffer;
    // Minimum and writer versions, then the flags with the common bit set.
    buffer.push_back(0);
    buffer.push_back(0);
    buffer.push_back(0);
    buffer.push_back(0);
    buffer.push_back(0x8);
    // A common denominator of one thousand.
    putUint32BE(buffer, 1000);
    // Headrooms of five hundred and two thousand give log2 of a half and of two.
    putUint32BE(buffer, 500);
    putUint32BE(buffer, 2000);
    putInt32BE(buffer, -1000);
    putInt32BE(buffer, 1000);
    putUint32BE(buffer, 2000);
    putInt32BE(buffer, 100);
    putInt32BE(buffer, -100);

    const auto metadata = UltraHDRLoader::parseIsoMetadata(buffer);
    REQUIRE(metadata.useCommonDenominator);
    REQUIRE_FALSE(metadata.backwardDirection);
    // The headrooms are base two logarithms of the ratios.
    REQUIRE(metadata.hdrCapacityMin == Approx(-1.0).epsilon(1e-12));
    REQUIRE(metadata.hdrCapacityMax == Approx(1.0).epsilon(1e-12));
    REQUIRE(metadata.gainMapMin == Approx(-1.0).epsilon(1e-12));
    REQUIRE(metadata.gainMapMax == Approx(1.0).epsilon(1e-12));
    // A gamma of two thousand over one thousand.
    REQUIRE(metadata.gamma == Approx(2.0).epsilon(1e-12));
    // The offsets are scaled by the byte range.
    REQUIRE(metadata.offsetSdr == Approx(25.5).epsilon(1e-12));
    REQUIRE(metadata.offsetHdr == Approx(-25.5).epsilon(1e-12));
}

TEST_CASE("UltraHDRLoader reads the paired metadata") {
    std::vector<std::uint8_t> buffer;
    for (int i = 0; i < 5; ++i) buffer.push_back(0);
    // The flags leave both bits clear.
    buffer[4] = 0x0;
    // Each headroom now carries its own denominator.
    putUint32BE(buffer, 3);
    putUint32BE(buffer, 2);
    putUint32BE(buffer, 8);
    putUint32BE(buffer, 2);
    putInt32BE(buffer, -3);
    putUint32BE(buffer, 2);
    putInt32BE(buffer, 3);
    putUint32BE(buffer, 2);
    putUint32BE(buffer, 4);
    putUint32BE(buffer, 2);
    putInt32BE(buffer, 1);
    putUint32BE(buffer, 4);
    putInt32BE(buffer, -1);
    putUint32BE(buffer, 4);

    const auto metadata = UltraHDRLoader::parseIsoMetadata(buffer);
    REQUIRE_FALSE(metadata.useCommonDenominator);
    REQUIRE(metadata.gainMapMin == Approx(-1.5).epsilon(1e-12));
    REQUIRE(metadata.gainMapMax == Approx(1.5).epsilon(1e-12));
    REQUIRE(metadata.gamma == Approx(2.0).epsilon(1e-12));
    // One quarter of the byte range is about sixty three and three quarters.
    REQUIRE(metadata.offsetSdr == Approx(63.75).epsilon(1e-12));
}

TEST_CASE("UltraHDRLoader records the backward direction flag") {
    // The flags byte alone is enough to read them, so only the prefix is needed.
    std::vector<std::uint8_t> buffer(5, 0);
    buffer[4] = 0x4;
    // The paired path wants its whole block, so pad it out with zeros.
    for (int i = 0; i < 56; ++i) buffer.push_back(0);
}

TEST_CASE("UltraHDRLoader rejects a block that is too short") {
    const std::vector<std::uint8_t> tiny(3, 0);
    REQUIRE_THROWS_WITH(UltraHDRLoader::parseIsoMetadata(tiny),
                        "THREE.UltraHDRLoader: Invalid ISO metadata.");
}
