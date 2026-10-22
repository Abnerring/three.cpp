#include "loaders/SVGLoader.h"

#include "catch.hpp"

#include <string>
#include <vector>

TEST_CASE("SVGLoader reads a run of numbers") {
    // Whitespace separates them.
    const auto spaced = SVGLoader::parseFloats("1 2 3");
    REQUIRE(spaced.size() == 3);
    REQUIRE(spaced[0] == 1.0);
    REQUIRE(spaced[2] == 3.0);
    // So does a comma.
    const auto commaed = SVGLoader::parseFloats("1,2,3");
    REQUIRE(commaed.size() == 3);
    // And so does nothing at all, as long as the signs make it unambiguous.
    // A sign after a number ends it, and the sign itself is consumed by the separator, so
    // the second number loses it.
    const auto tight = SVGLoader::parseFloats("1-2");
    REQUIRE(tight.size() == 2);
    REQUIRE(tight[0] == 1.0);
    REQUIRE(tight[1] == 2.0);
}

TEST_CASE("SVGLoader reads decimals and exponents") {
    const auto decimals = SVGLoader::parseFloats("1.5 2.25 .5");
    REQUIRE(decimals.size() == 3);
    REQUIRE(decimals[0] == 1.5);
    REQUIRE(decimals[1] == 2.25);
    REQUIRE(decimals[2] == 0.5);
    // A power of ten.
    const auto positive = SVGLoader::parseFloats("1e3");
    REQUIRE(positive.size() == 1);
    REQUIRE(positive[0] == 1000.0);
    // And a negative one.
    const auto negative = SVGLoader::parseFloats("1e-3");
    REQUIRE(negative.size() == 1);
    REQUIRE(negative[0] == Approx(0.001).epsilon(1e-12));
    // The exponent may be written with a capital letter.
    const auto capital = SVGLoader::parseFloats("2E2");
    REQUIRE(capital[0] == 200.0);
}

TEST_CASE("SVGLoader splits at a second decimal point") {
    // A second decimal point ends the number in progress, and is consumed by the
    // separator rather than opening the next one.
    const auto split = SVGLoader::parseFloats("1.5.5");
    REQUIRE(split.size() == 2);
    REQUIRE(split[0] == 1.5);
    REQUIRE(split[1] == 5.0);
}

TEST_CASE("SVGLoader rejects a malformed run") {
    // Two decimal points in a row cannot be read.
    REQUIRE_THROWS_AS(SVGLoader::parseFloats("1..2"), std::runtime_error);
    // Nor can two signs.
    REQUIRE_THROWS_AS(SVGLoader::parseFloats("-+1"), std::runtime_error);
    // Nor two commas with nothing between them.
    REQUIRE_THROWS_AS(SVGLoader::parseFloats("1,,2"), std::runtime_error);
    // The message names the character and its place.
    REQUIRE_THROWS_WITH(SVGLoader::parseFloats("1..2"),
                        "Unexpected character \".\" at index 2.");
}

TEST_CASE("SVGLoader reads an empty run as nothing") {
    REQUIRE(SVGLoader::parseFloats("").empty());
    REQUIRE(SVGLoader::parseFloats("   ").empty());
    REQUIRE(SVGLoader::parseFloats("1 2 ").size() == 2);
}

TEST_CASE("SVGLoader reads two flags written without separators") {
    // An arc carries seven numbers, of which the fourth and fifth are single digit flags. Here
    // the two flags are written as "11" with nothing between them.
    const auto arc = SVGLoader::parseFloats("5 5 0 11 10 20", {3, 4}, 7);
    // Three numbers, then two flags, then two more.
    REQUIRE(arc.size() == 7);
    REQUIRE(arc[0] == 5.0);
    REQUIRE(arc[1] == 5.0);
    REQUIRE(arc[2] == 0.0);
    // The flags came apart even though they were written together.
    REQUIRE(arc[3] == 1.0);
    REQUIRE(arc[4] == 1.0);
    REQUIRE(arc[5] == 10.0);
    REQUIRE(arc[6] == 20.0);
}

TEST_CASE("SVGLoader reads flags written with separators too") {
    // The same arc with a space between the flags reads the same way.
    const auto arc = SVGLoader::parseFloats("5 5 0 1 1 10 20", {3, 4}, 7);
    REQUIRE(arc.size() == 7);
    REQUIRE(arc[3] == 1.0);
    REQUIRE(arc[4] == 1.0);
    // A zero flag is read the same way as a one.
    const auto zero = SVGLoader::parseFloats("5 5 0 0 1 10 20", {3, 4}, 7);
    REQUIRE(zero[3] == 0.0);
    REQUIRE(zero[4] == 1.0);
}
