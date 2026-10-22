#include "loaders/OBJLoader.h"

#include "catch.hpp"

#include <vector>

TEST_CASE("OBJLoader builds a face normal from the three corners") {
    // A triangle in the XY plane: A (0,0,0), B (1,0,0), C (0,1,0).
    const std::vector<double> vertices{0, 0, 0, 1, 0, 0, 0, 1, 0};
    std::vector<double> normals;
    OBJLoader::addFaceNormal(vertices, 0, 3, 6, normals);
    // The normal is appended once per corner.
    REQUIRE(normals.size() == 9);
    // Upstream crosses C minus B with A minus B, which gives positive Z for this winding.
    REQUIRE(normals[0] == Approx(0.0).margin(1e-12));
    REQUIRE(normals[1] == Approx(0.0).margin(1e-12));
    REQUIRE(normals[2] == Approx(1.0).margin(1e-12));
    // All three copies are identical.
    REQUIRE(normals[3] == normals[0]);
    REQUIRE(normals[7] == normals[1]);
}

TEST_CASE("OBJLoader normalises the face normal") {
    // A larger triangle in the same plane has the same direction but a larger cross product,
    // and the result is still unit length.
    const std::vector<double> vertices{0, 0, 0, 5, 0, 0, 0, 5, 0};
    std::vector<double> normals;
    OBJLoader::addFaceNormal(vertices, 0, 3, 6, normals);
    REQUIRE(normals[2] == Approx(1.0).margin(1e-12));
    REQUIRE(normals.size() == 9);
}

TEST_CASE("OBJLoader yields a zero normal for a degenerate triangle") {
    // Three collinear points have no direction, so the normal is all zeroes rather than a
    // division by zero.
    const std::vector<double> vertices{0, 0, 0, 1, 0, 0, 2, 0, 0};
    std::vector<double> normals;
    OBJLoader::addFaceNormal(vertices, 0, 3, 6, normals);
    REQUIRE(normals.size() == 9);
    for (const double value : normals) REQUIRE(value == 0.0);
}
