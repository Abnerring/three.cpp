#include "catch.hpp"
#include "math/Color.h"
#include "math/ColorConverter.h"
#include "math/Vector3.h"
#include <cmath>

namespace {
bool close(double actual, double expected, double epsilon = 1e-12) {
    return std::abs(actual - expected) <= epsilon;
}
}

TEST_CASE("ColorConverter setHSV converts HSV to RGB", "[colorconverter]") {
    Color color;

    SECTION("the updated color is returned") {
        REQUIRE(&ColorConverter::setHSV(color, 0.0, 1.0, 1.0) == &color);
    }

    SECTION("full saturation at value 1 lands on the exact primaries") {
        ColorConverter::setHSV(color, 0.0, 1.0, 1.0);
        REQUIRE(color.r == 1.0);
        REQUIRE(color.g == 0.0);
        REQUIRE(color.b == 0.0);
    }

    SECTION("reference values of the pinned conversion") {
        ColorConverter::setHSV(color, 0.25, 0.5, 0.75);
        REQUIRE(color.r == 0.56250000000000011);
        REQUIRE(color.g == 0.75);
        REQUIRE(color.b == 0.375);
    }

    SECTION("the hue is wrapped by euclideanModulo") {
        Color wrapped, direct;
        ColorConverter::setHSV(wrapped, 1.25, 1.0, 1.0);
        ColorConverter::setHSV(direct, 0.25, 1.0, 1.0);
        REQUIRE(wrapped.r == direct.r);
        REQUIRE(wrapped.g == direct.g);
        REQUIRE(wrapped.b == direct.b);

        Color negative;
        ColorConverter::setHSV(negative, -0.75, 1.0, 1.0);
        ColorConverter::setHSV(direct, 0.25, 1.0, 1.0);
        REQUIRE(negative.r == direct.r);
        REQUIRE(negative.g == direct.g);
        REQUIRE(negative.b == direct.b);
    }

    SECTION("saturation and value are clamped to [0, 1]") {
        Color high, clamped;
        ColorConverter::setHSV(high, 0.0, 5.0, 1.0);
        ColorConverter::setHSV(clamped, 0.0, 1.0, 1.0);
        REQUIRE(high.r == clamped.r);
        REQUIRE(high.g == clamped.g);
        REQUIRE(high.b == clamped.b);

        // A clamped value of zero makes `l` zero, which forces the saturation term to zero.
        Color zeroValue, black;
        ColorConverter::setHSV(zeroValue, 0.0, 1.0, -2.0);
        ColorConverter::setHSV(black, 0.5, 0.0, 0.0);
        REQUIRE(zeroValue.r == 0.0);
        REQUIRE(zeroValue.g == 0.0);
        REQUIRE(zeroValue.b == 0.0);
        REQUIRE(zeroValue.r == black.r);
        REQUIRE(zeroValue.g == black.g);
        REQUIRE(zeroValue.b == black.b);
    }

    SECTION("fractional hue input keeps its exact double value") {
        Color color1, color2;
        ColorConverter::setHSV(color1, 0.1 + 0.2, 1.0, 1.0);
        ColorConverter::setHSV(color2, 0.30000000000000004, 1.0, 1.0);
        REQUIRE(color1.r == color2.r);
        REQUIRE(color1.g == color2.g);
        REQUIRE(color1.b == color2.b);
    }
}

TEST_CASE("ColorConverter getHSV reads the working space RGB values", "[colorconverter]") {
    Vector3 target;

    SECTION("the target is returned") {
        const Color color(1.0, 0.0, 0.0);
        REQUIRE(&ColorConverter::getHSV(color, target) == &target);
    }

    SECTION("red is exactly (0, 1, 1)") {
        ColorConverter::getHSV(Color(1.0, 0.0, 0.0), target);
        REQUIRE(target.x == 0.0);
        REQUIRE(target.y == 1.0);
        REQUIRE(target.z == 1.0);
    }

    SECTION("achromatic colors report hue 0, saturation 0 and v = l") {
        ColorConverter::getHSV(Color(0.5, 0.5, 0.5), target);
        REQUIRE(target.x == 0.0);
        REQUIRE(target.y == 0.0);
        REQUIRE(target.z == 0.5);

        ColorConverter::getHSV(Color(0.0, 0.0, 0.0), target);
        REQUIRE(target.x == 0.0);
        REQUIRE(target.y == 0.0);
        REQUIRE(target.z == 0.0);
    }

    SECTION("reference values of the pinned conversion") {
        ColorConverter::getHSV(Color(0.25, 0.5, 0.75), target);
        REQUIRE(target.x == 0.58333333333333337);
        REQUIRE(target.y == 0.66666666666666663);
        REQUIRE(target.z == 0.75);
    }

    SECTION("the module scratch record does not leak between calls") {
        Vector3 first, second, repeated;
        ColorConverter::getHSV(Color(1.0, 0.0, 0.0), first);
        ColorConverter::getHSV(Color(0.25, 0.5, 0.75), second);
        ColorConverter::getHSV(Color(1.0, 0.0, 0.0), repeated);
        REQUIRE(first.x == repeated.x);
        REQUIRE(first.y == repeated.y);
        REQUIRE(first.z == repeated.z);
        REQUIRE(first.x != second.x);
    }
}

