#include "loaders/EXRLoader.h"

#include "catch.hpp"

TEST_CASE("EXRLoader sizes the decode table") {
    // Fourteen bits, so sixteen thousand three hundred and eighty four entries.
    REQUIRE(EXRLoader::kHufDecodeBits == 14);
    REQUIRE(EXRLoader::kHufDecodeSize == 16384);
    REQUIRE(EXRLoader::kHufDecodeMask == 16383);
    REQUIRE(EXRLoader::kHufDecodeSize == EXRLoader::kHufDecodeMask + 1);
}

TEST_CASE("EXRLoader decides when a second level is needed") {
    // A code as long as the table is read from it directly.
    REQUIRE_FALSE(EXRLoader::hufUsesSubTable(1));
    REQUIRE_FALSE(EXRLoader::hufUsesSubTable(14));
    // One bit longer and it needs the second level.
    REQUIRE(EXRLoader::hufUsesSubTable(15));
    REQUIRE(EXRLoader::hufUsesSubTable(58));
}

TEST_CASE("EXRLoader indexes a short code by its whole value") {
    // A code no longer than the table is its own index.
    REQUIRE(EXRLoader::hufSubTableIndex(0, 1) == 0);
    REQUIRE(EXRLoader::hufSubTableIndex(5, 3) == 5);
    REQUIRE(EXRLoader::hufSubTableIndex(16383, 14) == 16383);
}

TEST_CASE("EXRLoader indexes a long code by its leading bits") {
    // A sixteen bit code keeps the two bits above the table, which always land inside it.
    REQUIRE(EXRLoader::hufSubTableIndex(0, 16) == 0);
    REQUIRE(EXRLoader::hufSubTableIndex(4, 16) == 1);
    REQUIRE(EXRLoader::hufSubTableIndex(0xFFFF, 16) == 16383);
    // A fifteen bit code keeps one bit.
    REQUIRE(EXRLoader::hufSubTableIndex(0x7FFF, 15) == 16383);
    REQUIRE(EXRLoader::hufSubTableIndex(0x4000, 15) == 8192);
}

TEST_CASE("EXRLoader keeps every long index inside the table") {
    // However long the code, the index stays within the first level.
    for (int length = 15; length <= EXRLoader::kHufMaxCodeLength; ++length) {
        const int top = (1 << length) - 1;
        const int index = EXRLoader::hufSubTableIndex(top, length);
        REQUIRE(index >= 0);
        REQUIRE(index <= EXRLoader::kHufDecodeMask);
    }
}
