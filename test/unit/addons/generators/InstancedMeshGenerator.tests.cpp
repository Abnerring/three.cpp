#include "generators/city/InstancedMeshGenerator.h"

#include "geometries/BoxGeometry.h"
#include "materials/MeshBasicMaterial.h"

#include "catch.hpp"

#include <memory>
#include <vector>

namespace {
std::vector<Matrix4> makePlacements() {
    std::vector<Matrix4> placements;
    Matrix4 first;
    first.makeTranslation(1, 0, 0);
    Matrix4 second;
    second.makeTranslation(2, 0, 0);
    placements.push_back(first);
    placements.push_back(second);
    return placements;
}
}  // namespace

TEST_CASE("createInstances allocates the next power of two capacity") {
    auto geometry = std::make_shared<BoxGeometry>();
    auto material = std::make_shared<MeshBasicMaterial>();
    const auto mesh = createInstances(geometry, material, 3, "instances");
    REQUIRE(mesh->name == "instances");
    REQUIRE(mesh->castShadow == true);
    REQUIRE(mesh->receiveShadow == true);
    // ceilPowerOfTwo( 3 ) is 4, which is the instance attribute count.
    REQUIRE(mesh->instanceMatrix->getCount() == Approx(4));
}

TEST_CASE("updateInstances writes the count, the visibility and the matrices") {
    auto mesh = createInstances(std::make_shared<BoxGeometry>(),
                                std::make_shared<MeshBasicMaterial>(), 2, "instances");
    const auto placements = makePlacements();
    updateInstances(mesh, placements);
    REQUIRE(mesh->count == Approx(2));
    REQUIRE(mesh->visible == true);
    Matrix4 matrix;
    mesh->getMatrixAt(0, matrix);
    REQUIRE(matrix.elements()[12] == Approx(1));
    mesh->getMatrixAt(1, matrix);
    REQUIRE(matrix.elements()[12] == Approx(2));
    // The bounding sphere is recomputed by the helper; its radius depends on the geometry data, so the
    // exact values are not asserted here (the needs update flag is consumed by the renderer).

    updateInstances(mesh, {});
    REQUIRE(mesh->count == Approx(0));
    REQUIRE(mesh->visible == false);
}

TEST_CASE("InstancedMeshGenerator builds once and rebuilds when the parameters change") {
    auto createGeometry = [](const JsonValue&) { return std::make_shared<BoxGeometry>(); };
    auto createMaterial = [] { return std::make_shared<MeshBasicMaterial>(); };
    InstancedMeshGenerator generator(JsonValue(), createGeometry, createMaterial, "city");
    REQUIRE(generator.receiveShadow == true);

    const auto placements = makePlacements();
    const auto firstMesh = generator.build(placements);
    REQUIRE(firstMesh != nullptr);
    REQUIRE(generator.geometry != nullptr);
    REQUIRE(generator.material != nullptr);
    REQUIRE(firstMesh->count == Approx(2));

    // Same parameters and same placement count: the mesh is reused.
    const auto secondMesh = generator.build(placements);
    REQUIRE(secondMesh == firstMesh);

    // A different parameter set disposes the previous resources and rebuilds.
    generator.parameters["size"] = JsonValue(2);
    const auto thirdMesh = generator.build(placements);
    REQUIRE(thirdMesh != firstMesh);

    generator.dispose();
    REQUIRE(generator.geometry == nullptr);
    REQUIRE(generator.material == nullptr);
    REQUIRE(generator.mesh == nullptr);
}

TEST_CASE("InstancedMeshGenerator rebuilds when the placements outgrow the allocated capacity") {
    auto createGeometry = [](const JsonValue&) { return std::make_shared<BoxGeometry>(); };
    auto createMaterial = [] { return std::make_shared<MeshBasicMaterial>(); };
    InstancedMeshGenerator generator(JsonValue(), createGeometry, createMaterial, "city");

    std::vector<Matrix4> twoPlacements;
    for (int i = 0; i < 2; ++i) {
        Matrix4 matrix;
        matrix.makeTranslation(i, 0, 0);
        twoPlacements.push_back(matrix);
    }
    // Two placements allocate a capacity of two.
    const auto firstMesh = generator.build(twoPlacements);
    REQUIRE(firstMesh->instanceMatrix->getCount() == Approx(2));
    REQUIRE(firstMesh->count == Approx(2));

    // A third placement no longer fits, so the mesh is rebuilt with the next power of two.
    std::vector<Matrix4> threePlacements = twoPlacements;
    Matrix4 extra;
    extra.makeTranslation(2, 0, 0);
    threePlacements.push_back(extra);
    const auto secondMesh = generator.build(threePlacements);
    REQUIRE(secondMesh != firstMesh);
    REQUIRE(secondMesh->instanceMatrix->getCount() == Approx(4));
    REQUIRE(secondMesh->count == Approx(3));
    REQUIRE(secondMesh->visible == true);
}