#include "loaders/LDrawLoader.h"

#include "catch.hpp"

#include <string>

TEST_CASE("LDrawLoader normalises a colour token") {
    // The zero x form becomes the hash form.
    REQUIRE(LDrawLoader::normalizeColour("0x1A2B3C", 1) == "#1A2B3C");
    // The hash form is left alone.
    REQUIRE(LDrawLoader::normalizeColour("#1A2B3C", 1) == "#1A2B3C");
    // Lower case letters survive.
    REQUIRE(LDrawLoader::normalizeColour("0xabcdef", 1) == "#abcdef");
}

TEST_CASE("LDrawLoader rejects a token that is not a colour") {
    // A bare number is not a colour, and the message names the line.
    REQUIRE_THROWS_WITH(LDrawLoader::normalizeColour("16", 7),
                        "THREE.LDrawLoader: Invalid color while parsing material at line 7.");
    // A line with no number says nothing about one.
    REQUIRE_THROWS_WITH(LDrawLoader::normalizeColour("", -1),
                        "THREE.LDrawLoader: Invalid color while parsing material.");
    // A word is not a colour either.
    REQUIRE_THROWS_WITH(LDrawLoader::normalizeColour("red", 2),
                        "THREE.LDrawLoader: Invalid color while parsing material at line 2.");
}

TEST_CASE("LDrawLoader turns an alpha byte into a fraction") {
    // The ends of the range are exact.
    REQUIRE(LDrawLoader::alphaFromByte(255) == 1.0);
    REQUIRE(LDrawLoader::alphaFromByte(0) == 0.0);
    // The middle is divided by the range.
    REQUIRE(LDrawLoader::alphaFromByte(128) == Approx(128.0 / 255.0).epsilon(1e-12));
    // Outside the range both ends saturate rather than wrapping.
    REQUIRE(LDrawLoader::alphaFromByte(300) == 1.0);
    REQUIRE(LDrawLoader::alphaFromByte(-5) == 0.0);
}

TEST_CASE("LDrawLoader only calls an alpha below one transparent") {
    // Fully opaque is not transparent.
    REQUIRE_FALSE(LDrawLoader::isTransparentAlpha(1.0));
    // Anything below is.
    REQUIRE(LDrawLoader::isTransparentAlpha(254.0 / 255.0));
    REQUIRE(LDrawLoader::isTransparentAlpha(0.0));
    // The boundary is what the clamped byte gives at its top.
    REQUIRE_FALSE(LDrawLoader::isTransparentAlpha(LDrawLoader::alphaFromByte(255)));
}
