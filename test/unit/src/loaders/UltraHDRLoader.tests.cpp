#include "loaders/UltraHDRLoader.h"

#include "catch.hpp"

#include <cmath>

TEST_CASE("UltraHDRLoader builds the sRGB table") {
    // Index zero is the offset raised to the exponent.
    REQUIRE(UltraHDRLoader::srgbToLinear(0) == Approx(std::pow(0.0521327014, 2.4)).epsilon(1e-12));
    // Index one follows the same formula.
    REQUIRE(UltraHDRLoader::srgbToLinear(1) ==
            Approx(std::pow(0.003717127 + 0.0521327014, 2.4)).epsilon(1e-12));
    // The table rises with the index.
    REQUIRE(UltraHDRLoader::srgbToLinear(1023) > UltraHDRLoader::srgbToLinear(512));
    REQUIRE(UltraHDRLoader::srgbToLinear(512) > UltraHDRLoader::srgbToLinear(0));
    // Black is near zero and white is above one, since the table overshoots for the top values.
    REQUIRE(UltraHDRLoader::srgbToLinear(0) < 0.01);
    REQUIRE(UltraHDRLoader::srgbToLinear(1023) > 1.0);
}

TEST_CASE("UltraHDRLoader clamps a table index") {
    // Past the end the last entry repeats, and below zero the first.
    REQUIRE(UltraHDRLoader::srgbToLinear(5000) == UltraHDRLoader::srgbToLinear(1023));
    REQUIRE(UltraHDRLoader::srgbToLinear(-3) == UltraHDRLoader::srgbToLinear(0));
}

TEST_CASE("UltraHDRLoader interpolates the gain boost") {
    // The endpoints of the map land on the declared boosts.
    REQUIRE(UltraHDRLoader::gainMapBoost(0.0, 1.0, true, -1.0, 2.0) == -1.0);
    REQUIRE(UltraHDRLoader::gainMapBoost(1.0, 1.0, true, -1.0, 2.0) == 2.0);
    // The middle lands halfway between.
    REQUIRE(UltraHDRLoader::gainMapBoost(0.5, 1.0, true, -1.0, 2.0) == Approx(0.5).epsilon(1e-12));
}

TEST_CASE("UltraHDRLoader raises the map value by the recovery exponent") {
    // A gamma of one passes the value through, any other raises it.
    REQUIRE(UltraHDRLoader::gainMapBoost(0.25, 2.0, true, 0.0, 1.0) == Approx(0.25).epsilon(1e-12));
    REQUIRE(UltraHDRLoader::gainMapBoost(0.25, 2.0, false, 0.0, 1.0) == Approx(0.0625).epsilon(1e-12));
}