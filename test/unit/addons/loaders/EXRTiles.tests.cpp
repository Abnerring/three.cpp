#include "loaders/EXRLoader.h"

#include "catch.hpp"

#include <cstdint>

TEST_CASE("EXRLoader cuts a tile at the right edge") {
    // A block wholly inside the image keeps its width.
    REQUIRE(EXRLoader::tileColumns(0, 64, 128) == 64);
    REQUIRE(EXRLoader::tileColumns(64, 64, 128) == 64);
    // One reaching past the edge stops there.
    REQUIRE(EXRLoader::tileColumns(64, 64, 100) == 36);
    REQUIRE(EXRLoader::tileColumns(0, 64, 50) == 50);
    // A block beginning at or past the edge holds nothing.
    REQUIRE(EXRLoader::tileColumns(128, 64, 128) == 0);
    REQUIRE(EXRLoader::tileColumns(200, 64, 128) == 0);
}

TEST_CASE("EXRLoader cuts a tile at the bottom edge") {
    // The same rule on the other side.
    REQUIRE(EXRLoader::tileLines(0, 64, 128) == 64);
    REQUIRE(EXRLoader::tileLines(64, 64, 100) == 36);
    REQUIRE(EXRLoader::tileLines(128, 64, 128) == 0);
    // A single row past the edge likewise.
    REQUIRE(EXRLoader::tileLines(128, 1, 128) == 0);
}

TEST_CASE("EXRLoader mirrors a tile row after adding the block's own") {
    // A block beginning at sixty four, four rows tall, in an image a hundred and twenty eight
    // rows tall: its first row is the output's sixty third and its last the sixtieth.
    REQUIRE(EXRLoader::tileOutputRow(64, 0, 128) == 63);
    REQUIRE(EXRLoader::tileOutputRow(64, 1, 128) == 62);
    REQUIRE(EXRLoader::tileOutputRow(64, 3, 128) == 60);
    // The block at the very bottom of the file lands at the very top of the output.
    REQUIRE(EXRLoader::tileOutputRow(124, 3, 128) == 0);
    // The block at the top of the file lands at the bottom of the output.
    REQUIRE(EXRLoader::tileOutputRow(0, 0, 128) == 127);
}

TEST_CASE("EXRLoader agrees with the scanline mirror when the block starts at the top") {
    // With no block offset the two rules are the same one, which shows they were meant to
    // agree rather than being two different conventions.
    for (std::uint32_t row = 0; row < 8; ++row) {
        REQUIRE(EXRLoader::tileOutputRow(0, row, 8) == EXRLoader::scanlineOutputRow(row, 8));
    }
}