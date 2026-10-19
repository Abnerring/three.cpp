#include "catch.hpp"
#include "core/BufferAttribute.h"
#include "core/BufferGeometry.h"
#include "objects/BatchedMesh.h"
#include <memory>
#include <stdexcept>
#include <vector>

// The data core of a batched mesh: the counters, the geometry it builds once from a reference, and the
// validation that keeps every geometry in a batch compatible with the first one.
TEST_CASE("BatchedMesh initializes its batch geometry from a reference") {
    auto batch = std::make_shared<BatchedMesh>(4, 64);
    REQUIRE(batch->getMaxInstanceCount() == 4.0);
    REQUIRE(batch->getInstanceCount() == 0.0);
    REQUIRE(batch->getUnusedVertexCount() == 64.0);
    REQUIRE(batch->getUnusedIndexCount() == 128.0);   // the index default is twice the vertex budget

    auto reference = std::make_shared<BufferGeometry>();
    reference->setAttribute("position", std::make_shared<Float32BufferAttribute>(std::vector<float>(9, 0.0f), 3));
    reference->setIndex(std::make_shared<Uint16BufferAttribute>(std::vector<std::uint16_t>{0, 1, 2}, 1));

    batch->initializeGeometry(reference);
    const auto position = std::dynamic_pointer_cast<BufferAttribute>(batch->geometry->getAttribute("position"));
    REQUIRE(position);
    REQUIRE(position->count == 64);      // sized from the vertex budget, not from the reference
    REQUIRE(position->itemSize == 3);
    REQUIRE(batch->geometry->getIndex() != nullptr);
    REQUIRE(batch->geometry->getIndex()->count == 128);
}

TEST_CASE("BatchedMesh rejects a geometry that does not match the batch") {
    auto batch = std::make_shared<BatchedMesh>(2, 8);
    auto reference = std::make_shared<BufferGeometry>();
    reference->setAttribute("position", std::make_shared<Float32BufferAttribute>(std::vector<float>(9, 0.0f), 3));
    batch->initializeGeometry(reference);

    // missing attribute
    auto missing = std::make_shared<BufferGeometry>();
    REQUIRE_THROWS_AS(batch->validateGeometry(missing), std::runtime_error);

    // inconsistent index presence
    auto indexed = std::make_shared<BufferGeometry>();
    indexed->setAttribute("position", std::make_shared<Float32BufferAttribute>(std::vector<float>(9, 0.0f), 3));
    indexed->setIndex(std::make_shared<Uint16BufferAttribute>(std::vector<std::uint16_t>{0, 1, 2}, 1));
    REQUIRE_THROWS_AS(batch->validateGeometry(indexed), std::runtime_error);

    // inconsistent item size
    auto wrongSize = std::make_shared<BufferGeometry>();
    wrongSize->setAttribute("position", std::make_shared<Float32BufferAttribute>(std::vector<float>(8, 0.0f), 2));
    REQUIRE_THROWS_AS(batch->validateGeometry(wrongSize), std::runtime_error);

    // a matching one passes
    auto matching = std::make_shared<BufferGeometry>();
    matching->setAttribute("position", std::make_shared<Float32BufferAttribute>(std::vector<float>(9, 0.0f), 3));
    REQUIRE_NOTHROW(batch->validateGeometry(matching));
}
// Adding a geometry reserves a range, copies its vertex data into the batch, rebases its index on the vertex
// range it occupies, and zero fills whatever it reserved but did not use.
TEST_CASE("BatchedMesh adds geometries and rebases their indices") {
    auto batch = std::make_shared<BatchedMesh>(4, 8);
    auto first = std::make_shared<BufferGeometry>();
    first->setAttribute("position", std::make_shared<Float32BufferAttribute>(
        std::vector<float>{1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f}, 3));
    first->setIndex(std::make_shared<Uint16BufferAttribute>(std::vector<std::uint16_t>{0, 1, 2}, 1));
    REQUIRE(batch->addGeometry(first) == 0.0);

    auto second = std::make_shared<BufferGeometry>();
    second->setAttribute("position", std::make_shared<Float32BufferAttribute>(
        std::vector<float>{10.0f, 11.0f, 12.0f, 13.0f, 14.0f, 15.0f, 16.0f, 17.0f, 18.0f}, 3));
    second->setIndex(std::make_shared<Uint16BufferAttribute>(std::vector<std::uint16_t>{2, 1, 0}, 1));
    REQUIRE(batch->addGeometry(second) == 1.0);

    REQUIRE(batch->getUnusedVertexCount() == 2.0);      // eight vertices, six used
    REQUIRE(batch->getUnusedIndexCount() == 10.0);      // sixteen indices, six used

    const auto positions = std::dynamic_pointer_cast<BufferAttribute>(batch->geometry->getAttribute("position"));
    REQUIRE(positions->getComponent(0.0, 0.0) == 1.0);
    REQUIRE(positions->getComponent(3.0, 0.0) == 10.0);  // the second geometry starts at vertex three
    REQUIRE(positions->getComponent(4.0, 2.0) == 15.0);

    const auto indices = batch->geometry->getIndex();
    REQUIRE(indices->getX(0.0) == 0.0);
    REQUIRE(indices->getX(3.0) == 5.0);                  // 3 + 2, rebased on the second vertex range
    REQUIRE(indices->getX(5.0) == 3.0);                  // 3 + 0
}

