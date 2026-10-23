#include "loaders/EXRLoader.h"

#include "catch.hpp"

TEST_CASE("EXRLoader rounds a bit count up to whole bytes") {
    // Nothing takes no bytes; a partial byte still takes one.
    REQUIRE(EXRLoader::bitByteCount(0) == 0);
    REQUIRE(EXRLoader::bitByteCount(1) == 1);
    REQUIRE(EXRLoader::bitByteCount(7) == 1);
    REQUIRE(EXRLoader::bitByteCount(8) == 1);
    REQUIRE(EXRLoader::bitByteCount(9) == 2);
    REQUIRE(EXRLoader::bitByteCount(16) == 2);
    REQUIRE(EXRLoader::bitByteCount(17) == 3);
}

TEST_CASE("EXRLoader indexes the table with the top fourteen bits") {
    // With exactly fourteen live the whole word is the index.
    REQUIRE(EXRLoader::decodeTableIndex(0x00FF, 14) == 0x00FF);
    // With sixteen live the top two are dropped by the shift.
    REQUIRE(EXRLoader::decodeTableIndex(0x1234, 16) == 0x048D);
    // With thirty two live only the top fourteen survive.
    // The word holds twenty eight bits, so eighteen dropped leaves ten of them.
    REQUIRE(EXRLoader::decodeTableIndex(0x0FFFFFFF, 32) == 0x3FF);
}

TEST_CASE("EXRLoader masks the sign out of the index") {
    // All ones as a signed word is minus one, and shifting it keeps it so; the mask is what
    // turns that into the largest index rather than a negative one.
    REQUIRE(EXRLoader::decodeTableIndex(-1, 32) == EXRLoader::kHufDecodeMask);
    REQUIRE(EXRLoader::decodeTableIndex(-1, 14) == EXRLoader::kHufDecodeMask);
    // A word whose top bit is set shifts in ones above it, and the mask keeps those.
    REQUIRE(EXRLoader::decodeTableIndex(static_cast<std::int32_t>(0x80000000u), 32) ==
            0x2000);
}

TEST_CASE("EXRLoader answers zero before the table is reachable") {
    // Fewer bits live than the first level needs means no lookup yet.
    REQUIRE(EXRLoader::decodeTableIndex(0xFF, 0) == 0);
    REQUIRE(EXRLoader::decodeTableIndex(0xFF, 13) == 0);
    // At the threshold it starts answering.
    REQUIRE(EXRLoader::decodeTableIndex(0xFF, 14) == 0xFF);
}

TEST_CASE("EXRLoader keeps every index inside the table") {
    // Whatever the word and however many bits are live, the index lands within the first level.
    for (int live = 14; live <= 32; ++live) {
        for (const std::int32_t word : {0, 1, 0x00FF, 0x1234, 0x0FFFFFFF, -1}) {
            const int index = EXRLoader::decodeTableIndex(word, live);
            REQUIRE(index >= 0);
            REQUIRE(index <= EXRLoader::kHufDecodeMask);
        }
    }
}
