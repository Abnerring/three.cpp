#include "loaders/VTKLoader.h"

#include "catch.hpp"

#include <string>
#include <vector>

TEST_CASE("VTKLoader triangulates a strip with alternating winding") {
    // Four nodes make two triangles, and the second swaps its last two.
    const std::vector<int> strip{0, 1, 2, 3};
    const auto triangles = VTKLoader::triangulateStrip(strip, 4);
    REQUIRE(triangles.size() == 6);
    // The first keeps the order it has.
    REQUIRE(triangles[0] == 0);
    REQUIRE(triangles[1] == 1);
    REQUIRE(triangles[2] == 2);
    // The second takes one, three, two.
    REQUIRE(triangles[3] == 1);
    REQUIRE(triangles[4] == 3);
    REQUIRE(triangles[5] == 2);
}

TEST_CASE("VTKLoader keeps the strip winding consistent") {
    // Six nodes give four triangles, alternating the swap each time.
    const std::vector<int> strip{0, 1, 2, 3, 4, 5};
    const auto triangles = VTKLoader::triangulateStrip(strip, 6);
    REQUIRE(triangles.size() == 12);
    // Second triangle is odd.
    REQUIRE(triangles[4] == 3);
    REQUIRE(triangles[5] == 2);
    // Third is even again.
    REQUIRE(triangles[6] == 2);
    REQUIRE(triangles[7] == 3);
    REQUIRE(triangles[8] == 4);
    // Fourth is odd.
    REQUIRE(triangles[10] == 5);
    REQUIRE(triangles[11] == 4);
}

TEST_CASE("VTKLoader drops a strip with too few nodes") {
    REQUIRE(VTKLoader::triangulateStrip({0, 1}, 2).empty());
    REQUIRE(VTKLoader::triangulateStrip({}, 0).empty());
    // A single triangle passes through in order.
    const auto one = VTKLoader::triangulateStrip({7, 8, 9}, 3);
    REQUIRE(one.size() == 3);
    REQUIRE(one[1] == 8);
}

TEST_CASE("VTKLoader reads a strip line into a mesh") {
    const std::string text =
        "DATASET POLYDATA\n"
        "POINTS 4 float\n"
        "0 0 0  1 0 0  1 1 0  0 1 0\n"
        "TRIANGLE_STRIPS 1 5\n"
        "4 0 1 2 3\n";
    const auto mesh = VTKLoader::parseAscii(text);
    REQUIRE(mesh.positions.size() == 12);
    REQUIRE(mesh.indices.size() == 6);
    // The second triangle is the swapped one.
    REQUIRE(mesh.indices[3] == 1);
    REQUIRE(mesh.indices[4] == 3);
    REQUIRE(mesh.indices[5] == 2);
}
