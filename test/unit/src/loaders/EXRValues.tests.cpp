#include "loaders/EXRLoader.h"

#include "catch.hpp"

#include <cstdint>
#include <string>
#include <utility>
#include <vector>

TEST_CASE("EXRLoader names the ten compressions") {
    const std::vector<std::pair<std::uint8_t, std::string>> table{
        {0, "NO_COMPRESSION"},   {1, "RLE_COMPRESSION"},  {2, "ZIPS_COMPRESSION"},
        {3, "ZIP_COMPRESSION"},  {4, "PIZ_COMPRESSION"},  {5, "PXR24_COMPRESSION"},
        {6, "B44_COMPRESSION"},  {7, "B44A_COMPRESSION"}, {8, "DWAA_COMPRESSION"},
        {9, "DWAB_COMPRESSION"},
    };
    for (const auto& row : table) {
        REQUIRE(EXRLoader::compressionName(row.first) == row.second);
    }
    // A code past the table names nothing rather than reading past it.
    REQUIRE(EXRLoader::compressionName(10).empty());
    REQUIRE(EXRLoader::compressionName(255).empty());
}

TEST_CASE("EXRLoader names the three line orders") {
    REQUIRE(EXRLoader::lineOrderName(0) == "INCREASING_Y");
    REQUIRE(EXRLoader::lineOrderName(1) == "DECREASING_Y");
    REQUIRE(EXRLoader::lineOrderName(2) == "RANDOM_Y");
    // A fourth code names nothing.
    REQUIRE(EXRLoader::lineOrderName(3).empty());
}

TEST_CASE("EXRLoader names the two environment maps") {
    REQUIRE(EXRLoader::envmapName(0) == "ENVMAP_LATLONG");
    REQUIRE(EXRLoader::envmapName(1) == "ENVMAP_CUBE");
    // A third code names nothing.
    REQUIRE(EXRLoader::envmapName(2).empty());
}

TEST_CASE("EXRLoader reads a box in axis order") {
    // The four numbers are the minima of the two axes and then their maxima, not one corner and
    // then the other.
    const auto box = EXRLoader::boxFromNumbers(1, 2, 3, 4);
    REQUIRE(box.xMin == 1);
    REQUIRE(box.yMin == 2);
    REQUIRE(box.xMax == 3);
    REQUIRE(box.yMax == 4);
    // Negative numbers are kept as they are, since the numbers are signed.
    const auto negative = EXRLoader::boxFromNumbers(-10, -20, 10, 20);
    REQUIRE(negative.xMin == -10);
    REQUIRE(negative.yMin == -20);
    REQUIRE(negative.xMax == 10);
    REQUIRE(negative.yMax == 20);
    // A box of one pixel has each maximum one above its minimum.
    const auto single = EXRLoader::boxFromNumbers(0, 0, 0, 0);
    REQUIRE(single.xMax == single.xMin);
    REQUIRE(single.yMax == single.yMin);
}