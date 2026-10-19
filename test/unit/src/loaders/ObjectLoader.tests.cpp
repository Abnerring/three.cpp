#include "catch.hpp"
#include "animation/AnimationUtils.h"
#include "core/BufferGeometry.h"
#include "core/Object3D.h"
#include "loaders/ObjectLoader.h"
#include "objects/Group.h"
#include "objects/Mesh.h"
#include "lights/AmbientLight.h"
#include "lights/DirectionalLight.h"
#include "objects/Line.h"
#include "objects/Points.h"
#include "cameras/OrthographicCamera.h"
#include "cameras/PerspectiveCamera.h"
#include "objects/BatchedMesh.h"
#include "objects/InstancedMesh.h"
#include "objects/LOD.h"
#include "objects/Skeleton.h"
#include "objects/SkinnedMesh.h"
#include "objects/Sprite.h"
#include "scenes/Fog.h"
#include "scenes/FogExp2.h"
#include "scenes/Scene.h"
#include "textures/Texture.h"
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

    const auto unsupported = Animation_detail::parseJSON(R"({"object": {"type": "RectAreaLight"}})");
    ObjectLoader other;
    REQUIRE_THROWS_AS(other.parse(unsupported), std::runtime_error);
}
// The second slice adds the line and point families, the sprite, the level of detail node, the bone and the
// five lights. The interesting part is the directional and spot target: upstream stores the uuid and binds it
// after the tree exists, because the target may be a sibling that has not been parsed when the light is.
TEST_CASE("ObjectLoader binds a light target after the tree is built") {
    const auto realm = JSValue::object();
    static_cast<void>(realm);
    const auto json = Animation_detail::parseJSON(R"({
        "object": {"type": "Scene", "uuid": "scene", "children": [
            {"type": "DirectionalLight", "uuid": "sun", "color": 16777215, "intensity": 2, "target": "aim"},
            {"type": "Object3D", "uuid": "aim"}
        ]}
    })");
    ObjectLoader loader;
    const auto result = loader.parse(json);
    REQUIRE(result.object->children.size() == 2);
    const auto sun = std::dynamic_pointer_cast<DirectionalLight>(result.object->children[0]);
    REQUIRE(sun);
    REQUIRE(sun->intensity == 2.0);
    REQUIRE(sun->target);
    REQUIRE(sun->target->uuid == "aim");
}

