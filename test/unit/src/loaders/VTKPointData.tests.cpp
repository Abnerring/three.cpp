#include "loaders/VTKLoader.h"

#include "catch.hpp"

#include <string>

TEST_CASE("VTKLoader reads colours from a point data section") {
    const std::string text =
        "DATASET POLYDATA\n"
        "POINTS 2 float\n"
        "0 0 0  1 1 1\n"
        "POINT_DATA 2\n"
        "COLOR_SCALARS Colors 3\n"
        "0.5 0.5 0.5\n"
        "1 1 1\n";
    const auto mesh = VTKLoader::parseAscii(text);
    REQUIRE(mesh.positions.size() == 6);
    // Two colours, each converted from the display range.
    REQUIRE(mesh.colors.size() == 6);
    REQUIRE(mesh.colors[0] == Approx(0.2140).margin(1e-3));
    REQUIRE(mesh.colors[3] == Approx(1.0).margin(1e-4));
    REQUIRE(mesh.normals.empty());
}

TEST_CASE("VTKLoader reads normals from a point data section") {
    const std::string text =
        "DATASET POLYDATA\n"
        "POINTS 1 float\n"
        "0 0 0\n"
        "POINT_DATA 1\n"
        "NORMALS Normals float\n"
        "0 0 1\n";
    const auto mesh = VTKLoader::parseAscii(text);
    REQUIRE(mesh.normals.size() == 3);
    REQUIRE(mesh.normals[2] == 1.0);
    REQUIRE(mesh.colors.empty());
}

TEST_CASE("VTKLoader prefers colours when both flags linger") {
    // A normals keyword followed by a colours keyword leaves the colours flag set, and the
    // colour branch is tried first.
    const std::string text =
        "DATASET POLYDATA\n"
        "POINTS 1 float\n"
        "0 0 0\n"
        "POINT_DATA 1\n"
        "NORMALS Normals float\n"
        "0 0 1\n"
        "COLOR_SCALARS Colors 3\n"
        "1 1 1\n";
    const auto mesh = VTKLoader::parseAscii(text);
    // The normal was read while its flag was set, and the colour afterwards.
    REQUIRE(mesh.normals.size() == 3);
    REQUIRE(mesh.colors.size() == 3);
}

TEST_CASE("VTKLoader ignores colours outside a data section") {
    // Without a point or cell data keyword the colour lines are not colour lines at all.
    const std::string text =
        "DATASET POLYDATA\n"
        "POINTS 1 float\n"
        "0 0 0\n"
        "COLOR_SCALARS Colors 3\n"
        "1 1 1\n";
    const auto mesh = VTKLoader::parseAscii(text);
    REQUIRE(mesh.colors.empty());
}