TEST_CASE("ColorConverter HSV round trips and the Color HSL pair", "[colorconverter]") {
    SECTION("setHSV followed by getHSV returns the input triple") {
        const double hues[] = {0.0, 0.05, 0.25, 1.0 / 3.0, 0.5, 0.75, 0.999999};
        const double saturations[] = {0.0, 0.25, 0.5, 0.9, 1.0};
        const double values[] = {0.0, 0.125, 0.5, 0.875, 1.0};
        for (const double h : hues) {
            for (const double s : saturations) {
                for (const double v : values) {
                    Color color;
                    ColorConverter::setHSV(color, h, s, v);
                    Vector3 hsv;
                    ColorConverter::getHSV(color, hsv);
                    // The HSV round trip goes through HSL, which costs up to one ulp on the
                    // saturation upstream, so the recovered components carry that tolerance;
                    // test/unit/utils/parity/compare-addon-math.mjs pins the exact bits.
                    // Zero value and zero saturation also drop the hue (and a zero value the
                    // saturation), exactly like the source records do.
                    REQUIRE(close(hsv.z, v, 1e-15));
                    if (v > 0.0) REQUIRE(close(hsv.y, s, 1e-15));
                    if (v > 0.0 && s > 0.0) REQUIRE(close(hsv.x, h, 1e-15));
                }
            }
        }
    }

    SECTION("Color.getHSL of a primary is exact") {
        Vector3 hsl;
        Color(1.0, 0.0, 0.0).getHSL(hsl);
        REQUIRE(hsl.x == 0.0);
        REQUIRE(hsl.y == 1.0);
        REQUIRE(hsl.z == 0.5);
    }

    SECTION("Color.setHSL wraps the hue and returns the color") {
        Color color;
        REQUIRE(&color.setHSL(0.6, 1.0, 0.5) == &color);
        REQUIRE(color.r == 0.0);
        REQUIRE(color.g == 0.39999999999999925);
        REQUIRE(color.b == 1.0);

        Color wrapped, direct;
        wrapped.setHSL(1.6, 1.0, 0.5);
        direct.setHSL(0.6, 1.0, 0.5);
        REQUIRE(wrapped.r == direct.r);
        REQUIRE(wrapped.g == direct.g);
        REQUIRE(wrapped.b == direct.b);
    }

    SECTION("setHSL followed by getHSL returns the input triple") {
        for (double h = 0.0; h < 1.0; h += 0.125) {
            for (const double l : {0.0, 0.25, 0.5, 0.75, 1.0}) {
                Color color;
                color.setHSL(h, 1.0, l);
                Vector3 hsl;
                color.getHSL(hsl);
                // Lightness and saturation come back exactly; the hue can lose one ulp.
                // Black and white drop the hue and the saturation, like the source does.
                REQUIRE(hsl.z == l);
                if (l > 0.0 && l < 1.0) {
                    REQUIRE(hsl.y == 1.0);
                    REQUIRE(close(hsl.x, h, 1e-15));
                }
            }
        }
    }

    SECTION("the conversion pair agrees with the raw HSL record") {
        Color color;
        ColorConverter::setHSV(color, 0.25, 0.5, 0.75);
        Vector3 hsl;
        color.getHSL(hsl);
        REQUIRE(hsl.x == 0.24999999999999992);
        REQUIRE(hsl.y == 0.42857142857142855);
        REQUIRE(hsl.z == 0.5625);
    }
}
