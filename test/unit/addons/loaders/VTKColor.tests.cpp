#include "loaders/VTKLoader.h"

#include "catch.hpp"

#include <string>

TEST_CASE("VTKLoader converts colours and copies normals") {
    // A mid grey and a white, each three values.
    const auto colors = VTKLoader::parseColors("0.5 0.5 0.5 1 1 1");
    REQUIRE(colors.size() == 6);
    // The middle of the display range lands near a quarter of the working range.
    REQUIRE(colors[0] == Approx(0.2140).margin(1e-3));
    // White stays one.
    REQUIRE(colors[3] == Approx(1.0).margin(1e-4));

    // Normals come through untouched.
    const auto normals = VTKLoader::parseNormals("0 0 1 -1 0 0");
    REQUIRE(normals.size() == 6);
    REQUIRE(normals[0] == 0.0);
    REQUIRE(normals[2] == 1.0);
    REQUIRE(normals[3] == -1.0);
}

TEST_CASE("VTKLoader drops a partial triple") {
    // Only whole triples count, so a trailing pair produces nothing.
    REQUIRE(VTKLoader::parseColors("0.5 0.5").empty());
    REQUIRE(VTKLoader::parseNormals("1 2").empty());
}

TEST_CASE("VTKLoader darkens the low end of the colour range") {
    // A small display value converts to a much smaller working one, which is why the
    // conversion cannot simply be skipped.
    const auto colors = VTKLoader::parseColors("0.04 0 0");
    REQUIRE(colors.size() == 3);
    REQUIRE(colors[0] < 0.005);
    REQUIRE(colors[0] > 0.0);
    // Pure black stays zero.
    const auto black = VTKLoader::parseColors("0 0 0");
    REQUIRE(black[0] == 0.0);
}