TEST_CASE("BatchedMesh zero fills reserved space and refuses one that does not fit") {
    auto batch = std::make_shared<BatchedMesh>(2, 8);
    auto geometry = std::make_shared<BufferGeometry>();
    geometry->setAttribute("position", std::make_shared<Float32BufferAttribute>(
        std::vector<float>{1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f}, 3));   // two vertices
    geometry->setIndex(std::make_shared<Uint16BufferAttribute>(std::vector<std::uint16_t>{0, 1, 0}, 1));
    // Reserve five vertices, so three of them must come back as zeroes.
    REQUIRE(batch->addGeometry(geometry, 5.0) == 0.0);
    const auto positions = std::dynamic_pointer_cast<BufferAttribute>(batch->geometry->getAttribute("position"));
    REQUIRE(positions->getComponent(2.0, 0.0) == 0.0);
    REQUIRE(positions->getComponent(4.0, 2.0) == 0.0);
    REQUIRE(batch->getUnusedVertexCount() == 3.0);

    // A reservation larger than the remaining buffer is refused rather than silently truncated.
    auto large = std::make_shared<BufferGeometry>();
    large->setAttribute("position", std::make_shared<Float32BufferAttribute>(std::vector<float>(30, 1.0f), 3));
    large->setIndex(std::make_shared<Uint16BufferAttribute>(std::vector<std::uint16_t>{0, 1, 2}, 1));
    REQUIRE_THROWS_AS(batch->addGeometry(large), std::runtime_error);
}
// Instances name a geometry, can be hidden, can be repointed, and are limited by the max instance count that
// the batch was constructed with.
TEST_CASE("BatchedMesh manages its instances") {
    auto batch = std::make_shared<BatchedMesh>(2, 8);
    auto geometry = std::make_shared<BufferGeometry>();
    geometry->setAttribute("position", std::make_shared<Float32BufferAttribute>(
        std::vector<float>{1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f}, 3));
    geometry->setIndex(std::make_shared<Uint16BufferAttribute>(std::vector<std::uint16_t>{0, 1, 2}, 1));
    REQUIRE(batch->addGeometry(geometry) == 0.0);

    REQUIRE(batch->addInstance(0.0) == 0.0);
    REQUIRE(batch->addInstance(0.0) == 1.0);
    REQUIRE(batch->getInstanceCount() == 2.0);
    // The batch is at its limit and nothing has been freed, so one more is refused.
    REQUIRE_THROWS_AS(batch->addInstance(0.0), std::runtime_error);

    REQUIRE(batch->getVisibleAt(0.0));
    batch->setVisibleAt(0.0, false);
    REQUIRE(batch->getVisibleAt(0.0) == false);
    REQUIRE(batch->getVisibleAt(1.0));

    REQUIRE(batch->getGeometryIdAt(1.0) == 0.0);
    batch->setGeometryIdAt(1.0, 0.0);
    REQUIRE(batch->getGeometryIdAt(1.0) == 0.0);

    const auto range = batch->getGeometryRangeAt(0.0);
    REQUIRE(range.vertexStart == 0.0);
    REQUIRE(range.reservedVertexCount == 3.0);
    REQUIRE(range.indexStart == 0.0);
    REQUIRE(range.reservedIndexCount == 3.0);
    REQUIRE(range.start == 0.0);
    REQUIRE(range.count == 3.0);

    // Ids that were never handed out are rejected rather than read past the records.
    REQUIRE_THROWS_AS(batch->getVisibleAt(7.0), std::runtime_error);
    REQUIRE_THROWS_AS(batch->getGeometryRangeAt(3.0), std::runtime_error);
}
// Per instance matrices and colours round trip, and deleting frees ids for reuse while taking the instances
// that pointed at a deleted geometry with it.
TEST_CASE("BatchedMesh stores per instance matrices and colours") {
    auto batch = std::make_shared<BatchedMesh>(2, 8);
    auto geometry = std::make_shared<BufferGeometry>();
    geometry->setAttribute("position", std::make_shared<Float32BufferAttribute>(
        std::vector<float>{1.0f, 2.0f, 3.0f}, 3));
    geometry->setIndex(std::make_shared<Uint16BufferAttribute>(std::vector<std::uint16_t>{0, 0, 0}, 1));
    batch->addGeometry(geometry);
    batch->addInstance(0.0);

    // A fresh instance is identity and white.
    REQUIRE(batch->getMatrixAt(0.0).elements()[12] == 0.0);
    REQUIRE(batch->getColorAt(0.0).r == 1.0);

    // Matrix4.set is the upstream one, which writes te[12] from its fourth argument, so a translation of
    // (4, 5, 6) sits in the fourth slot of each group rather than at the start of the last group.
    Matrix4 moved;
    moved.set(1, 0, 0, 4, 0, 1, 0, 5, 0, 0, 1, 6, 0, 0, 0, 1);
    REQUIRE(moved.elements()[12] == 4.0);
    batch->setMatrixAt(0.0, moved);
    REQUIRE(batch->getMatrixAt(0.0).elements()[12] == 4.0);
    REQUIRE(batch->getMatrixAt(0.0).elements()[14] == 6.0);

    batch->setColorAt(0.0, Color(0.25, 0.5, 0.75));
    REQUIRE(batch->getColorAt(0.0).g == 0.5);
}

