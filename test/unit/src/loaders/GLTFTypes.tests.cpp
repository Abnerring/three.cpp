#include "loaders/GLTFLoader.h"

#include "catch.hpp"

#include <cstdint>
#include <string>

TEST_CASE("GLTFLoader names each component type") {
    // The six the format allows.
    REQUIRE(GLTFLoader::componentTypeName(5120) == "int8");
    REQUIRE(GLTFLoader::componentTypeName(5121) == "uint8");
    REQUIRE(GLTFLoader::componentTypeName(5122) == "int16");
    REQUIRE(GLTFLoader::componentTypeName(5123) == "uint16");
    REQUIRE(GLTFLoader::componentTypeName(5125) == "uint32");
    REQUIRE(GLTFLoader::componentTypeName(5126) == "float32");
    // Anything else is refused.
    REQUIRE_THROWS_AS(GLTFLoader::componentTypeName(5124), std::runtime_error);
}

TEST_CASE("GLTFLoader gives the width of each component type") {
    // The width in bytes.
    REQUIRE(GLTFLoader::componentTypeSize(5120) == 1);
    REQUIRE(GLTFLoader::componentTypeSize(5121) == 1);
    REQUIRE(GLTFLoader::componentTypeSize(5122) == 2);
    REQUIRE(GLTFLoader::componentTypeSize(5123) == 2);
    REQUIRE(GLTFLoader::componentTypeSize(5125) == 4);
    REQUIRE(GLTFLoader::componentTypeSize(5126) == 4);
    REQUIRE_THROWS_AS(GLTFLoader::componentTypeSize(0), std::runtime_error);
}

TEST_CASE("GLTFLoader gives the size of each accessor type") {
    // A vector holds as many components as it has axes.
    REQUIRE(GLTFLoader::typeSize("SCALAR") == 1);
    REQUIRE(GLTFLoader::typeSize("VEC2") == 2);
    REQUIRE(GLTFLoader::typeSize("VEC3") == 3);
    REQUIRE(GLTFLoader::typeSize("VEC4") == 4);
    // A matrix holds rows times columns.
    REQUIRE(GLTFLoader::typeSize("MAT2") == 4);
    REQUIRE(GLTFLoader::typeSize("MAT3") == 9);
    REQUIRE(GLTFLoader::typeSize("MAT4") == 16);
    // An unknown shape is refused.
    REQUIRE_THROWS_AS(GLTFLoader::typeSize("VEC5"), std::runtime_error);
}

TEST_CASE("GLTFLoader maps a semantic to an attribute name") {
    // The ten named explicitly, including the ones the port spells differently.
    REQUIRE(GLTFLoader::attributeName("POSITION") == "position");
    REQUIRE(GLTFLoader::attributeName("NORMAL") == "normal");
    REQUIRE(GLTFLoader::attributeName("TANGENT") == "tangent");
    REQUIRE(GLTFLoader::attributeName("TEXCOORD_0") == "uv");
    REQUIRE(GLTFLoader::attributeName("TEXCOORD_1") == "uv1");
    REQUIRE(GLTFLoader::attributeName("TEXCOORD_2") == "uv2");
    REQUIRE(GLTFLoader::attributeName("TEXCOORD_3") == "uv3");
    REQUIRE(GLTFLoader::attributeName("COLOR_0") == "color");
    REQUIRE(GLTFLoader::attributeName("WEIGHTS_0") == "skinWeight");
    REQUIRE(GLTFLoader::attributeName("JOINTS_0") == "skinIndex");
}

TEST_CASE("GLTFLoader falls back to a lower cased semantic") {
    // A semantic not listed is its own name lower cased.
    REQUIRE(GLTFLoader::attributeName("TEXCOORD_4") == "texcoord_4");
    REQUIRE(GLTFLoader::attributeName("_CUSTOM") == "_custom");
    // Something already lower case is unchanged.
    REQUIRE(GLTFLoader::attributeName("custom") == "custom");
}

TEST_CASE("GLTFLoader names each filter") {
    REQUIRE(GLTFLoader::filterName(9728) == "NearestFilter");
    REQUIRE(GLTFLoader::filterName(9729) == "LinearFilter");
    REQUIRE(GLTFLoader::filterName(9984) == "NearestMipmapNearestFilter");
    REQUIRE(GLTFLoader::filterName(9985) == "LinearMipmapNearestFilter");
    REQUIRE(GLTFLoader::filterName(9986) == "NearestMipmapLinearFilter");
    REQUIRE(GLTFLoader::filterName(9987) == "LinearMipmapLinearFilter");
    REQUIRE_THROWS_AS(GLTFLoader::filterName(0), std::runtime_error);
}

TEST_CASE("GLTFLoader names each wrapping") {
    REQUIRE(GLTFLoader::wrappingName(33071) == "ClampToEdgeWrapping");
    REQUIRE(GLTFLoader::wrappingName(33648) == "MirroredRepeatWrapping");
    REQUIRE(GLTFLoader::wrappingName(10497) == "RepeatWrapping");
    REQUIRE_THROWS_AS(GLTFLoader::wrappingName(0), std::runtime_error);
}