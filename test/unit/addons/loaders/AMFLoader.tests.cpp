#include "loaders/AMFLoader.h"

#include "catch.hpp"

#include <cstdint>
#include <string>
#include <vector>

TEST_CASE("AMFLoader detects a zip container") {
    REQUIRE(AMFLoader::isZip({0x50, 0x4b, 0x03, 0x04}));
    // The same bytes later in the buffer are not a signature.
    REQUIRE_FALSE(AMFLoader::isZip({'<', '?', 'x', 'm'}));
    // A buffer too short to hold the signature is not one either.
    REQUIRE_FALSE(AMFLoader::isZip({0x50}));
    REQUIRE_FALSE(AMFLoader::isZip({}));
}

TEST_CASE("AMFLoader converts the declared unit to millimetres") {
    REQUIRE(AMFLoader::unitScale("millimeter") == 1.0);
    REQUIRE(AMFLoader::unitScale("inch") == 25.4);
    REQUIRE(AMFLoader::unitScale("feet") == 304.8);
    REQUIRE(AMFLoader::unitScale("meter") == 1000.0);
    REQUIRE(AMFLoader::unitScale("micron") == 0.001);
}

TEST_CASE("AMFLoader matches the unit without regard to case") {
    REQUIRE(AMFLoader::unitScale("Inch") == 25.4);
    REQUIRE(AMFLoader::unitScale("METER") == 1000.0);
}

TEST_CASE("AMFLoader treats an unknown unit as a millimetre") {
    // Upstream starts from a scale of one and only overwrites it for a known name.
    REQUIRE(AMFLoader::unitScale("furlong") == 1.0);
    REQUIRE(AMFLoader::unitScale("") == 1.0);
}