TEST_CASE("ObjectLoader builds the line, point and sprite families") {
    const auto realm = JSValue::object();
    static_cast<void>(realm);
    const auto json = Animation_detail::parseJSON(R"({
        "object": {"type": "Scene", "uuid": "scene", "children": [
            {"type": "Line", "uuid": "line", "geometry": "g", "material": "m"},
            {"type": "Points", "uuid": "points", "geometry": "g", "material": "m"},
            {"type": "Sprite", "uuid": "sprite", "material": "m"}
        ]},
        "geometries": [{"type": "BufferGeometry", "uuid": "g", "data": {"attributes": {}}}],
        "materials": [{"type": "MeshBasicMaterial", "uuid": "m", "color": 255}]
    })");
    ObjectLoader loader;
    const auto result = loader.parse(json);
    REQUIRE(result.object->children.size() == 3);
    const auto line = std::dynamic_pointer_cast<Line>(result.object->children[0]);
    REQUIRE(line);
    REQUIRE(line->geometry);
    REQUIRE(line->material);
    const auto points = std::dynamic_pointer_cast<Points>(result.object->children[1]);
    REQUIRE(points);
    REQUIRE(points->material);
    const auto sprite = std::dynamic_pointer_cast<Sprite>(result.object->children[2]);
    REQUIRE(sprite);
    REQUIRE(sprite->material);
}
// The instanced mesh is the first object whose payload is not just a geometry and a material: the per
// instance matrix and the optional per instance colour are re-read as float attributes with the upstream
// item sizes.
TEST_CASE("ObjectLoader builds an instanced mesh from its matrix and colour") {
    const auto realm = JSValue::object();
    static_cast<void>(realm);
    const auto json = Animation_detail::parseJSON(R"({
        "object": {"type": "InstancedMesh", "uuid": "instanced", "material": "m", "geometry": "g", "count": 3,
            "instanceMatrix": {"array": [1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1]},
            "instanceColor": {"array": [1,0,0], "itemSize": 3}},
        "geometries": [{"type": "BufferGeometry", "uuid": "g", "data": {"attributes": {}}}],
        "materials": [{"type": "MeshBasicMaterial", "uuid": "m", "color": 255}]
    })");
    ObjectLoader loader;
    const auto result = loader.parse(json);
    const auto mesh = std::dynamic_pointer_cast<InstancedMesh>(result.object);
    REQUIRE(mesh);
    REQUIRE(mesh->instanceMatrix);
    REQUIRE(mesh->instanceMatrix->itemSize == 16.0);
    REQUIRE(mesh->instanceColor);
    REQUIRE(mesh->instanceColor->itemSize == 3.0);
    REQUIRE(mesh->count == 3);
}
// The two cameras take their upstream parameter order and the fields that only one of them has.
TEST_CASE("ObjectLoader builds both cameras") {
    const auto realm = JSValue::object();
    static_cast<void>(realm);
    const auto json = Animation_detail::parseJSON(R"({
        "object": {"type": "Scene", "uuid": "scene", "children": [
            {"type": "PerspectiveCamera", "uuid": "pcam", "fov": 60, "aspect": 2, "near": 0.5, "far": 500,
             "zoom": 2, "focus": 5, "filmGauge": 70, "filmOffset": 3},
            {"type": "OrthographicCamera", "uuid": "ocam", "left": -2, "right": 2, "top": 3, "bottom": -3,
             "near": 0.1, "far": 50, "zoom": 4}
        ]}
    })");
    ObjectLoader loader;
    const auto result = loader.parse(json);
    REQUIRE(result.object->children.size() == 2);
    const auto perspective = std::dynamic_pointer_cast<PerspectiveCamera>(result.object->children[0]);
    REQUIRE(perspective);
    REQUIRE(perspective->fov == 60.0);
    REQUIRE(perspective->aspect == 2.0);
    REQUIRE(perspective->zoom == 2.0);
    REQUIRE(perspective->focus == 5.0);
    const auto orthographic = std::dynamic_pointer_cast<OrthographicCamera>(result.object->children[1]);
    REQUIRE(orthographic);
    REQUIRE(orthographic->left == -2.0);
    REQUIRE(orthographic->right == 2.0);
    REQUIRE(orthographic->top == 3.0);
    REQUIRE(orthographic->bottom == -3.0);
    REQUIRE(orthographic->zoom == 4.0);
}
// The remaining scene fields: both fog types and the background and environment numbers and eulers.
TEST_CASE("ObjectLoader builds the scene fog and background fields") {
    const auto realm = JSValue::object();
    static_cast<void>(realm);
    const auto json = Animation_detail::parseJSON(R"({
        "object": {"type": "Scene", "uuid": "scene", "background": 255,
            "backgroundBlurriness": 0.5, "backgroundIntensity": 2, "environmentIntensity": 3,
            "backgroundRotation": [1, 2, 3], "environmentRotation": [4, 5, 6],
            "fog": {"type": "Fog", "color": 16777215, "near": 1, "far": 100}}
    })");
    ObjectLoader loader;
    const auto result = loader.parse(json);
    const auto scene = std::dynamic_pointer_cast<Scene>(result.object);
    REQUIRE(scene);
    REQUIRE(scene->background.has_value());
    REQUIRE(scene->backgroundBlurriness == 0.5);
    REQUIRE(scene->backgroundIntensity == 2.0);
    REQUIRE(scene->environmentIntensity == 3.0);
    REQUIRE(scene->backgroundRotation.x == 1.0);
    REQUIRE(scene->environmentRotation.z == 6.0);
    const auto fog = std::dynamic_pointer_cast<Fog>(scene->fog);
    REQUIRE(fog);
    REQUIRE(fog->near == 1.0);
    REQUIRE(fog->far == 100.0);
}

