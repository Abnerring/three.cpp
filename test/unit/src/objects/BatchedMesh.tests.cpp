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