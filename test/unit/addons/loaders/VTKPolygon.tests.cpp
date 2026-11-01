#include "loaders/VTKLoader.h"

#include "catch.hpp"

#include <string>
#include <vector>

TEST_CASE("VTKLoader reads a connectivity line") {
    VTKLoader::Connectivity connectivity;
    REQUIRE(VTKLoader::parseConnectivity("4 0 1 2 3", connectivity));
    REQUIRE(connectivity.numVertices == 4);
    REQUIRE(connectivity.indices.size() == 4);
    REQUIRE(connectivity.indices[0] == 0);
    REQUIRE(connectivity.indices[3] == 3);
}

TEST_CASE("VTKLoader rejects a line without a count") {
    VTKLoader::Connectivity connectivity;
    // A keyword line has no leading count.
    REQUIRE_FALSE(VTKLoader::parseConnectivity("POLYGONS 2 8", connectivity));
    REQUIRE(VTKLoader::parseConnectivity("", connectivity) == false);
}

TEST_CASE("VTKLoader splits a polygon into triangles") {
    // A quad fans into two triangles sharing the first node.
    const std::vector<int> quad{0, 1, 2, 3};
    const auto triangles = VTKLoader::triangulateFan(quad, 4);
    REQUIRE(triangles.size() == 6);
    REQUIRE(triangles[0] == 0);
    REQUIRE(triangles[1] == 1);
    REQUIRE(triangles[2] == 2);
    // The second triangle replaces the middle node.
    REQUIRE(triangles[3] == 0);
    REQUIRE(triangles[4] == 2);
    REQUIRE(triangles[5] == 3);
}

TEST_CASE("VTKLoader drops a polygon with too few nodes") {
    // Two nodes cannot make a triangle.
    REQUIRE(VTKLoader::triangulateFan({0, 1}, 2).empty());
    REQUIRE(VTKLoader::triangulateFan({}, 0).empty());
    // A triangle passes through unchanged.
    const auto one = VTKLoader::triangulateFan({5, 6, 7}, 3);
    REQUIRE(one.size() == 3);
    REQUIRE(one[0] == 5);
    REQUIRE(one[2] == 7);
}

TEST_CASE("VTKLoader tells a number line from a keyword line") {
    REQUIRE(VTKLoader::isNumberLine("1.0 2.0 3.0"));
    REQUIRE(VTKLoader::isNumberLine("-1 -2"));
    REQUIRE_FALSE(VTKLoader::isNumberLine("POINTS 8 float"));
    // A line that starts with spaces then a word is still a keyword line.
    REQUIRE_FALSE(VTKLoader::isNumberLine("   POINTS 8 float"));
}

TEST_CASE("VTKLoader reads a hexahedron as four triangles") {
    // Six nodes give four triangles.
    const std::vector<int> hex{0, 1, 2, 3, 4, 5};
    const auto triangles = VTKLoader::triangulateFan(hex, 6);
    REQUIRE(triangles.size() == 12);
    // The last triangle is the fifth and sixth nodes with the first.
    REQUIRE(triangles[9] == 0);
    REQUIRE(triangles[10] == 4);
    REQUIRE(triangles[11] == 5);
}
