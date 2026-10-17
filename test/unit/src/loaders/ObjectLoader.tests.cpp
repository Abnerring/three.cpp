#include "catch.hpp"
#include "animation/AnimationUtils.h"
#include "core/BufferGeometry.h"
#include "core/Object3D.h"
#include "loaders/ObjectLoader.h"
#include "objects/Group.h"
#include "objects/Mesh.h"
#include "scenes/Scene.h"
#include <memory>
#include <stdexcept>

// The first slice of ObjectLoader: animations, geometries and materials are resolved first, then the object
// tree is built with the shared epilogue. These cases pin the pipeline order, the transform paths, the
// children recursion and the refusal of the types the slice does not cover.
TEST_CASE("ObjectLoader parses a scene tree") {
    const auto realm = JSValue::object();
    static_cast<void>(realm);
    const auto json = Animation_detail::parseJSON(R"({
        "object": {
            "type": "Scene", "uuid": "scene", "name": "root",
            "position": [1, 2, 3], "layers": 3, "visible": false,
            "userData": {"tag": 7},
            "children": [
                {"type": "Group", "uuid": "group", "name": "g", "scale": [2, 2, 2], "children": [
                    {"type": "Mesh", "uuid": "mesh", "material": "material", "geometry": "geometry"}
                ]}
            ]
        },
        "geometries": [{"type": "BufferGeometry", "uuid": "geometry", "name": "quad", "data": {"attributes": {}}}],
        "materials": [{"type": "MeshBasicMaterial", "uuid": "material", "color": 255}]
    })");
    ObjectLoader loader;
    const auto result = loader.parse(json);
    REQUIRE(result.object);
    const auto scene = std::dynamic_pointer_cast<Scene>(result.object);
    REQUIRE(scene);
    REQUIRE(scene->name == "root");
    REQUIRE(scene->uuid == "scene");
    REQUIRE(scene->position.x == 1.0);
    REQUIRE(scene->position.y == 2.0);
    REQUIRE(scene->position.z == 3.0);
    REQUIRE(scene->layers.mask == 3.0);
    REQUIRE(scene->visible == false);
    REQUIRE(scene->children.size() == 1);
    const auto group = std::dynamic_pointer_cast<Group>(scene->children[0]);
    REQUIRE(group);
    REQUIRE(group->scale.x == 2.0);
    const auto mesh = std::dynamic_pointer_cast<Mesh>(group->children[0]);
    REQUIRE(mesh);
    REQUIRE(mesh->geometry);
    REQUIRE(mesh->geometry->name == "quad");
    REQUIRE(mesh->material);
}

TEST_CASE("ObjectLoader takes the matrix path and refuses what it cannot build") {
    const auto realm = JSValue::object();
    static_cast<void>(realm);
    // Column-major, so the translation sits at the last four entries.
    const auto json = Animation_detail::parseJSON(R"({
        "object": {"type": "Object3D", "uuid": "a",
            "matrix": [1,0,0,0, 0,1,0,0, 0,0,1,0, 4,5,6,1]}
    })");
    ObjectLoader loader;
    const auto result = loader.parse(json);
    REQUIRE(result.object->position.x == 4.0);
    REQUIRE(result.object->position.y == 5.0);
    REQUIRE(result.object->position.z == 6.0);

    const auto unsupported = Animation_detail::parseJSON(R"({"object": {"type": "Points"}})");
    ObjectLoader other;
    REQUIRE_THROWS_AS(other.parse(unsupported), std::runtime_error);
}