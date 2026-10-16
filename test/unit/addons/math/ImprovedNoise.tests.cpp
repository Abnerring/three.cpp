#include "catch.hpp"
#include "math/ImprovedNoise.h"
#include <cmath>
#include <limits>

TEST_CASE("ImprovedNoise noise reproduces the pinned reference values", "[improvednoise]") {
    ImprovedNoise noise;

    SECTION("values of the fractional lattice") {
        REQUIRE(noise.noise(0.1, 0.2, 0.3) == 0.35122924878110723);
        REQUIRE(noise.noise(0.5, 0.5, 0.5) == -0.25);
        REQUIRE(noise.noise(0.25, 0.5, 0.75) == -0.26971530914306641);
        REQUIRE(noise.noise(-1.25, 2.75, -3.5) == 0.40050268173217773);
        REQUIRE(noise.noise(255.75, 0.5, 1.5) == 0.2025146484375);
    }

    SECTION("every integer lattice point is exactly zero") {
        // At integer coordinates the three fade weights are zero and the first gradient of
        // each lerp() pair is evaluated at the undisplaced coordinate.
        for (int x = -3; x <= 3; ++x)
            for (int y = -3; y <= 3; ++y)
                for (int z = -3; z <= 3; ++z) REQUIRE(noise.noise(x, y, z) == 0.0);
    }

    SECTION("noise stays inside [-1, 1]") {
        for (int i = 0; i <= 20; ++i) {
            for (int j = 0; j <= 20; ++j) {
                const double value = noise.noise(i * 0.37 - 4.1, j * 0.53 - 5.2, i * j * 0.071 - 2.0);
                REQUIRE(value >= -1.0);
                REQUIRE(value <= 1.0);
            }
        }
    }
}

TEST_CASE("ImprovedNoise applies the upstream lattice reduction", "[improvednoise]") {
    ImprovedNoise noise;

    SECTION("the table repeats every 256 units") {
        REQUIRE(noise.noise(0.5, 0.5, 0.5) == noise.noise(256.5, 0.5, 0.5));
        REQUIRE(noise.noise(0.5, 0.5, 0.5) == noise.noise(0.5, 512.5, 0.5));
        REQUIRE(noise.noise(3.25, 1.5, 0.75) == noise.noise(3.25 + 256 * 5, 1.5, 0.75 - 256));
    }

    SECTION("negative floors wrap into the same table entries as their positive shift") {
        REQUIRE(noise.noise(-0.25, 0.5, 1.5) == noise.noise(255.75, 0.5, 1.5));
        REQUIRE(noise.noise(0.25, -0.5, -1.5) == noise.noise(0.25, 255.5, 254.5));
        REQUIRE(noise.noise(-1.0, -1.0, -1.0) == 0.0);
    }

    SECTION("`floorX & 255` is a ToInt32 conversion, not a truncation") {
        // The fractional part of 1e300 is exactly zero, and ToInt32( 1e300 ) is 0, so this
        // call equals the one at the origin of the same fractional offset.
        REQUIRE(noise.noise(1e300, 0.5, 0.5) == noise.noise(0.0, 0.5, 0.5));
        REQUIRE(noise.noise(-1e300, 0.5, 0.5) == noise.noise(-0.0, 0.5, 0.5));
        REQUIRE(noise.noise(4294967296.5, 0.5, 0.5) == noise.noise(0.5, 0.5, 0.5));
    }

    SECTION("non-finite coordinates produce NaN like the source arithmetic") {
        REQUIRE(std::isnan(noise.noise(std::numeric_limits<double>::quiet_NaN(), 0.0, 0.0)));
        REQUIRE(std::isnan(noise.noise(0.0, std::numeric_limits<double>::infinity(), 0.0)));
        REQUIRE(std::isnan(noise.noise(0.0, -std::numeric_limits<double>::infinity(), 0.5)));
    }
}