TEST_CASE("BatchedMesh reuses ids freed by delete") {
    auto batch = std::make_shared<BatchedMesh>(3, 16);
    auto geometry = std::make_shared<BufferGeometry>();
    geometry->setAttribute("position", std::make_shared<Float32BufferAttribute>(
        std::vector<float>{1.0f, 2.0f, 3.0f}, 3));
    geometry->setIndex(std::make_shared<Uint16BufferAttribute>(std::vector<std::uint16_t>{0, 0, 0}, 1));
    const auto geometryId = batch->addGeometry(geometry);
    REQUIRE(geometryId == 0.0);

    batch->addInstance(0.0);
    batch->addInstance(0.0);
    batch->addInstance(0.0);
    batch->deleteInstance(1.0);
    // The freed record is inactive, so reading it is refused rather than answering stale data.
    REQUIRE_THROWS_AS(batch->getVisibleAt(1.0), std::runtime_error);
    // The smallest freed id is handed out again.
    REQUIRE(batch->addInstance(0.0) == 1.0);

    // Deleting a geometry takes its instances with it and frees the geometry id.
    batch->deleteGeometry(0.0);
    REQUIRE_THROWS_AS(batch->getGeometryRangeAt(0.0), std::runtime_error);
    REQUIRE(batch->getInstanceCount() == 0.0);
    // Deleting it again is quietly ignored, which is the upstream asymmetry against deleteInstance.
    REQUIRE_NOTHROW(batch->deleteGeometry(0.0));
}