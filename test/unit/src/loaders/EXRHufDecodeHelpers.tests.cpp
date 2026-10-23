#include "loaders/EXRLoader.h"

#include "catch.hpp"

#include <cstdint>
#include <cstddef>

TEST_CASE("EXRLoader pads the last byte of a block") {
    // A block whose bit count is a multiple of eight carries no padding.
    REQUIRE(EXRLoader::hufPadBits(8) == 0);
    REQUIRE(EXRLoader::hufPadBits(16) == 0);
    // One bit short of a byte leaves one bit of padding, and so on.
    REQUIRE(EXRLoader::hufPadBits(7) == 1);
    REQUIRE(EXRLoader::hufPadBits(1) == 7);
    // The mask is taken, so a bit count of zero pads a whole byte rather than nothing.
    REQUIRE(EXRLoader::hufPadBits(0) == 0);
}

TEST_CASE("EXRLoader moves the held bits up to the table's width") {
    // Thirteen bits held, with the top one set: moving it up by one puts it at the top of the
    // table's fourteen.
    REQUIRE(EXRLoader::hufFlushIndex(0x1000, 13) == 0x2000);
    // All ones held stays at the top and is masked to the table's width.
    REQUIRE(EXRLoader::hufFlushIndex(0x3FF, 10) == 0x3FF << 4);
    // Nothing held is nothing.
    REQUIRE(EXRLoader::hufFlushIndex(0, 14) == 0);
    // The result never leaves the table.
    REQUIRE(EXRLoader::hufFlushIndex(0xFFFFFFFFu, 1) <= 0x3FFF);
}

TEST_CASE("EXRLoader compares only a candidate's own length") {
    // A code of three bits taken from the top of five held ones: the top three of 0b10101 are
    // 0b101, so a code of five matches and a code of two does not.
    REQUIRE(EXRLoader::hufCandidateMatches(0b101, 3, 0b10101, 5));
    REQUIRE_FALSE(EXRLoader::hufCandidateMatches(0b010, 3, 0b10101, 5));
    // Fewer bits held than the code needs matches nothing.
    REQUIRE_FALSE(EXRLoader::hufCandidateMatches(0b101, 3, 0b101, 2));
    // A length of zero matches nothing.
    REQUIRE_FALSE(EXRLoader::hufCandidateMatches(0, 0, 0b101, 3));
}

TEST_CASE("EXRLoader names the decode error") {
    REQUIRE(EXRLoader::hufDecodeMessage() == "THREE.EXRLoader: hufDecode issues");
}