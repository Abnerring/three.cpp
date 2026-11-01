#include "loaders/KSPLATLoader.h"

#include "catch.hpp"

#include <cstdint>
#include <cstring>
#include <vector>

namespace {
void putUint32(std::vector<std::uint8_t>& out, std::size_t at, std::uint32_t value) {
    for (int i = 0; i < 4; ++i) out[at + i] = std::uint8_t((value >> (i * 8)) & 0xff);
}
void putUint16(std::vector<std::uint8_t>& out, std::size_t at, std::uint16_t value) {
    out[at] = std::uint8_t(value & 0xff);
    out[at + 1] = std::uint8_t((value >> 8) & 0xff);
}
void putFloat32(std::vector<std::uint8_t>& out, std::size_t at, float value) {
    std::uint32_t bits = 0;
    std::memcpy(&bits, &value, sizeof(bits));
    putUint32(out, at, bits);
}
std::vector<std::uint8_t> headerBytes(float minCoefficient, float maxCoefficient) {
    std::vector<std::uint8_t> out(4096, 0);
    out[0] = 0;
    out[1] = 1;
    putUint32(out, 4, 8);
    putUint32(out, 8, 3);
    putUint32(out, 12, 1000);
    putUint32(out, 16, 900);
    putUint16(out, 20, 2);
    putFloat32(out, 36, minCoefficient);
    putFloat32(out, 40, maxCoefficient);
    return out;
}
}  // namespace

TEST_CASE("KSPLATLoader reads the file header") {
    const auto header = KSPLATLoader::parseHeader(headerBytes(-2.0f, 2.0f));
    REQUIRE(header.versionMajor == 0);
    REQUIRE(header.versionMinor == 1);
    REQUIRE(header.maxSectionCount == 8);
    REQUIRE(header.sectionCount == 3);
    REQUIRE(header.maxSplatCount == 1000);
    REQUIRE(header.splatCount == 900);
    REQUIRE(header.compressionLevel == 2);
    REQUIRE(header.minHarmonicCoefficient == Approx(-2.0).epsilon(1e-6));
    REQUIRE(header.maxHarmonicCoefficient == Approx(2.0).epsilon(1e-6));
}

TEST_CASE("KSPLATLoader replaces a zero harmonic range with the default") {
    // A stored zero is falsy upstream, so the defaults take over.
    const auto header = KSPLATLoader::parseHeader(headerBytes(0.0f, 0.0f));
    REQUIRE(header.minHarmonicCoefficient == -1.5);
    REQUIRE(header.maxHarmonicCoefficient == 1.5);
}

TEST_CASE("KSPLATLoader rejects a short header") {
    const std::vector<std::uint8_t> tiny(10, 0);
    REQUIRE_THROWS_WITH(KSPLATLoader::parseHeader(tiny), "THREE.KSPLATLoader: Header is too short");
}

TEST_CASE("KSPLATLoader reads a section header") {
    std::vector<std::uint8_t> out(1024, 0);
    putUint32(out, 0, 500);
    putUint32(out, 4, 600);
    putUint32(out, 8, 64);
    putUint32(out, 12, 10);
    putFloat32(out, 16, 1.25f);
    putUint16(out, 20, 128);
    putUint32(out, 24, 16383);
    putUint32(out, 32, 9);
    putUint32(out, 36, 1);
    putUint16(out, 40, 1);

    const auto layout = KSPLATLoader::compression(1);
    const auto section = KSPLATLoader::parseSectionHeader(out, 0, layout);
    REQUIRE(section.splatCount == 500);
    REQUIRE(section.maxSplatCount == 600);
    REQUIRE(section.bucketSize == 64);
    REQUIRE(section.bucketCount == 10);
    REQUIRE(section.bucketBlockSize == Approx(1.25).epsilon(1e-6));
    REQUIRE(section.bucketStorageSizeBytes == 128);
    REQUIRE(section.compressionScaleRange == 16383);
    REQUIRE(section.fullBucketCount == 9);
    REQUIRE(section.partiallyFilledBucketCount == 1);
    REQUIRE(section.sphericalHarmonicsDegree == 1);
}

TEST_CASE("KSPLATLoader falls back to the level range when the section stores zero") {
    std::vector<std::uint8_t> out(1024, 0);
    // The scale range is left at zero, so the level supplies it.
    const auto section = KSPLATLoader::parseSectionHeader(out, 0, KSPLATLoader::compression(1));
    REQUIRE(section.compressionScaleRange == 32767);
}

TEST_CASE("KSPLATLoader reads a section at an offset") {
    std::vector<std::uint8_t> out(2048, 0);
    putUint32(out, 1024 + 0, 7);
    const auto section = KSPLATLoader::parseSectionHeader(out, 1024, KSPLATLoader::compression(0));
    REQUIRE(section.splatCount == 7);
}