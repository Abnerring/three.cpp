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
// Two uncompressed rows of forty four bytes with distinct first centres.
std::vector<std::uint8_t> twoRows() {
    std::vector<std::uint8_t> out(88, 0);
    putFloat32(out, 0, 1.0f);
    putFloat32(out, 44, 2.0f);
    return out;
}
}  // namespace

TEST_CASE("KSPLATLoader decodes every row of a section") {
    auto buffer = twoRows();
    KSPLATLoader::SectionHeader section;
    section.splatCount = 2;
    section.maxSplatCount = 2;
    // No buckets, so the rows start at the section base.
    section.bucketCount = 0;
    const auto rows = KSPLATLoader::decodeSection(buffer, section, KSPLATLoader::compression(0), 0);
    REQUIRE(rows.size() == 2);
    REQUIRE(rows[0].center[0] == 1.0);
    REQUIRE(rows[1].center[0] == 2.0);
}

TEST_CASE("KSPLATLoader decodes a section placed after the header") {
    std::vector<std::uint8_t> buffer(100 + 88, 0);
    putFloat32(buffer, 100, 7.0f);
    KSPLATLoader::SectionHeader section;
    section.splatCount = 1;
    section.maxSplatCount = 1;
    section.bucketCount = 0;
    const auto rows = KSPLATLoader::decodeSection(buffer, section, KSPLATLoader::compression(0), 100);
    REQUIRE(rows.size() == 1);
    REQUIRE(rows[0].center[0] == 7.0);
}

TEST_CASE("KSPLATLoader adds the bucket coordinate to a compressed section") {
    // One bucket of sixteen bytes for three floats plus padding, then one row of twenty four.
    std::vector<std::uint8_t> buffer(16 + 24, 0);
    putFloat32(buffer, 0, 5.0f);
    putFloat32(buffer, 4, 6.0f);
    putFloat32(buffer, 8, 7.0f);
    // The row's centre shorts sit at the middle of the range, so they add nothing.
    const std::size_t row = 16;
    buffer[row + 0] = 0xff;
    buffer[row + 1] = 0x7f;
    buffer[row + 2] = 0xff;
    buffer[row + 3] = 0x7f;
    buffer[row + 4] = 0xff;
    buffer[row + 5] = 0x7f;

    KSPLATLoader::SectionHeader section;
    section.splatCount = 1;
    section.maxSplatCount = 1;
    section.bucketCount = 1;
    // One full bucket of one splat, so the first splat divides straight into bucket zero.
    section.fullBucketCount = 1;
    section.bucketSize = 1;
    section.bucketStorageSizeBytes = 16;
    section.bucketBlockSize = 4.0;
    section.compressionScaleRange = 32767;

    const auto rows = KSPLATLoader::decodeSection(buffer, section, KSPLATLoader::compression(1), 0);
    REQUIRE(rows.size() == 1);
    // The bucket coordinate is the only contribution at the middle of the range.
    REQUIRE(rows[0].center[0] == Approx(5.0).epsilon(1e-9));
    REQUIRE(rows[0].center[2] == Approx(7.0).epsilon(1e-9));
}

TEST_CASE("KSPLATLoader returns nothing for an empty section") {
    const std::vector<std::uint8_t> buffer(10, 0);
    KSPLATLoader::SectionHeader section;
    section.splatCount = 0;
    const auto rows = KSPLATLoader::decodeSection(buffer, section, KSPLATLoader::compression(0), 0);
    REQUIRE(rows.empty());
}
