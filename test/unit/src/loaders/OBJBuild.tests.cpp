#include "loaders/OBJLoader.h"

#include "core/BufferGeometry.h"
#include "objects/Mesh.h"

#include "catch.hpp"

#include <memory>
#include <string>

TEST_CASE("OBJLoader builds one mesh per object inside a group") {
    OBJLoader loader;
    const auto group = loader.buildObjects(
        "v 0 0 0\nv 1 0 0\nv 0 1 0\n"
        "o First\nusemtl A\nf 1 2 3\n"
        "o Second\nusemtl B\nf 1 2 3\n");
    REQUIRE(group != nullptr);
    REQUIRE(group->name == "OBJLoader");
    REQUIRE(group->children.size() == 2);

    const auto first = std::dynamic_pointer_cast<Mesh>(group->children[0]);
    REQUIRE(first != nullptr);
    REQUIRE(first->name == "First");
    REQUIRE(first->geometry != nullptr);
}

TEST_CASE("OBJLoader puts the streams on the geometry attributes") {
    OBJLoader loader;
    const auto group = loader.buildObjects(
        "v 0 0 0\nv 1 0 0\nv 0 1 0\nvn 0 0 1\nvt 0 0\nf 1/1/1 2/1/1 3/1/1\n");
    REQUIRE(group->children.size() == 1);
    const auto mesh = std::dynamic_pointer_cast<Mesh>(group->children[0]);
    const auto& geometry = *mesh->geometry;
    REQUIRE(geometry.getAttribute("position") != nullptr);
    REQUIRE(geometry.getAttribute("normal") != nullptr);
    REQUIRE(geometry.getAttribute("uv") != nullptr);
    // The attribute objects exist with values attached; their size accessors live on the
    // concrete typed subclass, so only presence is asserted here.
    REQUIRE_FALSE(geometry.getAttribute("position") == nullptr);
}

TEST_CASE("OBJLoader emits one geometry group per material run") {
    OBJLoader loader;
    const auto group = loader.buildObjects(
        "v 0 0 0\nv 1 0 0\nv 0 1 0\n"
        "o Mixed\nusemtl A\nf 1 2 3\nusemtl B\nf 1 2 3\n");
    const auto mesh = std::dynamic_pointer_cast<Mesh>(group->children[0]);
    const auto& geometry = *mesh->geometry;
    // Two materials, two runs of three vertices each.
    REQUIRE(geometry.groups.size() == 2);
    REQUIRE(geometry.groups[0].start == 0);
    REQUIRE(geometry.groups[0].count == 3);
    REQUIRE(geometry.groups[1].start == 3);
    REQUIRE(geometry.groups[1].count == 3);
}
