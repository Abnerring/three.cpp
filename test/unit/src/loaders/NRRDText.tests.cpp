#include "loaders/NRRDLoader.h"

#include "catch.hpp"

#include <string>

TEST_CASE("NRRDLoader reads whitespace separated numbers") {
    const auto values = NRRDLoader::parseTextData("1 2 3 4", {2, 2}, true, false);
    REQUIRE(values.size() == 4);
    REQUIRE(values[0] == 1.0);
    REQUIRE(values[3] == 4.0);
}

TEST_CASE("NRRDLoader treats every whitespace character as a separator") {
    // Tab, newline and carriage return all separate, and consecutive separators are harmless.
    const auto values = NRRDLoader::parseTextData("1\t2\n3\r4  5", {5}, true, false);
    REQUIRE(values.size() == 5);
    REQUIRE(values[1] == 2.0);
    REQUIRE(values[4] == 5.0);
}

TEST_CASE("NRRDLoader truncates fractions for an integer type") {
    // The integer path uses parseInt, which stops at the decimal point.
    const auto values = NRRDLoader::parseTextData("1.9 2.1", {2}, false, false);
    REQUIRE(values.size() == 2);
    REQUIRE(values[0] == 1.0);
    REQUIRE(values[1] == 2.0);
}

TEST_CASE("NRRDLoader keeps fractions for a floating type") {
    const auto values = NRRDLoader::parseTextData("1.5 2.5", {2}, true, false);
    REQUIRE(values[0] == 1.5);
    REQUIRE(values[1] == 2.5);
}

TEST_CASE("NRRDLoader reads hex when the encoding says so") {
    const auto values = NRRDLoader::parseTextData("ff 10", {2}, false, true);
    REQUIRE(values.size() == 2);
    REQUIRE(values[0] == 255.0);
    REQUIRE(values[1] == 16.0);
}

TEST_CASE("NRRDLoader stops at the expected count") {
    // Four extra numbers arrive but only three voxels were declared.
    const auto values = NRRDLoader::parseTextData("1 2 3 4 5 6 7", {3}, true, false);
    REQUIRE(values.size() == 3);
}
