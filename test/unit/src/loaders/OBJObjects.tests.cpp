#include "loaders/OBJLoader.h"

#include "catch.hpp"

#include <string>

TEST_CASE("OBJLoader splits the file into objects on o and g") {
    OBJLoader loader;
    const auto objects = loader.parseObjects(
        "v 0 0 0\nv 1 0 0\nv 0 1 0\n"
        "o First\nusemtl A\nf 1 2 3\n"
        "o Second\nusemtl B\nf 1 2 3\n");
    // The initial undeclared object is renamed by the first o, so there are two.
    REQUIRE(objects.size() == 2);
    REQUIRE(objects[0].name == "First");
    REQUIRE(objects[0].fromDeclaration);
    REQUIRE(objects[1].name == "Second");
    REQUIRE(objects[0].vertices.size() == 9);
    REQUIRE(objects[1].vertices.size() == 9);
}

TEST_CASE("OBJLoader records one material run per object") {
    OBJLoader loader;
    const auto objects = loader.parseObjects(
        "v 0 0 0\nv 1 0 0\nv 0 1 0\n"
        "o First\nusemtl A\nf 1 2 3\n"
        "o Second\nusemtl B\nf 1 2 3\n");
    REQUIRE(objects[0].groups.materials().size() == 1);
    REQUIRE(objects[0].groups.materials()[0].name == "A");
    // The run covers the three vertices the face contributed.
    REQUIRE(objects[0].groups.materials()[0].groupStart == 0);
    REQUIRE(objects[0].groups.materials()[0].groupEnd == 3);
    REQUIRE(objects[1].groups.materials()[0].name == "B");
}

TEST_CASE("OBJLoader keeps a single object when the file declares none") {
    OBJLoader loader;
    const auto objects = loader.parseObjects("v 0 0 0\nv 1 0 0\nv 0 1 0\nf 1 2 3\n");
    REQUIRE(objects.size() == 1);
    // The undeclared object keeps its empty name and is not marked as a declaration.
    REQUIRE(objects[0].name.empty());
    REQUIRE_FALSE(objects[0].fromDeclaration);
    REQUIRE(objects[0].vertices.size() == 9);
    // With no usemtl at all, closing guarantees one placeholder material.
    REQUIRE(objects[0].groups.materials().size() == 1);
    REQUIRE(objects[0].groups.materials()[0].name.empty());
}

TEST_CASE("OBJLoader inherits the smooth setting from the s statement") {
    OBJLoader loader;
    const auto objects = loader.parseObjects(
        "v 0 0 0\nv 1 0 0\nv 0 1 0\ns off\nusemtl A\nf 1 2 3\n");
    REQUIRE(objects.size() == 1);
    REQUIRE_FALSE(objects[0].groups.smooth());
    REQUIRE_FALSE(objects[0].groups.materials()[0].smooth);
}
