#include "exporters/OBJExporter.h"

#include "core/BufferAttribute.h"
#include "core/BufferGeometry.h"
#include "geometries/BoxGeometry.h"
#include "materials/MeshBasicMaterial.h"
#include "math/Matrix4.h"
#include "objects/Mesh.h"

#include "catch.hpp"

#include <memory>

TEST_CASE("OBJExporter writes a triangle with normals and uvs") {
    auto geometry = std::make_shared<BufferGeometry>();
    geometry->setAttribute("position", std::make_shared<BufferAttribute>(
                                           std::vector<float>{0, 0, 0, 1, 0, 0, 0, 1, 0}, 3));
    geometry->setAttribute("normal", std::make_shared<BufferAttribute>(
                                         std::vector<float>{0, 0, 1, 0, 0, 1, 0, 0, 1}, 3));
    geometry->setAttribute("uv", std::make_shared<BufferAttribute>(
                                     std::vector<float>{0, 0, 1, 0, 0, 1}, 2));
    auto mesh = std::make_shared<Mesh>(geometry, std::make_shared<MeshBasicMaterial>());
    mesh->name = "triangle";

    Object3D scene;
    scene.add(mesh);

    const std::string obj = OBJExporter().parse(scene);
    REQUIRE(obj ==
            "o triangle\n"
            "v 0 0 0\nv 1 0 0\nv 0 1 0\n"
            "vt 0 0\nvt 1 0\nvt 0 1\n"
            "vn 0 0 1\nvn 0 0 1\nvn 0 0 1\n"
            "f 1/1/1 2/2/2 3/3/3\n");
}

TEST_CASE("OBJExporter walks the index and applies the world matrix") {
    auto geometry = std::make_shared<BoxGeometry>();
    auto material = std::make_shared<MeshBasicMaterial>();
    material->name = "mat";
    auto mesh = std::make_shared<Mesh>(geometry, material);
    mesh->name = "box";
    mesh->matrixWorld.makeTranslation(1, 0, 0);

    Object3D scene;
    scene.add(mesh);

    const std::string obj = OBJExporter().parse(scene);
    REQUIRE(obj.find("o box\nusemtl mat\n") == 0);
    // The first vertex of the box is at (-0.5, 0.5, 0.5) offset by the world translation.
    REQUIRE(obj.find("v 0.5 0.5 0.5\n") != std::string::npos);
    REQUIRE(obj.find("\nf ") != std::string::npos);
    REQUIRE(obj.find("/") != std::string::npos);
}

TEST_CASE("OBJExporter accumulates the vertex index across meshes") {
    auto firstGeometry = std::make_shared<BufferGeometry>();
    firstGeometry->setAttribute("position", std::make_shared<BufferAttribute>(
                                                std::vector<float>{0, 0, 0, 1, 0, 0, 0, 1, 0}, 3));
    auto first = std::make_shared<Mesh>(firstGeometry, std::make_shared<MeshBasicMaterial>());
    first->name = "first";

    auto secondGeometry = std::make_shared<BufferGeometry>();
    secondGeometry->setAttribute("position", std::make_shared<BufferAttribute>(
                                                 std::vector<float>{2, 0, 0, 3, 0, 0, 2, 1, 0}, 3));
    auto second = std::make_shared<Mesh>(secondGeometry, std::make_shared<MeshBasicMaterial>());
    second->name = "second";

    Object3D scene;
    scene.add(first);
    scene.add(second);

    // The second mesh has no index of its own, so its face indices must be offset by the three
    // vertices written for the first mesh.
    const std::string obj = OBJExporter().parse(scene);
    REQUIRE(obj.find("o first\nv 0 0 0\nv 1 0 0\nv 0 1 0\nf 1 2 3\n") == 0);
    REQUIRE(obj.find("o second\nv 2 0 0\nv 3 0 0\nv 2 1 0\nf 4 5 6\n") != std::string::npos);
}