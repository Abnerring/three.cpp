#include "loaders/VTKLoader.h"

#include "catch.hpp"

#include <string>

TEST_CASE("VTKLoader reads a whole text file") {
    const std::string text =
        "# vtk DataFile Version 3.0\n"
        "a quad\n"
        "ASCII\n"
        "DATASET POLYDATA\n"
        "POINTS 4 float\n"
        "0 0 0  1 0 0  1 1 0  0 1 0\n"
        "POLYGONS 1 5\n"
        "4 0 1 2 3\n";
    const auto mesh = VTKLoader::parseAscii(text);
    // Four points, each three values.
    REQUIRE(mesh.positions.size() == 12);
    REQUIRE(mesh.positions[0] == 0.0);
    REQUIRE(mesh.positions[3] == 1.0);
    // The quad fans into two triangles.
    REQUIRE(mesh.indices.size() == 6);
    REQUIRE(mesh.indices[0] == 0);
    REQUIRE(mesh.indices[2] == 2);
    REQUIRE(mesh.indices[5] == 3);
}

TEST_CASE("VTKLoader reads points across several lines") {
    const std::string text =
        "DATASET POLYDATA\n"
        "POINTS 2 float\n"
        "0 0 0\n"
        "1 1 1\n";
    const auto mesh = VTKLoader::parseAscii(text);
    REQUIRE(mesh.positions.size() == 6);
    REQUIRE(mesh.positions[5] == 1.0);
    REQUIRE(mesh.indices.empty());
}

TEST_CASE("VTKLoader rejects a dataset that is not polygonal") {
    const std::string text =
        "DATASET STRUCTURED_POINTS\n"
        "POINTS 1 float\n";
    REQUIRE_THROWS_WITH(VTKLoader::parseAscii(text),
                        "Unsupported DATASET type: STRUCTURED_POINTS");
}

TEST_CASE("VTKLoader ignores a keyword line inside a points section") {
    // The points branch runs first, sees a keyword rather than numbers, and the polygon branch
    // is not tried, so only the points are read.
    const std::string text =
        "DATASET POLYDATA\n"
        "POINTS 2 float\n"
        "0 0 0  1 1 1\n"
        "POLYGONS 1 4\n"
        "3 0 1 0\n";
    const auto mesh = VTKLoader::parseAscii(text);
    REQUIRE(mesh.positions.size() == 6);
    // The polygon line after the switch is read, since the section changed in between.
    REQUIRE(mesh.indices.size() == 3);
}

TEST_CASE("VTKLoader reads an empty file as nothing") {
    const auto mesh = VTKLoader::parseAscii("");
    REQUIRE(mesh.positions.empty());
    REQUIRE(mesh.indices.empty());
}
