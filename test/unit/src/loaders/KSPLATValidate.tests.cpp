#include "loaders/KSPLATLoader.h"

#include "catch.hpp"

#include <cstdint>
#include <vector>

namespace {
KSPLATLoader::Header goodHeader() {
    KSPLATLoader::Header header;
    header.versionMajor = 0;
    header.versionMinor = 1;
    header.maxSectionCount = 2;
    header.splatCount = 100;
    header.compressionLevel = 1;
    return header;
}
}  // namespace

TEST_CASE("KSPLATLoader accepts a consistent file") {
    // Two section headers need four kilobytes plus two thousand and forty eight.
    const std::vector<std::uint8_t> buffer(4096 + 2 * 1024, 0);
    REQUIRE_NOTHROW(KSPLATLoader::validateFile(buffer, goodHeader()));
}

TEST_CASE("KSPLATLoader rejects a file shorter than its header") {
    const std::vector<std::uint8_t> buffer(100, 0);
    REQUIRE_THROWS_WITH(KSPLATLoader::validateFile(buffer, goodHeader()),
                        "THREE.KSPLATLoader: Invalid KSPLAT header.");
}

TEST_CASE("KSPLATLoader rejects an unsupported version") {
    const std::vector<std::uint8_t> buffer(4096 + 2 * 1024, 0);
    auto header = goodHeader();
    header.versionMajor = 1;
    REQUIRE_THROWS_WITH(KSPLATLoader::validateFile(buffer, header),
                        "THREE.KSPLATLoader: Unsupported KSPLAT version 1.1.");
    // A minor older than the first release is refused too.
    header = goodHeader();
    header.versionMinor = 0;
    REQUIRE_THROWS_WITH(KSPLATLoader::validateFile(buffer, header),
                        "THREE.KSPLATLoader: Unsupported KSPLAT version 0.0.");
}

TEST_CASE("KSPLATLoader rejects an unknown compression level") {
    const std::vector<std::uint8_t> buffer(4096 + 2 * 1024, 0);
    auto header = goodHeader();
    header.compressionLevel = 3;
    REQUIRE_THROWS_WITH(KSPLATLoader::validateFile(buffer, header),
                        "THREE.KSPLATLoader: Unsupported KSPLAT compression level 3.");
}

TEST_CASE("KSPLATLoader rejects too many splats") {
    const std::vector<std::uint8_t> buffer(4096 + 2 * 1024, 0);
    auto header = goodHeader();
    header.splatCount = 10000001;
    REQUIRE_THROWS_WITH(
        KSPLATLoader::validateFile(buffer, header),
        "THREE.KSPLATLoader: KSPLAT file contains too many splats (10000001).");
}

TEST_CASE("KSPLATLoader rejects a file without room for its section headers") {
    // The header claims three sections but the file ends right after the file header.
    const std::vector<std::uint8_t> buffer(4096, 0);
    auto header = goodHeader();
    header.maxSectionCount = 3;
    REQUIRE_THROWS_WITH(KSPLATLoader::validateFile(buffer, header),
                        "THREE.KSPLATLoader: Invalid KSPLAT section headers.");
}

TEST_CASE("KSPLATLoader sizes a section") {
    KSPLATLoader::SectionHeader section;
    section.maxSplatCount = 100;
    section.bucketCount = 10;
    section.bucketStorageSizeBytes = 128;
    section.partiallyFilledBucketCount = 1;
    // Degree zero, so a splat is just the four fixed fields.
    const auto layout = KSPLATLoader::sectionLayout(section, KSPLATLoader::compression(0));
    REQUIRE(layout.bytesPerSplat == 44);
    // One partial length of four bytes.
    REQUIRE(layout.bucketsMetaDataSize == 4);
    // Ten buckets of one hundred and twenty eight bytes plus the lengths.
    REQUIRE(layout.bucketsStorageSize == 1280 + 4);
    // A hundred splats of forty four bytes.
    REQUIRE(layout.splatDataSize == 4400);
    REQUIRE(layout.storageSize == 1280 + 4 + 4400);
}

TEST_CASE("KSPLATLoader rejects an unsupported harmonic degree") {
    KSPLATLoader::SectionHeader section;
    section.sphericalHarmonicsDegree = 4;
    REQUIRE_THROWS_WITH(KSPLATLoader::sectionLayout(section, KSPLATLoader::compression(0)),
                        "THREE.KSPLATLoader: Unsupported KSPLAT spherical harmonics degree 4.");
}
