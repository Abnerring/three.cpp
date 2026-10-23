#include "loaders/GCodeLoader.h"

#include "catch.hpp"

#include <cstddef>
#include <string>

TEST_CASE("GCodeLoader names a layer without a separator") {
    // The number follows the word directly, so the first is zero and the tenth is ten beside it.
    REQUIRE(GCodeLoader::layerObjectName(0) == "layer0");
    REQUIRE(GCodeLoader::layerObjectName(1) == "layer1");
    REQUIRE(GCodeLoader::layerObjectName(9) == "layer9");
    REQUIRE(GCodeLoader::layerObjectName(10) == "layer10");
    REQUIRE(GCodeLoader::layerObjectName(100) == "layer100");
    // There is no underscore, which a caller searching for one would not find.
    REQUIRE(GCodeLoader::layerObjectName(1).find('_') == std::string::npos);
}

TEST_CASE("GCodeLoader names the group it builds") {
    // The format's own name, written in lower case.
    REQUIRE(GCodeLoader::rootObjectName() == "gcode");
    REQUIRE(GCodeLoader::rootObjectName() != "GCode");
}

TEST_CASE("GCodeLoader names the two materials after their lists") {
    // The extruded material and the path one carry the names of the lists they draw.
    REQUIRE(GCodeLoader::materialName(true) == "extruded");
    REQUIRE(GCodeLoader::materialName(false) == "path");
    // Those are the same two names the lists report, so a caller can match them up.
    REQUIRE(GCodeLoader::materialName(true) ==
            GCodeLoader::segmentListName(GCodeLoader::SegmentList::Extruded));
    REQUIRE(GCodeLoader::materialName(false) ==
            GCodeLoader::segmentListName(GCodeLoader::SegmentList::Path));
    // The two differ, so neither can stand in for the other.
    REQUIRE(GCodeLoader::materialName(true) != GCodeLoader::materialName(false));
}