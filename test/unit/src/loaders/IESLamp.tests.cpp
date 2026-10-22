#include "loaders/IESLoader.h"

#include "catch.hpp"

#include <string>
#include <vector>

TEST_CASE("IESLoader reads the ten lamp values") {
    // All ten on one line, in the order the format writes them.
    const std::vector<std::string> lines{"1 1000 0.5 5 7 2 1 10 20 30"};
    std::size_t cursor = 0;
    const auto values = IESLoader::readLampValues(lines, cursor);
    REQUIRE(values.count == 1);
    REQUIRE(values.lumens == 1000.0);
    REQUIRE(values.multiplier == 0.5);
    REQUIRE(values.numVerticalAngles == 5);
    REQUIRE(values.numHorizontalAngles == 7);
    REQUIRE(values.gonioType == 2);
    REQUIRE(values.units == 1);
    REQUIRE(values.width == 10.0);
    REQUIRE(values.length == 20.0);
    REQUIRE(values.height == 30.0);
}

TEST_CASE("IESLoader reads lamp values spread over lines") {
    // The same ten numbers split across three lines, which the format allows.
    const std::vector<std::string> lines{"1 1000", "0.5 5 7 2", "1 10 20 30"};
    std::size_t cursor = 0;
    const auto values = IESLoader::readLampValues(lines, cursor);
    REQUIRE(values.count == 1);
    REQUIRE(values.multiplier == 0.5);
    REQUIRE(values.height == 30.0);
    // Three lines were consumed.
    REQUIRE(cursor == 3);
}

TEST_CASE("IESLoader leaves the values at zero when the block is short") {
    const std::vector<std::string> lines{"1 1000"};
    std::size_t cursor = 0;
    const auto values = IESLoader::readLampValues(lines, cursor);
    // Fewer than ten numbers means the block is not read at all.
    REQUIRE(values.count == 0);
    REQUIRE(values.lumens == 0.0);
}

TEST_CASE("IESLoader reads the three lamp factors") {
    const std::vector<std::string> lines{"1.1 0.9 60"};
    std::size_t cursor = 0;
    const auto factors = IESLoader::readLampFactors(lines, cursor);
    REQUIRE(factors.ballast == Approx(1.1).epsilon(1e-12));
    REQUIRE(factors.ballastLightOutput == Approx(0.9).epsilon(1e-12));
    REQUIRE(factors.inputWatts == 60.0);
}

TEST_CASE("IESLoader leaves the factors at zero when short") {
    const std::vector<std::string> lines{"1.1"};
    std::size_t cursor = 0;
    const auto factors = IESLoader::readLampFactors(lines, cursor);
    REQUIRE(factors.ballast == 0.0);
    REQUIRE(factors.inputWatts == 0.0);
}