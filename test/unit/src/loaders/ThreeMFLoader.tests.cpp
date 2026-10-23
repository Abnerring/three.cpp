#include "loaders/ThreeMFLoader.h"

#include "catch.hpp"

#include <string>

TEST_CASE("ThreeMFLoader names the fallback unit") {
    // The format's fallback is millimeters, named once as a constant.
    REQUIRE(std::string(ThreeMFLoader::kDefaultUnit) == "millimeter");
    // A document that says nothing gets it.
    REQUIRE(ThreeMFLoader::unitOf("") == "millimeter");
    // A document that names a unit keeps it as written, without any case folding.
    REQUIRE(ThreeMFLoader::unitOf("inch") == "inch");
    REQUIRE(ThreeMFLoader::unitOf("micron") == "micron");
    REQUIRE(ThreeMFLoader::unitOf("Meter") == "Meter");
    // The fallback is a real value, not a sentinel a caller has to translate.
    REQUIRE(ThreeMFLoader::unitOf("") != "");
}

TEST_CASE("ThreeMFLoader names the colour space") {
    // The colours a 3MF document carries are display referred, so they are read in the srgb
    // space rather than the linear one.
    REQUIRE(std::string(ThreeMFLoader::kColourSpace) == "srgb");
    // That is not the space the glTF factors use, which is the distinction worth recording.
    REQUIRE(std::string(ThreeMFLoader::kColourSpace) != "srgb-linear");
}
TEST_CASE("ThreeMFLoader records the unit but does not scale by it") {
    // The attribute survives as written, whatever it says.
    REQUIRE(ThreeMFLoader::unitOf("inch") == "inch");
    REQUIRE(ThreeMFLoader::unitOf("micron") == "micron");
    REQUIRE(ThreeMFLoader::unitOf("meter") == "meter");
    // And the loader applies none of them: the geometry keeps the numbers the document wrote.
    // This is a difference from the format, which expects the unit to be honoured.
    REQUIRE_FALSE(ThreeMFLoader::unitIsApplied());
    // So a document in inches and one in millimeters give the same numbers.
    REQUIRE(ThreeMFLoader::unitOf("inch") != ThreeMFLoader::unitOf("millimeter"));
}