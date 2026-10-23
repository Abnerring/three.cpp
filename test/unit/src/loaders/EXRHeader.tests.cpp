#include "loaders/EXRLoader.h"

#include "catch.hpp"

#include <cstdint>
#include <string>
#include <vector>

TEST_CASE("EXRLoader recognises the file magic") {
    // Two million six thousand six hundred and thirty, little endian.
    REQUIRE(EXRLoader::looksLikeExr({0x76, 0x2F, 0x31, 0x01}));
    // Anything else is not an EXR file.
    REQUIRE_FALSE(EXRLoader::looksLikeExr({0x76, 0x2F, 0x31, 0x00}));
    REQUIRE_FALSE(EXRLoader::looksLikeExr({0x00, 0x00, 0x00, 0x00}));
    // Too short to hold the magic at all.
    REQUIRE_FALSE(EXRLoader::looksLikeExr({}));
    REQUIRE_FALSE(EXRLoader::looksLikeExr({0x76, 0x2F, 0x31}));
    // Extra bytes after the magic do not disturb the answer.
    REQUIRE(EXRLoader::looksLikeExr({0x76, 0x2F, 0x31, 0x01, 0xFF, 0xFF}));
}

TEST_CASE("EXRLoader reads the specification flags") {
    // Nothing set is a single part, short names, no tiles, no deep data.
    const auto none = EXRLoader::headerFlags(0);
    REQUIRE_FALSE(none.singleTile);
    REQUIRE_FALSE(none.longName);
    REQUIRE_FALSE(none.deepFormat);
    REQUIRE_FALSE(none.multiPart);
    // Each flag on its own.
    REQUIRE(EXRLoader::headerFlags(2).singleTile);
    REQUIRE(EXRLoader::headerFlags(4).longName);
    REQUIRE(EXRLoader::headerFlags(8).deepFormat);
    REQUIRE(EXRLoader::headerFlags(16).multiPart);
}

TEST_CASE("EXRLoader leaves the first bit alone") {
    // The lowest bit belongs to none of the four flags.
    const auto first = EXRLoader::headerFlags(1);
    REQUIRE_FALSE(first.singleTile);
    REQUIRE_FALSE(first.longName);
    REQUIRE_FALSE(first.deepFormat);
    REQUIRE_FALSE(first.multiPart);
    // Setting it alongside another changes only that other.
    const auto both = EXRLoader::headerFlags(1 | 4);
    REQUIRE(both.longName);
    REQUIRE_FALSE(both.singleTile);
}

TEST_CASE("EXRLoader reads several flags at once") {
    // Tiles and multiple parts together.
    const auto both = EXRLoader::headerFlags(2 | 16);
    REQUIRE(both.singleTile);
    REQUIRE(both.multiPart);
    REQUIRE_FALSE(both.longName);
    REQUIRE_FALSE(both.deepFormat);
}

TEST_CASE("EXRLoader reports the header failures") {
    REQUIRE(EXRLoader::notExrMessage() ==
            "THREE.EXRLoader: Provided file doesn't appear to be in OpenEXR format.");
    REQUIRE(EXRLoader::noPartsMessage() == "THREE.EXRLoader: No valid part headers found.");
    // The two are different, so neither test can pass by accident.
    REQUIRE(EXRLoader::notExrMessage() != EXRLoader::noPartsMessage());
}

TEST_CASE("EXRLoader starts the header after the pre-amble") {
    // Four bytes of magic and two of version and specification, padded to eight.
    REQUIRE(EXRLoader::kHeaderStart == 8);
}