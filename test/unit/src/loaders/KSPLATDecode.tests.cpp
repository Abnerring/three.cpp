#include "loaders/KSPLATLoader.h"

#include "catch.hpp"

#include <cstdint>
#include <cstring>
#include <vector>

namespace {
void putFloat32(std::vector<std::uint8_t>& out, std::size_t at, float value) {
    std::uint32_t bits = 0;
    std::memcpy(&bits, &value, sizeof(bits));
    for (int i = 0; i < 4; ++i) out[at + i] = std::uint8_t((bits >> (i * 8)) & 0xff);
}
void putUint16(std::vector<std::uint8_t>& out, std::size_t at, std::uint16_t value) {
    out[at] = std::uint8_t(value & 0xff);
    out[at + 1] = std::uint8_t((value >> 8) & 0xff);
}
}  // namespace

TEST_CASE("KSPLATLoader reads a full float vector as floats") {
    std::vector<std::uint8_t> buffer(16, 0);
    putFloat32(buffer, 0, 1.5f);
    putFloat32(buffer, 4, 2.5f);
    // A twelve or sixteen byte vector is plain floats.
    REQUIRE(KSPLATLoader::readCompressedFloat(buffer, 0, 12) == Approx(1.5).epsilon(1e-6));
    REQUIRE(KSPLATLoader::readCompressedFloat(buffer, 4, 16) == Approx(2.5).epsilon(1e-6));
}

TEST_CASE("KSPLATLoader reads a narrow vector as half floats") {
    std::vector<std::uint8_t> buffer(8, 0);
    // One as a half float.
    putUint16(buffer, 0, 0x3c00);
    putUint16(buffer, 2, 0x4000);
    REQUIRE(KSPLATLoader::readCompressedFloat(buffer, 0, 6) == 1.0);
    REQUIRE(KSPLATLoader::readCompressedFloat(buffer, 2, 6) == 2.0);
}

TEST_CASE("KSPLATLoader reads a one byte harmonic as a fraction of the range") {
    const std::vector<std::uint8_t> buffer{0, 128, 255};
    // The ends map to the declared range and the middle to its midpoint.
    REQUIRE(KSPLATLoader::readCompressedHarmonic(buffer, 0, 1, -1.5, 1.5) == Approx(-1.5).epsilon(1e-12));
    REQUIRE(KSPLATLoader::readCompressedHarmonic(buffer, 2, 1, -1.5, 1.5) == Approx(1.5).epsilon(1e-12));
    REQUIRE(KSPLATLoader::readCompressedHarmonic(buffer, 1, 1, -1.5, 1.5) ==
            Approx(-1.5 + (128.0 / 255.0) * 3.0).epsilon(1e-12));
}

TEST_CASE("KSPLATLoader reads wider harmonics directly") {
    std::vector<std::uint8_t> buffer(8, 0);
    putFloat32(buffer, 0, 0.25f);
    putUint16(buffer, 4, 0x3c00);
    REQUIRE(KSPLATLoader::readCompressedHarmonic(buffer, 0, 4, -1.5, 1.5) == Approx(0.25).epsilon(1e-6));
    REQUIRE(KSPLATLoader::readCompressedHarmonic(buffer, 4, 2, -1.5, 1.5) == 1.0);
}

TEST_CASE("KSPLATLoader assigns the bucket of a splat") {
    KSPLATLoader::SectionHeader section;
    section.bucketSize = 4;
    section.bucketCount = 10;
    section.fullBucketCount = 2;
    // No buckets at all puts everything in bucket zero.
    KSPLATLoader::SectionHeader none;
    REQUIRE(KSPLATLoader::bucketIndex(5, none, 0, {}).value == 0);
    // Eight splats fill the two full buckets of four.
    REQUIRE(KSPLATLoader::bucketIndex(0, section, 8, {}).value == 0);
    REQUIRE(KSPLATLoader::bucketIndex(3, section, 8, {}).value == 0);
    REQUIRE(KSPLATLoader::bucketIndex(4, section, 8, {}).value == 1);
    REQUIRE(KSPLATLoader::bucketIndex(7, section, 8, {}).value == 1);
}

TEST_CASE("KSPLATLoader walks the partial buckets") {
    KSPLATLoader::SectionHeader section;
    section.bucketSize = 4;
    section.bucketCount = 4;
    section.fullBucketCount = 2;
    // Two full buckets hold eight, then the partial lengths are three and two.
    const std::vector<std::uint32_t> lengths{3, 2};
    REQUIRE(KSPLATLoader::bucketIndex(8, section, 8, lengths).value == 2);
    REQUIRE(KSPLATLoader::bucketIndex(10, section, 8, lengths).value == 2);
    REQUIRE(KSPLATLoader::bucketIndex(11, section, 8, lengths).value == 3);
    REQUIRE(KSPLATLoader::bucketIndex(12, section, 8, lengths).value == 3);
    // The base of the second partial bucket starts after the first.
    REQUIRE(KSPLATLoader::bucketIndex(11, section, 8, lengths).partialBase == 11);
}

TEST_CASE("KSPLATLoader reports a splat past every bucket") {
    KSPLATLoader::SectionHeader section;
    section.bucketSize = 4;
    section.bucketCount = 3;
    section.fullBucketCount = 2;
    // One partial bucket of two leaves no room for a thirteenth splat.
    REQUIRE_THROWS_WITH(KSPLATLoader::bucketIndex(13, section, 8, {2}),
                        "THREE.KSPLATLoader: Invalid KSPLAT bucket data.");
}