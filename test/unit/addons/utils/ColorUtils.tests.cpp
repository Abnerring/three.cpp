#include "utils/ColorUtils.h"

#include "math/Color.h"

#include "catch.hpp"

TEST_CASE("ColorUtils setKelvin follows the black body branches") {
    Color color;

    // 1000 K is the lower clamp: the red channel saturates, the blue channel is zero.
    ColorUtils::setKelvin(color, 1000);
    REQUIRE(color.r == Approx(1.0));
    REQUIRE(color.g > 0.0);
    REQUIRE(color.g < 1.0);
    REQUIRE(color.b == Approx(0.0));

    // Below the clamp the result is identical to the clamp value.
    const auto atMinimum = color;
    ColorUtils::setKelvin(color, 100);
    REQUIRE(color.r == Approx(atMinimum.r));
    REQUIRE(color.g == Approx(atMinimum.g));
    REQUIRE(color.b == Approx(atMinimum.b));

    // 40000 K is the upper clamp: the blue channel saturates.
    ColorUtils::setKelvin(color, 40000);
    REQUIRE(color.b == Approx(1.0));
    REQUIRE(color.r < 1.0);
    REQUIRE(color.g < 1.0);
    const auto atMaximum = color;
    ColorUtils::setKelvin(color, 100000);
    REQUIRE(color.r == Approx(atMaximum.r));
    REQUIRE(color.g == Approx(atMaximum.g));
    REQUIRE(color.b == Approx(atMaximum.b));
}

TEST_CASE("ColorUtils setKelvin warms towards red at low temperatures") {
    Color warm;
    Color neutral;
    Color cool;
    ColorUtils::setKelvin(warm, 2000);
    ColorUtils::setKelvin(neutral, 6500);
    ColorUtils::setKelvin(cool, 20000);

    // Red stays high while blue grows with the temperature.
    REQUIRE(warm.b < neutral.b);
    REQUIRE(neutral.b < cool.b);
    REQUIRE(warm.r >= neutral.r);
    REQUIRE(neutral.r >= cool.r);
}