TEST_CASE("ObjectLoader builds an exponential fog and refuses a texture environment") {
    const auto realm = JSValue::object();
    static_cast<void>(realm);
    const auto json = Animation_detail::parseJSON(R"({
        "object": {"type": "Scene", "uuid": "scene",
            "fog": {"type": "FogExp2", "color": 255, "density": 0.5}}
    })");
    ObjectLoader loader;
    const auto result = loader.parse(json);
    const auto scene = std::dynamic_pointer_cast<Scene>(result.object);
    const auto fog = std::dynamic_pointer_cast<FogExp2>(scene->fog);
    REQUIRE(fog);
    REQUIRE(fog->density == 0.5);

    const auto texture = Animation_detail::parseJSON(R"({"object": {"type": "Scene", "environment": "env"}})");
    ObjectLoader other;
    REQUIRE_THROWS_AS(other.parse(texture), std::runtime_error);
}
// A level names one of the object's own children by uuid, so the restore has to run after the children exist.
TEST_CASE("ObjectLoader restores the levels of a level of detail node") {
    const auto realm = JSValue::object();
    static_cast<void>(realm);
    const auto json = Animation_detail::parseJSON(R"({
        "object": {"type": "LOD", "uuid": "lod", "autoUpdate": false,
            "levels": [{"object": "near", "distance": 10, "hysteresis": 0.5}, {"object": "far", "distance": 20}],
            "children": [
                {"type": "Mesh", "uuid": "near", "material": "m", "geometry": "g"},
                {"type": "Mesh", "uuid": "far", "material": "m", "geometry": "g"}
            ]},
        "geometries": [{"type": "BufferGeometry", "uuid": "g", "data": {"attributes": {}}}],
        "materials": [{"type": "MeshBasicMaterial", "uuid": "m", "color": 255}]
    })");
    ObjectLoader loader;
    const auto result = loader.parse(json);
    const auto lod = std::dynamic_pointer_cast<LOD>(result.object);
    REQUIRE(lod);
    REQUIRE(lod->autoUpdate.toBoolean() == false);
    const auto levels = lod->levels;
    REQUIRE(levels.length() == 2.0);
    REQUIRE(levels.get("0").get("distance").toNumber() == 10.0);
    REQUIRE(levels.get("1").get("distance").toNumber() == 20.0);
}
// Texture records are pure data apart from the image, which is a host concern, so every other field is
// carried over and the material that references the texture by uuid resolves to it.
TEST_CASE("ObjectLoader builds texture records and resolves them from materials") {
    const auto realm = JSValue::object();
    static_cast<void>(realm);
    const auto json = Animation_detail::parseJSON(R"({
        "object": {"type": "Mesh", "uuid": "mesh", "material": "m", "geometry": "g"},
        "geometries": [{"type": "BufferGeometry", "uuid": "g", "data": {"attributes": {}}}],
        "textures": [{"uuid": "tex", "name": "checker", "repeat": [2, 3], "wrap": [1000, 1001],
            "magFilter": 1006, "colorSpace": "srgb", "flipY": false}],
        "materials": [{"type": "MeshBasicMaterial", "uuid": "m", "color": 255, "map": "tex"}]
    })");
    ObjectLoader loader;
    const auto result = loader.parse(json);
    const auto mesh = std::dynamic_pointer_cast<Mesh>(result.object);
    REQUIRE(mesh);
    REQUIRE(mesh->material);
    const auto material = std::dynamic_pointer_cast<MeshBasicMaterial>(mesh->material);
    REQUIRE(material);
    REQUIRE(material->map);
    REQUIRE(material->map->uuid == "tex");
    REQUIRE(material->map->repeat.x == 2.0);
    REQUIRE(material->map->repeat.y == 3.0);
    REQUIRE(material->map->wrapS == 1000.0);
    REQUIRE(material->map->wrapT == 1001.0);
    REQUIRE(material->map->magFilter == 1006.0);
    REQUIRE(material->map->colorSpace == "srgb");
    REQUIRE(material->map->flipY == false);
}
// A skeleton resolves its bones by uuid against the finished tree, and the skinned mesh that named it is
// bound afterwards, which is the same two step shape as the light targets.
TEST_CASE("ObjectLoader binds a skinned mesh to its skeleton") {
    const auto realm = JSValue::object();
    static_cast<void>(realm);
    const auto json = Animation_detail::parseJSON(R"({
        "object": {"type": "Scene", "uuid": "scene", "children": [
            {"type": "Bone", "uuid": "bone0"},
            {"type": "Bone", "uuid": "bone1"},
            {"type": "SkinnedMesh", "uuid": "skin", "material": "m", "geometry": "g", "skeleton": "skel",
             "bindMode": "detached"}
        ]},
        "geometries": [{"type": "BufferGeometry", "uuid": "g", "data": {"attributes": {}}}],
        "materials": [{"type": "MeshBasicMaterial", "uuid": "m", "color": 255}],
        "skeletons": [{"uuid": "skel", "bones": ["bone0", "bone1"], "boneInverses": [[1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1],[1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1]]}]
    })");
    ObjectLoader loader;
    const auto result = loader.parse(json);
    const auto mesh = std::dynamic_pointer_cast<SkinnedMesh>(result.object->children[2]);
    REQUIRE(mesh);
    REQUIRE(mesh->bindMode == "detached");
    REQUIRE(mesh->skeleton);
    REQUIRE(mesh->skeleton->bones.size() == 2);
}
// A mesh may name several materials, which upstream keeps as an array and which decides the draw groups.
TEST_CASE("ObjectLoader keeps a material array on a mesh") {
    const auto realm = JSValue::object();
    static_cast<void>(realm);
    const auto json = Animation_detail::parseJSON(R"({
        "object": {"type": "Mesh", "uuid": "mesh", "geometry": "g", "material": ["m1", "m2"]},
        "geometries": [{"type": "BufferGeometry", "uuid": "g", "data": {"attributes": {}}}],
        "materials": [{"type": "MeshBasicMaterial", "uuid": "m1", "color": 255},
                      {"type": "MeshBasicMaterial", "uuid": "m2", "color": 65280}]
    })");
    ObjectLoader loader;
    const auto result = loader.parse(json);
    const auto mesh = std::dynamic_pointer_cast<Mesh>(result.object);
    REQUIRE(mesh);
    REQUIRE(mesh->materialIsArray);
    REQUIRE(mesh->materials.size() == 2);
    // The port keeps an array in materials and leaves the single material empty, which is its convention.
    REQUIRE(mesh->material == nullptr);
    REQUIRE(mesh->materials[0]->uuid == "m1");
    REQUIRE(mesh->materials[1]->uuid == "m2");
}

