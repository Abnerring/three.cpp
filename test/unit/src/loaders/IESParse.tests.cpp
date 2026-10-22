#include "loaders/IESLoader.h"

#include "catch.hpp"

#include <string>

namespace {
const char* kIes =
    "IESNA:LM-63-2002\n"
    "[TEST] sample\n"
    "TILT=NONE\n"
    "1 1000 1 2 2 1 1 0 0 0\n"
    "1 1 100\n"
    "0 90\n"
    "0 45\n"
    "1 2\n"
    "3 4\n";
}  // namespace

TEST_CASE("IESLoader parses a complete lamp") {
    const auto lamp = IESLoader::parse(kIes);
    REQUIRE(lamp.values.count == 1);
    REQUIRE(lamp.values.lumens == 1000.0);
    REQUIRE(lamp.values.numVerticalAngles == 2);
    REQUIRE(lamp.values.numHorizontalAngles == 2);
    // The factors scale the readings, so with all three at one they stay as written.
    REQUIRE(lamp.factors.inputWatts == 100.0);
    REQUIRE(lamp.tables.verticalAngles.size() == 2);
    REQUIRE(lamp.tables.candela.size() == 2);
    REQUIRE(lamp.tables.candela[1][0] == 3.0);
    // The horizontal angles start at zero already.
    REQUIRE(lamp.tables.horizontalAngles[0] == 0.0);
    REQUIRE(lamp.maximum == 4.0);
}

TEST_CASE("IESLoader records that the tilt was absent") {
    const auto lamp = IESLoader::parse(kIes);
    // The keyword said none, so no tilt block was read.
    REQUIRE_FALSE(lamp.hasTilt);
}

TEST_CASE("IESLoader reports a file without the tilt keyword") {
    REQUIRE_THROWS_WITH(IESLoader::parse("IESNA:LM-63-2002\n1 2 3\n"),
                        "THREE.IESLoader: TILT keyword not found");
}

TEST_CASE("IESLoader scales the parsed lamp by its factors") {
    // A multiplier of two doubles every reading.
    const std::string text =
        "IESNA:LM-63-2002\n"
        "TILT=NONE\n"
        "1 1000 2 2 2 1 1 0 0 0\n"
        "1 1 100\n"
        "0 90\n"
        "0 45\n"
        "1 2\n"
        "3 4\n";
    const auto lamp = IESLoader::parse(text);
    REQUIRE(lamp.tables.candela[0][0] == 2.0);
    REQUIRE(lamp.maximum == 8.0);
}