#include "loaders/IESLoader.h"

#include "catch.hpp"

#include <string>
#include <vector>

TEST_CASE("IESLoader splits numbers from several dialects") {
    // Whitespace separated.
    const auto spaced = IESLoader::textToNumbers("1 2 3");
    REQUIRE(spaced.size() == 3);
    REQUIRE(spaced[2] == 3.0);
    // Commas count as separators and runs of spaces collapse.
    const auto comma = IESLoader::textToNumbers("1, 2,   3");
    REQUIRE(comma.size() == 3);
    REQUIRE(comma[1] == 2.0);
    // Surrounding whitespace is trimmed.
    const auto padded = IESLoader::textToNumbers("   4 5  ");
    REQUIRE(padded.size() == 2);
    REQUIRE(padded[0] == 4.0);
    // An empty line has no numbers.
    REQUIRE(IESLoader::textToNumbers("   ").empty());
}

TEST_CASE("IESLoader reads a table spread over lines") {
    const std::vector<std::string> lines{"0 1", "2 3", "4 5"};
    std::size_t cursor = 0;
    const auto numbers = IESLoader::readNumbers(lines, cursor, 4);
    // Two lines hold the four values, so the cursor sits on the third.
    REQUIRE(numbers.size() == 4);
    REQUIRE(numbers[3] == 3.0);
    REQUIRE(cursor == 2);
}

TEST_CASE("IESLoader keeps only the requested count") {
    const std::vector<std::string> lines{"0 1 2 3 4"};
    std::size_t cursor = 0;
    const auto numbers = IESLoader::readNumbers(lines, cursor, 2);
    REQUIRE(numbers.size() == 2);
    REQUIRE(numbers[1] == 1.0);
}

TEST_CASE("IESLoader reads a tilt block") {
    const std::vector<std::string> lines{"1.0", "3", "0 90 180", "1.0 0.5 0.25"};
    std::size_t cursor = 0;
    const auto tilt = IESLoader::readTilt(lines, cursor);
    REQUIRE(tilt.present);
    REQUIRE(tilt.lampToLuminaireGeometry == 1.0);
    REQUIRE(tilt.numAngles == 3);
    REQUIRE(tilt.angles.size() == 3);
    REQUIRE(tilt.angles[2] == 180.0);
    // The multipliers are the same length as the angles.
    REQUIRE(tilt.multipliers.size() == 3);
    REQUIRE(tilt.multipliers[2] == 0.25);
    REQUIRE(cursor == 4);
}

TEST_CASE("IESLoader handles a tilt block with no angles") {
    const std::vector<std::string> lines{"0.0", "0"};
    std::size_t cursor = 0;
    const auto tilt = IESLoader::readTilt(lines, cursor);
    // Without angles the block is not marked present.
    REQUIRE_FALSE(tilt.present);
    REQUIRE(tilt.numAngles == 0);
}

TEST_CASE("IESLoader splits lines and strips carriage returns") {
    const auto lines = IESLoader::splitLines("a\r\nb\nc");
    REQUIRE(lines.size() == 3);
    REQUIRE(lines[0] == "a");
    REQUIRE(lines[2] == "c");
}