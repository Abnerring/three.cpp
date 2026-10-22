#include "loaders/UltraHDRLoader.h"

#include "catch.hpp"

#include <cmath>
#include <cstdint>
#include <vector>

namespace {
void putUint32BE(std::vector<std::uint8_t>& out, std::uint32_t value) {
    for (int i = 3; i >= 0; --i) out.push_back(std::uint8_t((value >> (i * 8)) & 0xff));
}
void putInt32BE(std::vector<std::uint8_t>& out, std::int32_t value) {
    putUint32BE(out, static_cast<std::uint32_t>(value));
}
}  // namespace

TEST_CASE("UltraHDRLoader converts an sRGB sample in three ranges") {
    // Below about ten and a third the curve is the linear segment.
    REQUIRE(UltraHDRLoader::srgbToLinear(0.0) == 0.0);
    REQUIRE(UltraHDRLoader::srgbToLinear(5.0) == Approx(5.0 * 0.000303527).epsilon(1e-12));
    // Inside the table the formula gives the same value as the table entry would.
    REQUIRE(UltraHDRLoader::srgbToLinear(100.0) ==
            Approx(std::pow(100.0 * 0.003717127 + 0.0521327014, 2.4)).epsilon(1e-9));
    // A fractional sample is truncated to an index, not interpolated.
    REQUIRE(UltraHDRLoader::srgbToLinear(100.9) == UltraHDRLoader::srgbToLinear(100.0));
    // Past the table the formula applies to the value as it is.
    REQUIRE(UltraHDRLoader::srgbToLinear(2000.0) ==
            Approx(std::pow(2000.0 * 0.003717127 + 0.0521327014, 2.4)).epsilon(1e-9));
    // The conversion rises with the sample.
    REQUIRE(UltraHDRLoader::srgbToLinear(1000.0) > UltraHDRLoader::srgbToLinear(100.0));
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

TEST_CASE("UltraHDRLoader composes a gain map onto an sRGB sample") {
    // No boost leaves the sample alone, then the conversion applies.
    const double plain = UltraHDRLoader::composeHdr(100.0, 0.0, 1.0, 0.0, 0.0);
    REQUIRE(plain == Approx(UltraHDRLoader::srgbToLinear(100.0)).epsilon(1e-12));
    // A boost of one doubles the value before conversion.
    const double doubled = UltraHDRLoader::composeHdr(100.0, 1.0, 1.0, 0.0, 0.0);
    REQUIRE(doubled == Approx(UltraHDRLoader::srgbToLinear(200.0)).epsilon(1e-12));
    // The weight multiplies into the exponent, so a half weight is a square root.
    const double half = UltraHDRLoader::composeHdr(100.0, 1.0, 0.5, 0.0, 0.0);
    REQUIRE(half == Approx(UltraHDRLoader::srgbToLinear(100.0 * std::sqrt(2.0))).epsilon(1e-9));
}

TEST_CASE("UltraHDRLoader applies the offsets and clamps") {
    // The sdr offset lifts the sample and the hdr offset lowers the result.
    REQUIRE(UltraHDRLoader::composeHdr(100.0, 0.0, 1.0, 10.0, 0.0) ==
            Approx(UltraHDRLoader::srgbToLinear(110.0)).epsilon(1e-12));
    REQUIRE(UltraHDRLoader::composeHdr(100.0, 0.0, 1.0, 0.0, 10.0) ==
            Approx(UltraHDRLoader::srgbToLinear(90.0)).epsilon(1e-12));
    // A negative result clamps to zero rather than going below it.
    REQUIRE(UltraHDRLoader::composeHdr(0.0, 0.0, 1.0, 0.0, 1000.0) == 0.0);
}

TEST_CASE("UltraHDRLoader reads the common denominator metadata") {
    std::vector<std::uint8_t> buffer;
    buffer.push_back(0);
    buffer.push_back(0);
    buffer.push_back(0);
    buffer.push_back(0);
    buffer.push_back(0x8);
    putUint32BE(buffer, 1000);
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
    REQUIRE(metadata.hdrCapacityMin == Approx(-1.0).epsilon(1e-12));
    REQUIRE(metadata.hdrCapacityMax == Approx(1.0).epsilon(1e-12));
    REQUIRE(metadata.gainMapMin == Approx(-1.0).epsilon(1e-12));
    REQUIRE(metadata.gainMapMax == Approx(1.0).epsilon(1e-12));
    REQUIRE(metadata.gamma == Approx(2.0).epsilon(1e-12));
    REQUIRE(metadata.offsetSdr == Approx(25.5).epsilon(1e-12));
    REQUIRE(metadata.offsetHdr == Approx(-25.5).epsilon(1e-12));
}

TEST_CASE("UltraHDRLoader reads the paired metadata") {
    std::vector<std::uint8_t> buffer;
    for (int i = 0; i < 5; ++i) buffer.push_back(0);
    buffer[4] = 0x0;
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
    REQUIRE(metadata.offsetSdr == Approx(63.75).epsilon(1e-12));
}

TEST_CASE("UltraHDRLoader records the backward direction flag") {
    // The paired path needs its whole block, so the buffer is padded out.
    std::vector<std::uint8_t> buffer(61, 0);
    buffer[4] = 0x4;
    const auto metadata = UltraHDRLoader::parseIsoMetadata(buffer);
    REQUIRE(metadata.backwardDirection);
    REQUIRE_FALSE(metadata.useCommonDenominator);
}

TEST_CASE("UltraHDRLoader rejects a block that is too short") {
    const std::vector<std::uint8_t> tiny(3, 0);
    REQUIRE_THROWS_WITH(UltraHDRLoader::parseIsoMetadata(tiny),
                        "THREE.UltraHDRLoader: Invalid ISO metadata.");
}
