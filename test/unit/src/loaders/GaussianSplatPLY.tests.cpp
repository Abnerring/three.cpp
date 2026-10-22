#include "loaders/GaussianSplatPLYLoader.h"

#include "catch.hpp"

#include <string>

namespace {
// Builds a header with the given number of f_rest properties.
std::string makeHeader(int restCount, bool withOpacity = true) {
    std::string text = "ply\nformat binary_little_endian 1.0\nelement vertex 1\n";
    text += "property float x\nproperty float y\nproperty float z\n";
    text += "property float scale_0\nproperty float scale_1\nproperty float scale_2\n";
    text += "property float rot_0\nproperty float rot_1\nproperty float rot_2\nproperty float rot_3\n";
    text += "property float f_dc_0\nproperty float f_dc_1\nproperty float f_dc_2\n";
    if (withOpacity) text += "property float opacity\n";
    for (int i = 0; i < restCount; ++i) text += "property float f_rest_" + std::to_string(i) + "\n";
    text += "end_header\n";
    return text;
}
}  // namespace

TEST_CASE("GaussianSplatPLYLoader works out the degree from the rest count") {
    // Nine, twenty four and forty five map to degrees one, two and three.
    REQUIRE(GaussianSplatPLYLoader::scanHeader(makeHeader(0)).degree == 0);
    REQUIRE(GaussianSplatPLYLoader::scanHeader(makeHeader(9)).degree == 1);
    REQUIRE(GaussianSplatPLYLoader::scanHeader(makeHeader(24)).degree == 2);
    REQUIRE(GaussianSplatPLYLoader::scanHeader(makeHeader(45)).degree == 3);
}

TEST_CASE("GaussianSplatPLYLoader counts the rest properties") {
    const auto header = GaussianSplatPLYLoader::scanHeader(makeHeader(9));
    REQUIRE(header.isPly);
    REQUIRE(header.restComponents == 9);
    // The count that matches no degree leaves it at zero.
    REQUIRE(GaussianSplatPLYLoader::scanHeader(makeHeader(5)).degree == 0);
    REQUIRE(GaussianSplatPLYLoader::scanHeader(makeHeader(5)).restComponents == 5);
}

TEST_CASE("GaussianSplatPLYLoader reports the missing required properties") {
    const auto full = GaussianSplatPLYLoader::scanHeader(makeHeader(0));
    REQUIRE(full.missing.empty());
    // Without an opacity property that one name is reported.
    const auto partial = GaussianSplatPLYLoader::scanHeader(makeHeader(0, false));
    REQUIRE(partial.missing.size() == 1);
    REQUIRE(partial.missing[0] == "opacity");
}

TEST_CASE("GaussianSplatPLYLoader collects the property names") {
    const auto header = GaussianSplatPLYLoader::scanHeader(makeHeader(9));
    // The three position names are the first three.
    REQUIRE(header.properties.size() > 3);
    REQUIRE(header.properties[0] == "x");
    REQUIRE(header.properties[2] == "z");
    // The rest properties are at the end.
    REQUIRE(header.properties.back() == "f_rest_8");
}

TEST_CASE("GaussianSplatPLYLoader rejects text that is not a header") {
    // Without the magic word nothing is read.
    REQUIRE_FALSE(GaussianSplatPLYLoader::scanHeader("not a ply\n").isPly);
    // Without the end marker nothing is read either.
    REQUIRE_FALSE(GaussianSplatPLYLoader::scanHeader("ply\nelement vertex 1\n").isPly);
}