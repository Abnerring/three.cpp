#include "loaders/OBJLoader.h"

#include "catch.hpp"

#include <string>

TEST_CASE("OBJLoader reads the three vertex streams") {
    OBJLoader loader;
    const auto geometry = loader.parseGeometry(
        "v 1 2 3\nv 4 5 6\nv 7 8 9\nvt 0 1\nvn 0 1 0\n");
    REQUIRE(geometry.vertices.size() == 9);
    REQUIRE(geometry.vertices[0] == 1.0);
    REQUIRE(geometry.vertices[8] == 9.0);
    REQUIRE(geometry.uvs.size() == 2);
    REQUIRE(geometry.uvs[0] == 0.0);
    REQUIRE(geometry.normals.size() == 3);
    REQUIRE(geometry.normals[1] == 1.0);
    // Without colours the stream still has one placeholder triple per vertex.
    REQUIRE(geometry.colors.size() == 9);
    REQUIRE_FALSE(geometry.hasVertexColors);
    REQUIRE(geometry.triangles.empty());
}

TEST_CASE("OBJLoader resolves one based and relative face indices") {
    OBJLoader loader;
    const auto geometry = loader.parseGeometry(
        "v 0 0 0\nv 1 0 0\nv 0 1 0\nvn 0 0 1\nvt 0 0\nf 1/1/1 2/1/1 3/1/1\n");
    REQUIRE(geometry.triangles.size() == 1);
    const auto& triangle = geometry.triangles[0];
    // Indices are multiplied by the stride of their kind, so vertex indices step by three.
    REQUIRE(triangle[0] == 0);
    REQUIRE(triangle[1] == 3);
    REQUIRE(triangle[2] == 6);
    REQUIRE(triangle[3] == 0);
    REQUIRE(triangle[6] == 0);

    // Negative indices count back from the end of the stream.
    const auto relative = loader.parseGeometry(
        "v 0 0 0\nv 1 0 0\nv 0 1 0\nf -3 -2 -1\n");
    REQUIRE(relative.triangles.size() == 1);
    REQUIRE(relative.triangles[0][0] == 0);
    REQUIRE(relative.triangles[0][1] == 3);
    REQUIRE(relative.triangles[0][2] == 6);
}

TEST_CASE("OBJLoader fans a quad into two triangles") {
    OBJLoader loader;
    const auto geometry = loader.parseGeometry(
        "v 0 0 0\nv 1 0 0\nv 1 1 0\nv 0 1 0\nf 1 2 3 4\n");
    REQUIRE(geometry.triangles.size() == 2);
    REQUIRE(geometry.triangles[0][0] == 0);
    REQUIRE(geometry.triangles[0][2] == 6);
    REQUIRE(geometry.triangles[1][0] == 0);
    REQUIRE(geometry.triangles[1][1] == 6);
    REQUIRE(geometry.triangles[1][2] == 9);
}

TEST_CASE("OBJLoader converts vertex colours out of sRGB") {
    OBJLoader loader;
    const auto geometry = loader.parseGeometry(
        "v 1 2 3 0.5 0.25 0.125\nv 4 5 6\n");
    REQUIRE(geometry.hasVertexColors);
    REQUIRE(geometry.colors.size() == 6);
    // The first vertex had a colour and it was converted, so it is positive but not the raw
    // value; the second vertex has placeholders.
    // Half in sRGB becomes about 0.214 in the working space.
    REQUIRE(geometry.colors[0] == Approx(0.214041).epsilon(1e-6));
    REQUIRE(geometry.colors[3] == 0.0);
}

TEST_CASE("OBJLoader skips comments and joins continued lines") {
    OBJLoader loader;
    const auto geometry = loader.parseGeometry(
        "# a comment\nv 0 0 0\nv 1 0 0\nv 0 1 0\nf 1 2 \\\n3\n");
    REQUIRE(geometry.vertices.size() == 9);
    // The backslash newline pair was removed, so the face line reads as one.
    REQUIRE(geometry.triangles.size() == 1);
}
