#include "loaders/EXRLoader.h"

#include "catch.hpp"

#include <cstdint>
#include <vector>

TEST_CASE("EXRLoader steps over a part number only when there is one") {
    // A file with several parts writes four bytes before each chunk; a single part file does
    // not, so the first value the reader takes is the row itself.
    REQUIRE(EXRLoader::chunkPrefix(false) == 0);
    REQUIRE(EXRLoader::chunkPrefix(true) == 4);
    REQUIRE(EXRLoader::kPartPrefixSize == 4);
}

TEST_CASE("EXRLoader counts rows from the window's first row") {
    // A window starting at zero leaves the row as written.
    REQUIRE(EXRLoader::rowWithinWindow(0, 0) == 0);
    REQUIRE(EXRLoader::rowWithinWindow(7, 0) == 7);
    // A window starting below zero shifts the other way, so a row written as minus three in a
    // window starting at minus five is the second row of the window.
    REQUIRE(EXRLoader::rowWithinWindow(-3, -5) == 2);
    // And a window starting above zero subtracts, so row twelve in a window from ten is two.
    REQUIRE(EXRLoader::rowWithinWindow(12, 10) == 2);
}

TEST_CASE("EXRLoader takes the total from the last cumulative count") {
    // The counts accumulate, so the last one is the row's total.
    REQUIRE(EXRLoader::totalSamples({2, 5, 9}) == 9);
    REQUIRE(EXRLoader::totalSamples({0, 0, 7}) == 7);
    // A single entry is both the first count and the total.
    REQUIRE(EXRLoader::totalSamples({4}) == 4);
    // A row with no counts has no samples, which is what the loop checks before reading data.
    REQUIRE(EXRLoader::totalSamples({}) == 0);
    REQUIRE(EXRLoader::totalSamples({0, 0, 0}) == 0);
}

TEST_CASE("EXRLoader reads the cumulative counts the other way round") {
    // Lowest byte first: the four bytes of one in little order give one.
    REQUIRE(EXRLoader::cumulativeCount(1, 0, 0, 0) == 1);
    // Whereas the file's usual high first order would give a very large number from the same
    // bytes, which is the mistake this guards against.
    REQUIRE(EXRLoader::cumulativeCount(0, 0, 0, 1) == 0x01000000u);
    REQUIRE(EXRLoader::cumulativeCount(1, 0, 0, 0) != 0x01000000u);
    // A real count reads back as itself.
    REQUIRE(EXRLoader::cumulativeCount(0x78, 0x56, 0x34, 0x12) == 0x12345678u);
    // All zero stays zero whichever way it is read.
    REQUIRE(EXRLoader::cumulativeCount(0, 0, 0, 0) == 0);
}