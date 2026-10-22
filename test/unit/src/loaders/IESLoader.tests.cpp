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
TEST_CASE("IESLoader reads the three tables in file order") {
    // Two vertical angles, two horizontal, then one row of readings per horizontal angle.
    const std::vector<std::string> lines{"0 90", "0 45", "1 2", "3 4"};
    std::size_t cursor = 0;
    const auto tables = IESLoader::readTables(lines, cursor, 2, 2);
    REQUIRE(tables.verticalAngles.size() == 2);
    REQUIRE(tables.verticalAngles[1] == 90.0);
    REQUIRE(tables.horizontalAngles.size() == 2);
    REQUIRE(tables.horizontalAngles[1] == 45.0);
    // The outer index is the horizontal angle, so there are two rows of two.
    REQUIRE(tables.candela.size() == 2);
    REQUIRE(tables.candela[0].size() == 2);
    REQUIRE(tables.candela[0][0] == 1.0);
    REQUIRE(tables.candela[1][0] == 3.0);
    REQUIRE(tables.candela[1][1] == 4.0);
    REQUIRE(cursor == 4);
}

TEST_CASE("IESLoader scales the readings and shifts the horizontal angles") {
    std::vector<std::string> lines{"0 90", "10 55", "1 2", "3 4"};
    std::size_t cursor = 0;
    auto tables = IESLoader::readTables(lines, cursor, 2, 2);
    // The multiplier and both ballast factors multiply every reading.
    const double maximum = IESLoader::normalizeTables(tables, 2.0, 1.0, 1.0);
    REQUIRE(tables.candela[0][0] == 2.0);
    REQUIRE(tables.candela[1][1] == 8.0);
    // The largest scaled reading is reported.
    REQUIRE(maximum == 8.0);
    // The horizontal angles now start at zero and keep their spacing.
    REQUIRE(tables.horizontalAngles[0] == 0.0);
    REQUIRE(tables.horizontalAngles[1] == 45.0);
}

TEST_CASE("IESLoader applies all three factors") {
    std::vector<std::string> lines{"0", "0", "1"};
    std::size_t cursor = 0;
    auto tables = IESLoader::readTables(lines, cursor, 1, 1);
    // Four tenths times five tenths times one is a fifth.
    IESLoader::normalizeTables(tables, 1.0, 0.4, 0.5);
    REQUIRE(tables.candela[0][0] == Approx(0.2).epsilon(1e-12));
}

TEST_CASE("IESLoader refuses empty tables") {
    const std::vector<std::string> lines{"0"};
    std::size_t cursor = 0;
    // A zero count in either direction means there is nothing to read.
    REQUIRE(IESLoader::readTables(lines, cursor, 0, 3).candela.empty());
    REQUIRE(IESLoader::readTables(lines, cursor, 3, 0).candela.empty());
}