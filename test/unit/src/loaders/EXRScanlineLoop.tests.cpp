#include "loaders/EXRLoader.h"

#include "catch.hpp"

#include <cstdint>

TEST_CASE("EXRLoader counts the blocks with the remainder dropped") {
    // A height that divides evenly gives exactly that many blocks.
    REQUIRE(EXRLoader::scanlineBlockCount(16, 16) == 1);
    REQUIRE(EXRLoader::scanlineBlockCount(32, 16) == 2);
    REQUIRE(EXRLoader::scanlineBlockCount(64, 32) == 2);
    // A height that does not divide leaves the remainder unread, which is a difference from
    // what the format intends rather than a rounding rule.
    REQUIRE(EXRLoader::scanlineBlockCount(17, 16) == 1);
    REQUIRE(EXRLoader::scanlineBlockCount(31, 16) == 1);
    REQUIRE(EXRLoader::scanlineBlockCount(15, 16) == 0);
    // A block height of zero would divide by nothing.
    REQUIRE(EXRLoader::scanlineBlockCount(16, 0) == 0);
}

TEST_CASE("EXRLoader shortens the last block to fit") {
    // A block that would run past the bottom stops at it.
    REQUIRE(EXRLoader::scanlineBlockLines(0, 16, 16) == 16);
    REQUIRE(EXRLoader::scanlineBlockLines(0, 20, 16) == 16);
    REQUIRE(EXRLoader::scanlineBlockLines(16, 20, 16) == 4);
    // A block beginning at or past the bottom holds nothing.
    REQUIRE(EXRLoader::scanlineBlockLines(20, 20, 16) == 0);
    REQUIRE(EXRLoader::scanlineBlockLines(25, 20, 16) == 0);
}

TEST_CASE("EXRLoader decides compression from the two sizes") {
    // Four lines of ten bytes are forty bytes of pixels. A block smaller than that was
    // compressed; one the same size or larger was stored plainly.
    REQUIRE(EXRLoader::scanlineBlockCompressed(39, 4, 10));
    REQUIRE_FALSE(EXRLoader::scanlineBlockCompressed(40, 4, 10));
    REQUIRE_FALSE(EXRLoader::scanlineBlockCompressed(41, 4, 10));
    // The decision does not look at the compression's name at all, which is the point: a file
    // whose compression made a block no smaller is read plainly even though it named one.
    REQUIRE_FALSE(EXRLoader::scanlineBlockCompressed(100, 1, 10));
}

TEST_CASE("EXRLoader mirrors a row into the output") {
    // Four rows: the file's first is the output's last and the other way about.
    REQUIRE(EXRLoader::scanlineOutputRow(0, 4) == 3);
    REQUIRE(EXRLoader::scanlineOutputRow(1, 4) == 2);
    REQUIRE(EXRLoader::scanlineOutputRow(2, 4) == 1);
    REQUIRE(EXRLoader::scanlineOutputRow(3, 4) == 0);
    // A single row is its own mirror.
    REQUIRE(EXRLoader::scanlineOutputRow(0, 1) == 0);
    // A row past the bottom has nowhere to go.
    REQUIRE(EXRLoader::scanlineOutputRow(4, 4) == 0);
}