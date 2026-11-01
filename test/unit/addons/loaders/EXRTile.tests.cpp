#include "loaders/EXRLoader.h"

#include "catch.hpp"

#include <cstdint>
#include <string>

TEST_CASE("EXRLoader reads chromaticities in pairs") {
    // Eight numbers, each colour's x followed by its y.
    const double numbers[8] = {1, 2, 3, 4, 5, 6, 7, 8};
    const auto points = EXRLoader::chromaticitiesFromNumbers(numbers);
    REQUIRE(points.redX == 1);
    REQUIRE(points.redY == 2);
    REQUIRE(points.greenX == 3);
    REQUIRE(points.greenY == 4);
    REQUIRE(points.blueX == 5);
    REQUIRE(points.blueY == 6);
    REQUIRE(points.whiteX == 7);
    REQUIRE(points.whiteY == 8);
    // The x of one colour is never the y of the one before it.
    REQUIRE(points.redY != points.greenX);
}

TEST_CASE("EXRLoader names the three level modes") {
    REQUIRE(EXRLoader::levelModeName(0) == "ONE_LEVEL");
    REQUIRE(EXRLoader::levelModeName(1) == "MIPMAP_LEVELS");
    REQUIRE(EXRLoader::levelModeName(2) == "RIPMAP_LEVELS");
    // A fourth code names nothing.
    REQUIRE(EXRLoader::levelModeName(3).empty());
    REQUIRE(EXRLoader::levelModeName(-1).empty());
}

TEST_CASE("EXRLoader names the two rounding modes") {
    REQUIRE(EXRLoader::roundingModeName(0) == "ROUND_DOWN");
    REQUIRE(EXRLoader::roundingModeName(1) == "ROUND_UP");
    // A third code names nothing.
    REQUIRE(EXRLoader::roundingModeName(2).empty());
    REQUIRE(EXRLoader::roundingModeName(-1).empty());
}

TEST_CASE("EXRLoader splits the tile mode byte") {
    // The low nibble is the level mode and the high one the rounding mode.
    const auto low = EXRLoader::tileDescription(64, 32, 0x01);
    REQUIRE(low.xSize == 64);
    REQUIRE(low.ySize == 32);
    REQUIRE(low.levelMode == "MIPMAP_LEVELS");
    REQUIRE(low.roundingMode == "ROUND_DOWN");

    const auto high = EXRLoader::tileDescription(64, 32, 0x10);
    REQUIRE(high.levelMode == "ONE_LEVEL");
    REQUIRE(high.roundingMode == "ROUND_UP");

    // Both at once.
    const auto both = EXRLoader::tileDescription(1, 2, 0x12);
    REQUIRE(both.levelMode == "RIPMAP_LEVELS");
    REQUIRE(both.roundingMode == "ROUND_UP");
    // The size fields are unaffected by the modes.
    REQUIRE(both.xSize == 1);
    REQUIRE(both.ySize == 2);
}