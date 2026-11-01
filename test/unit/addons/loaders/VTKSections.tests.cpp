#include "loaders/VTKLoader.h"

#include "catch.hpp"

#include <string>

TEST_CASE("VTKLoader switches sections on a keyword") {
    VTKLoader::Sections sections;
    // Points opens the points section and closes the others.
    VTKLoader::applyKeyword("POINTS 8 float", sections);
    REQUIRE(sections.points);
    REQUIRE_FALSE(sections.polygons);

    // Polygons takes over from points.
    VTKLoader::applyKeyword("POLYGONS 2 8", sections);
    REQUIRE(sections.polygons);
    REQUIRE_FALSE(sections.points);

    // Triangle strips take over from polygons.
    VTKLoader::applyKeyword("TRIANGLE_STRIPS 2 8", sections);
    REQUIRE(sections.triangleStrips);
    REQUIRE_FALSE(sections.polygons);
}

TEST_CASE("VTKLoader leaves the other flags as they were") {
    VTKLoader::Sections sections;
    // A point data keyword does not clear the normals or colours flags.
    VTKLoader::applyKeyword("NORMALS Normals float", sections);
    REQUIRE(sections.normals);
    VTKLoader::applyKeyword("POINT_DATA 8", sections);
    REQUIRE(sections.pointData);
    REQUIRE(sections.normals);
    // A normals keyword clears the colours flag but not the point data one.
    VTKLoader::applyKeyword("COLOR_SCALARS Colors 3", sections);
    REQUIRE(sections.colors);
    REQUIRE_FALSE(sections.normals);
    VTKLoader::applyKeyword("NORMALS Normals float", sections);
    REQUIRE(sections.normals);
    REQUIRE_FALSE(sections.colors);
    REQUIRE(sections.pointData);
}

TEST_CASE("VTKLoader ignores a line with no keyword") {
    VTKLoader::Sections sections;
    VTKLoader::applyKeyword("1.0 2.0 3.0", sections);
    REQUIRE_FALSE(sections.points);
    REQUIRE_FALSE(sections.polygons);
    // A bare word without its trailing space does not match either.
    VTKLoader::applyKeyword("POINTS", sections);
    REQUIRE_FALSE(sections.points);
}

TEST_CASE("VTKLoader recognises the cell data keyword") {
    VTKLoader::Sections sections;
    VTKLoader::applyKeyword("CELL_DATA 4", sections);
    REQUIRE(sections.cellData);
    REQUIRE_FALSE(sections.points);
}

TEST_CASE("VTKLoader does not confuse point data with points") {
    // The point data keyword must not open the points section.
    VTKLoader::Sections sections;
    VTKLoader::applyKeyword("POINT_DATA 8", sections);
    REQUIRE(sections.pointData);
    REQUIRE_FALSE(sections.points);
}
