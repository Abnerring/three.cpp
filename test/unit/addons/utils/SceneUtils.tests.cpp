#include "utils/SceneUtils.h"

#include "geometries/BoxGeometry.h"
#include "core/InstancedBufferAttribute.h"
#include "math/Color.h"
#include "materials/MeshBasicMaterial.h"
#include "core/BufferAttribute.h"
#include "core/BufferGeometry.h"
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
        return positionAt(a, instanced) - positionAt(b, instanced);
    });
    REQUIRE(positionAt(0, instanced) == Approx(1));
    REQUIRE(positionAt(1, instanced) == Approx(2));
    REQUIRE(positionAt(2, instanced) == Approx(3));
}
TEST_CASE("SceneUtils sortInstancedMesh carries the instance colour and attribute payloads") {
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
    instanced->setColorAt(0, Color(1, 0, 0));
    instanced->setColorAt(1, Color(0, 1, 0));
    instanced->setColorAt(2, Color(0, 0, 1));
    auto values = std::make_shared<InstancedBufferAttribute>(std::vector<float>{30, 10, 20}, 1);
    instanced->geometry->setAttribute("instanceValue", values);

    SceneUtils::sortInstancedMesh(instanced, [&](double a, double b) {
        Matrix4 left;
        Matrix4 right;
        instanced->getMatrixAt(a, left);
        instanced->getMatrixAt(b, right);
        return left.elements()[12] - right.elements()[12];
    });

    // Ascending by translation x: the green instance (x = 1) comes first, then blue (2), then red (3).
    Color first;
    Color second;
    Color third;
    instanced->getColorAt(0, first);
    instanced->getColorAt(1, second);
    instanced->getColorAt(2, third);
    REQUIRE(first.g == Approx(1));
    REQUIRE(first.r == Approx(0));
    REQUIRE(second.b == Approx(1));
    REQUIRE(third.r == Approx(1));
    REQUIRE(values->getX(0) == Approx(10));
    REQUIRE(values->getX(1) == Approx(20));
    REQUIRE(values->getX(2) == Approx(30));
}
TEST_CASE("SceneUtils splits a multi material mesh by geometry group") {
    auto geometry = std::make_shared<BufferGeometry>();
    geometry->setAttribute("position", std::make_shared<BufferAttribute>(
                                           std::vector<float>{0, 0, 0, 1, 0, 0, 0, 1, 0,
                                                              2, 0, 0, 3, 0, 0, 2, 1, 0}, 3));
    geometry->addGroup(0, 3, 0);
    geometry->addGroup(3, 3, 1);
    auto firstMaterial = std::make_shared<MeshBasicMaterial>();
    auto secondMaterial = std::make_shared<MeshBasicMaterial>();
    auto mesh = std::make_shared<Mesh>(geometry,
                                       std::vector<std::shared_ptr<Material>>{firstMaterial,
                                                                              secondMaterial});
    REQUIRE(mesh->materialIsArray);

    const auto result = SceneUtils::createMeshesFromMultiMaterialMesh(mesh);
    const auto group = std::dynamic_pointer_cast<Group>(result);
    REQUIRE(group != nullptr);
    REQUIRE(group->children.size() == 2);
    const auto first = std::dynamic_pointer_cast<Mesh>(group->children[0]);
    const auto second = std::dynamic_pointer_cast<Mesh>(group->children[1]);
    REQUIRE(first != nullptr);
    REQUIRE(second != nullptr);
    REQUIRE(first->material == firstMaterial);
    REQUIRE(second->material == secondMaterial);
    // Each group becomes its own three vertex geometry.
    REQUIRE(first->geometry->getAttribute("position")->getCount() == Approx(3));
    REQUIRE(second->geometry->getAttribute("position")->getCount() == Approx(3));
    REQUIRE(first->geometry->getAttribute("position")->getX(0) == Approx(0));
    REQUIRE(second->geometry->getAttribute("position")->getX(0) == Approx(2));
}

TEST_CASE("SceneUtils returns a single material mesh unchanged") {
    auto mesh = std::make_shared<Mesh>(std::make_shared<BoxGeometry>(),
                                       std::make_shared<MeshBasicMaterial>());
    const auto result = SceneUtils::createMeshesFromMultiMaterialMesh(mesh);
    REQUIRE(result == mesh);
}
TEST_CASE("SceneUtils split copies the uv and normal attributes per group") {
    auto geometry = std::make_shared<BufferGeometry>();
    geometry->setAttribute("position", std::make_shared<BufferAttribute>(
                                           std::vector<float>{0, 0, 0, 1, 0, 0, 0, 1, 0,
                                                              2, 0, 0, 3, 0, 0, 2, 1, 0}, 3));
    geometry->setAttribute("normal", std::make_shared<BufferAttribute>(
                                         std::vector<float>{0, 0, 1, 0, 0, 1, 0, 0, 1,
                                                            1, 0, 0, 1, 0, 0, 1, 0, 0}, 3));
    geometry->setAttribute("uv", std::make_shared<BufferAttribute>(
                                     std::vector<float>{0, 0, 1, 0, 0, 1, 0.5, 0.5, 1, 1, 0, 1}, 2));
    geometry->addGroup(0, 3, 0);
    geometry->addGroup(3, 3, 1);
    auto mesh = std::make_shared<Mesh>(geometry,
                                       std::vector<std::shared_ptr<Material>>{
                                           std::make_shared<MeshBasicMaterial>(),
                                           std::make_shared<MeshBasicMaterial>()});

    const auto result = SceneUtils::createMeshesFromMultiMaterialMesh(mesh);
    const auto group = std::dynamic_pointer_cast<Group>(result);
    REQUIRE(group != nullptr);
    REQUIRE(group->children.size() == 2);
    const auto first = std::dynamic_pointer_cast<Mesh>(group->children[0]);
    const auto second = std::dynamic_pointer_cast<Mesh>(group->children[1]);
    // Every attribute travels with its group, keeping the group length.
    REQUIRE(first->geometry->getAttribute("normal")->getCount() == Approx(3));
    REQUIRE(first->geometry->getAttribute("uv")->getCount() == Approx(3));
    REQUIRE(second->geometry->getAttribute("normal")->getCount() == Approx(3));
    REQUIRE(second->geometry->getAttribute("uv")->getCount() == Approx(3));
    // The second group keeps the values of its own source vertices.
    REQUIRE(second->geometry->getAttribute("normal")->getX(0) == Approx(1));
    REQUIRE(second->geometry->getAttribute("uv")->getX(0) == Approx(0.5));
    REQUIRE(second->geometry->getAttribute("uv")->getY(0) == Approx(0.5));
}