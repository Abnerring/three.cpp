#include "loaders/PLYLoader.h"

#include "catch.hpp"

#include <string>

TEST_CASE("PLYLoader maps both type spellings to one attribute class") {
    REQUIRE(PLYLoader::bufferAttributeClass("char") == "Int8BufferAttribute");
    REQUIRE(PLYLoader::bufferAttributeClass("int8") == "Int8BufferAttribute");
    REQUIRE(PLYLoader::bufferAttributeClass("uchar") == "Uint8BufferAttribute");
    REQUIRE(PLYLoader::bufferAttributeClass("short") == "Int16BufferAttribute");
    REQUIRE(PLYLoader::bufferAttributeClass("ushort") == "Uint16BufferAttribute");
    REQUIRE(PLYLoader::bufferAttributeClass("int") == "Int32BufferAttribute");
    REQUIRE(PLYLoader::bufferAttributeClass("uint") == "Uint32BufferAttribute");
    REQUIRE(PLYLoader::bufferAttributeClass("float") == "Float32BufferAttribute");
    REQUIRE(PLYLoader::bufferAttributeClass("double") == "Float64BufferAttribute");
    // An unknown spelling has no class, as upstream returns undefined.
    REQUIRE(PLYLoader::bufferAttributeClass("nonsense").empty());
}

TEST_CASE("PLYLoader scales colours by the storage width") {
    REQUIRE(PLYLoader::colorScale("uchar") == 1.0 / 255.0);
    REQUIRE(PLYLoader::colorScale("uint8") == 1.0 / 255.0);
    REQUIRE(PLYLoader::colorScale("ushort") == 1.0 / 65535.0);
    REQUIRE(PLYLoader::colorScale("uint16") == 1.0 / 65535.0);
    REQUIRE(PLYLoader::colorScale("float") == 1.0);
    REQUIRE(PLYLoader::colorScale("double") == 1.0);
    // Anything else behaves like a byte.
    REQUIRE(PLYLoader::colorScale("int") == 1.0 / 255.0);
}

TEST_CASE("PLYLoader parses ascii tokens by type") {
    // Integer spellings truncate, as parseInt does.
    REQUIRE(PLYLoader::parseASCIINumber("42", "int") == 42.0);
    REQUIRE(PLYLoader::parseASCIINumber("3.7", "uchar") == 3.0);
    // Floating spellings keep the fraction.
    REQUIRE(PLYLoader::parseASCIINumber("3.7", "float") == Approx(3.7).epsilon(1e-12));
    REQUIRE(PLYLoader::parseASCIINumber("-2.5", "double") == Approx(-2.5).epsilon(1e-12));
    REQUIRE(PLYLoader::isIntegerType("uint16"));
    REQUIRE_FALSE(PLYLoader::isIntegerType("float64"));
}
