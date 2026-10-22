#include "loaders/VTKLoader.h"

#include "catch.hpp"

#include <cmath>
#include <string>
#include <vector>

TEST_CASE("VTKLoader splits a line into values") {
    // Runs of whitespace separate the tokens.
    const auto values = VTKLoader::parseFloats("1 2.5   -3\t4\n");
    REQUIRE(values.size() == 4);
    REQUIRE(values[0] == 1.0);
    REQUIRE(values[1] == 2.5);
    REQUIRE(values[2] == -3.0);
    REQUIRE(values[3] == 4.0);
}

TEST_CASE("VTKLoader reads a number prefix") {
    // The conversion takes as much of a token as it can, like the upstream one.
    const auto values = VTKLoader::parseFloats("1.5abc 2");
    REQUIRE(values.size() == 2);
    REQUIRE(values[0] == 1.5);
    REQUIRE(values[1] == 2.0);
}

TEST_CASE("VTKLoader reads a word as NaN") {
    // A token that does not begin with a number has no value at all.
    const auto values = VTKLoader::parseFloats("abc 3");
    REQUIRE(values.size() == 2);
    REQUIRE(std::isnan(values[0]));
    REQUIRE(values[1] == 3.0);
}

TEST_CASE("VTKLoader keeps an exponent with its number") {
    const auto values = VTKLoader::parseFloats("1e3 -2.5E-2");
    REQUIRE(values.size() == 2);
    REQUIRE(values[0] == 1000.0);
    REQUIRE(values[1] == Approx(-0.025).epsilon(1e-12));
}

TEST_CASE("VTKLoader measures the leading word") {
    // A keyword runs up to the first digit.
    REQUIRE(VTKLoader::wordLength("POINTS 3 float") == 6);
    // A line that begins with a digit has no word.
    REQUIRE(VTKLoader::wordLength("42") == 0);
    // A minus sign also ends a word.
    REQUIRE(VTKLoader::wordLength("abc -1") == 3);
    // A dot ends it too.
    REQUIRE(VTKLoader::wordLength("x.y") == 1);
}

TEST_CASE("VTKLoader returns nothing for an empty line") {
    REQUIRE(VTKLoader::parseFloats("").empty());
    REQUIRE(VTKLoader::parseFloats("   \t ").empty());
}
