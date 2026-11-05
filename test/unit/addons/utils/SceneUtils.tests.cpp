#include "utils/SceneUtils.h"

#include "geometries/BoxGeometry.h"
#include "materials/MeshBasicMaterial.h"
#include "objects/Group.h"
#include "objects/InstancedMesh.h"
#include "objects/Mesh.h"

#include "catch.hpp"

#include <memory>
#include <vector>

TEST_CASE("SceneUtils expands an InstancedMesh into a Group of Meshes") {
    auto geometry = std::make_shared<BoxGeometry>();
    auto material = std::make_shared<MeshBasicMaterial>();
    auto instanced = std::make_shared<InstancedMesh>(geometry, material, JSValue(3));
    Matrix4 matrix;
    matrix.makeTranslation(1, 2, 3);
    instanced->setMatrixAt(0, matrix);
    matrix.makeTranslation(4, 5, 6);
    instanced->setMatrixAt(1, matrix);
    matrix.makeTranslation(7, 8, 9);
    instanced->setMatrixAt(2, matrix);

    const auto group = SceneUtils::createMeshesFromInstancedMesh(instanced);
    // The pinned helper copies the instanced mesh properties onto the group and then fills it.
    REQUIRE(group->children.size() >= 3);
    const auto first = std::dynamic_pointer_cast<Mesh>(group->children[0]);
    REQUIRE(first != nullptr);
    REQUIRE(first->position.x == Approx(1));
    REQUIRE(first->position.y == Approx(2));
    REQUIRE(first->position.z == Approx(3));
}

TEST_CASE("SceneUtils builds one mesh per material") {
    auto geometry = std::make_shared<BoxGeometry>();
    std::vector<std::shared_ptr<Material>> materials{std::make_shared<MeshBasicMaterial>(),
                                                    std::make_shared<MeshBasicMaterial>()};
    const auto group = SceneUtils::createMultiMaterialObject(geometry, materials);
    REQUIRE(group->children.size() == 2);
    const auto first = std::dynamic_pointer_cast<Mesh>(group->children[0]);
    const auto second = std::dynamic_pointer_cast<Mesh>(group->children[1]);
    REQUIRE(first->geometry == geometry);
    REQUIRE(second->geometry == geometry);
    REQUIRE(first->material == materials[0]);
    REQUIRE(second->material == materials[1]);
}

TEST_CASE("SceneUtils reduceVertices folds every visible vertex") {
    auto mesh = std::make_shared<Mesh>(std::make_shared<BoxGeometry>(),
                                       std::make_shared<MeshBasicMaterial>());
    auto scene = std::make_shared<Object3D>();
    scene->add(mesh);

    // Count the vertices and sum their y coordinates after the world matrix is applied.
    const double count = SceneUtils::reduceVertices(
        scene, [](double value, const Vector3&) { return value + 1; }, 0);
    REQUIRE(count == mesh->geometry->getAttribute("position")->getCount());
    const double sumY = SceneUtils::reduceVertices(
        scene,
        [](double value, const Vector3& vertex) { return value + vertex.y; }, 0);
    // A unit box has vertices at y = +/- 0.5, so the sum cancels out.
    REQUIRE(sumY == Approx(0));
}

TEST_CASE("SceneUtils sorts instanced payloads by the comparator") {
    auto geometry = std::make_shared<BoxGeometry>();
    auto material = std::make_shared<MeshBasicMaterial>();
    auto instanced = std::make_shared<InstancedMesh>(geometry, material, JSValue(3));
    Matrix4 matrix;
    matrix.makeTranslation(3, 0, 0);
    instanced->setMatrixAt(0, matrix);
    matrix.makeTranslation(1, 0, 0);
    instanced->setMatrixAt(1, matrix);
    matrix.makeTranslation(2, 0, 0);
    instanced->setMatrixAt(2, matrix);

    auto positionAt = [](double index, const std::shared_ptr<InstancedMesh>& mesh) {
        Matrix4 current;
        mesh->getMatrixAt(index, current);
        return current.elements()[12];
    };
    // Sort ascending by the instance translation x, which is stored in the last column.
    SceneUtils::sortInstancedMesh(instanced, [&](double a, double b) {
        return positionAt(a, instanced) < positionAt(b, instanced);
    });
    REQUIRE(positionAt(0, instanced) == Approx(1));
    REQUIRE(positionAt(1, instanced) == Approx(2));
    REQUIRE(positionAt(2, instanced) == Approx(3));
}