// The line and point families have no array form in this port yet, so an array is refused by name rather
// than reduced to its first entry.
TEST_CASE("ObjectLoader refuses a material array on a line") {
    const auto realm = JSValue::object();
    static_cast<void>(realm);
    const auto json = Animation_detail::parseJSON(R"({
        "object": {"type": "Line", "uuid": "line", "geometry": "g", "material": ["m1", "m2"]},
        "geometries": [{"type": "BufferGeometry", "uuid": "g", "data": {"attributes": {}}}],
        "materials": [{"type": "MeshBasicMaterial", "uuid": "m1", "color": 255},
                      {"type": "MeshBasicMaterial", "uuid": "m2", "color": 65280}]
    })");
    ObjectLoader loader;
    REQUIRE_THROWS_AS(loader.parse(json), std::runtime_error);
}
// A batched mesh carries its whole bookkeeping in the document, so the loader has to put every record back
// rather than rebuild the batch from the geometry alone.
TEST_CASE("ObjectLoader restores a batched mesh") {
    const auto realm = JSValue::object();
    static_cast<void>(realm);
    const auto json = Animation_detail::parseJSON(R"({
        "object": {"type": "BatchedMesh", "uuid": "batch", "geometry": "g", "material": "m",
            "maxInstanceCount": 4, "maxVertexCount": 8, "maxIndexCount": 16,
            "perObjectFrustumCulled": false, "sortObjects": false,
            "geometryInitialized": true, "geometryCount": 1,
            "nextVertexStart": 3, "nextIndexStart": 3,
            "geometryInfo": [{"vertexStart": 0, "vertexCount": 3, "reservedVertexCount": 3,
                              "indexStart": 0, "indexCount": 3, "reservedIndexCount": 3,
                              "start": 0, "count": 3, "active": true}],
            "instanceInfo": [{"geometryIndex": 0, "active": true, "visible": false}],
            "availableInstanceIds": [], "availableGeometryIds": []},
        "geometries": [{"type": "BufferGeometry", "uuid": "g", "data": {"attributes": {}}}],
        "materials": [{"type": "MeshBasicMaterial", "uuid": "m", "color": 255}]
    })");
    ObjectLoader loader;
    const auto result = loader.parse(json);
    const auto batch = std::dynamic_pointer_cast<BatchedMesh>(result.object);
    REQUIRE(batch);
    REQUIRE(batch->perObjectFrustumCulled == false);
    REQUIRE(batch->sortObjects == false);
    REQUIRE(batch->getInstanceCount() == 1.0);
    REQUIRE(batch->getVisibleAt(0.0) == false);
    const auto range = batch->getGeometryRangeAt(0.0);
    REQUIRE(range.vertexStart == 0.0);
    REQUIRE(range.reservedVertexCount == 3.0);
    REQUIRE(range.count == 3.0);
    REQUIRE(batch->getUnusedVertexCount() == 5.0);    // eight vertices, three already reserved
}