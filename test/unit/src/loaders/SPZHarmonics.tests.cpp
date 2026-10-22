#include "loaders/SPZLoader.h"

#include "catch.hpp"

#include <cstdint>

TEST_CASE("SPZLoader counts the colour vectors per degree") {
    // The bands are one more vector than the previous, so the counts are the triangular sums
    // of three, five and seven.
    REQUIRE(SPZLoader::degreeToVectors(0) == 0);
    REQUIRE(SPZLoader::degreeToVectors(1) == 3);
    REQUIRE(SPZLoader::degreeToVectors(2) == 8);
    REQUIRE(SPZLoader::degreeToVectors(3) == 15);
    // A degree beyond the format falls back to the largest.
    REQUIRE(SPZLoader::degreeToVectors(9) == 15);
}

TEST_CASE("SPZLoader sizes one splat of harmonics") {
    // Three channels per vector.
    REQUIRE(SPZLoader::harmonicsSourceStride(0) == 0);
    REQUIRE(SPZLoader::harmonicsSourceStride(1) == 9);
    REQUIRE(SPZLoader::harmonicsSourceStride(2) == 24);
    REQUIRE(SPZLoader::harmonicsSourceStride(3) == 45);
}

TEST_CASE("SPZLoader reports a compressed container") {
    // The zstd frame magic is refused rather than decoded.
    REQUIRE_THROWS_WITH(SPZLoader::requireUncompressed(0x28, 0xb5),
                        "THREE.SPZLoader: Compressed SPZ is not supported");
    // Anything else passes.
    REQUIRE_NOTHROW(SPZLoader::requireUncompressed('P', 'K'));
    REQUIRE_NOTHROW(SPZLoader::requireUncompressed(0, 0));
}