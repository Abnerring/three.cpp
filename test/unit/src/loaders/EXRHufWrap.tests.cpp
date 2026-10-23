#include "loaders/EXRLoader.h"

#include "catch.hpp"

#include <cstdint>
#include <string>

TEST_CASE("EXRLoader bounds the symbol range") {
    // Both ends inside the table are usable.
    REQUIRE(EXRLoader::hufRangeValid(0, 0));
    REQUIRE(EXRLoader::hufRangeValid(0, 100));
    // An end at or past the size is not.
    REQUIRE_FALSE(EXRLoader::hufRangeValid(65536, 0));
    REQUIRE_FALSE(EXRLoader::hufRangeValid(0, 65536));
    REQUIRE_FALSE(EXRLoader::hufRangeValid(70000, 70000));
    // The largest usable index is one below the size.
    REQUIRE(EXRLoader::hufRangeValid(65535, 65535));
}

TEST_CASE("EXRLoader bounds the bits against the bytes left") {
    // Eight bits fit in one byte, and no more do.
    REQUIRE(EXRLoader::hufBitsFit(8, 1));
    REQUIRE_FALSE(EXRLoader::hufBitsFit(9, 1));
    // Nothing wanted fits in nothing left.
    REQUIRE(EXRLoader::hufBitsFit(0, 0));
    REQUIRE_FALSE(EXRLoader::hufBitsFit(1, 0));
    // A larger run fits when there is room for it.
    REQUIRE(EXRLoader::hufBitsFit(80, 10));
    REQUIRE(EXRLoader::hufBitsFit(0xFFFFFFFFu, 0x20000000u));
}

TEST_CASE("EXRLoader reports the two wrapper failures") {
    REQUIRE(EXRLoader::hufRangeMessage() ==
            "THREE.EXRLoader: Something wrong with HUF_ENCSIZE");
    REQUIRE(EXRLoader::hufBitsMessage() ==
            "THREE.EXRLoader: Something wrong with hufUncompress");
    // The two are different messages, so a test cannot pass by getting either.
    REQUIRE(EXRLoader::hufRangeMessage() != EXRLoader::hufBitsMessage());
}