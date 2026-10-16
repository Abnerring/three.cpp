#include "catch.hpp"
#include "core/BufferAttribute.h"
#include "core/BufferGeometry.h"
#include "core/InstancedBufferAttribute.h"
#include "core/InterleavedBuffer.h"
#include "core/InterleavedBufferAttribute.h"
#include "core/TypedArray.h"
#include "objects/Bone.h"
#include "objects/Mesh.h"
#include "objects/Skeleton.h"
#include "objects/SkinnedMesh.h"
#include "utils/BufferGeometryUtils.h"
#include <cmath>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

namespace {
using BufferGeometryUtils::AttributePtr;
using BufferGeometryUtils::GeometryPtr;

// Captures the messages the failure paths write to std::cerr.
struct CaptureStderr {
    std::ostringstream messages;
    std::streambuf* previous;
    CaptureStderr() : previous(std::cerr.rdbuf(messages.rdbuf())) {}
    ~CaptureStderr() { std::cerr.rdbuf(previous); }
    std::string text() const { return messages.str(); }
};

std::vector<double> valuesOf(const AttributePtr& attribute) {
    const auto array = attribute->getArray();
    std::vector<double> result;
    for (double i = 0, length = double(array.size()); i < length; ++i)
        result.push_back(array.get(i).toNumber());
    return result;
}

std::vector<double> indexValues(const BufferGeometry& geometry) {
    std::vector<double> result;
    const auto index = geometry.getIndex();
    if (!index) return result;
    for (double i = 0; i < index->count; ++i) result.push_back(index->getX(i));
    return result;
}

// A single triangle with a normal, optionally indexed.
GeometryPtr triangle(bool indexed) {
    auto geometry = std::make_shared<BufferGeometry>();
    geometry->setAttribute("position", std::make_shared<Float32BufferAttribute>(
        std::vector<float>{0, 0, 0, 1, 0, 0, 0, 1, 0}, 3));
    geometry->setAttribute("normal", std::make_shared<Float32BufferAttribute>(
        std::vector<float>{0, 0, 1, 0, 0, 1, 0, 0, 1}, 3));
    if (indexed) geometry->setIndex(std::vector<std::uint32_t>{0, 1, 2});
    return geometry;
}

// Two interleaved attributes over one buffer: position at offset 0, normal at offset 3.
AttributePtr interleavedPair(InterleavedBuffer& buffer, double offset) {
    return std::make_shared<InterleavedBufferAttribute>(buffer.object(), JSValue(3.), JSValue(offset),
                                                        JSValue(false));
}
}

TEST_CASE("BufferGeometryUtils mergeGeometries concatenates attributes, index and groups",
          "[buffergeometryutils]") {
    const auto first = triangle(true);
    const auto second = triangle(true);
    second->getAttribute("position")->setXYZ(0, 2., 0., 0.);

    const auto merged = BufferGeometryUtils::mergeGeometries({first, second});
    REQUIRE(merged != nullptr);
    CHECK(valuesOf(merged->getAttribute("position")) ==
          std::vector<double>{0, 0, 0, 1, 0, 0, 0, 1, 0, 2, 0, 0, 1, 0, 0, 0, 1, 0});
    CHECK(merged->getAttribute("position")->getCount() == 6);
    CHECK(indexValues(*merged) == std::vector<double>{0, 1, 2, 3, 4, 5});
    CHECK(merged->groups.empty());

    const auto grouped = BufferGeometryUtils::mergeGeometries({first, second}, true);
    REQUIRE(grouped != nullptr);
    REQUIRE(grouped->groups.size() == 2);
    CHECK(grouped->groups[0].start == 0);
    CHECK(grouped->groups[0].count == 3);
    CHECK(grouped->groups[0].materialIndex == 0);
    CHECK(grouped->groups[1].start == 3);
    CHECK(grouped->groups[1].count == 3);
    CHECK(grouped->groups[1].materialIndex == 1);

    // Non-indexed geometries merge their vertex streams and generate no index.
    const auto plain = BufferGeometryUtils::mergeGeometries({triangle(false), triangle(false)}, true);
    REQUIRE(plain != nullptr);
    CHECK(plain->getIndex() == nullptr);
    CHECK(plain->getAttribute("position")->getCount() == 6);
    REQUIRE(plain->groups.size() == 2);
    CHECK(plain->groups[0].count == 3);
}

TEST_CASE("BufferGeometryUtils mergeGeometries reports incompatible geometries and returns null",
          "[buffergeometryutils]") {
    const auto indexed = triangle(true);
    const auto plain = triangle(false);

    {
        CaptureStderr capture;
        CHECK(BufferGeometryUtils::mergeGeometries({indexed, plain}) == nullptr);
        CHECK(capture.text() ==
              "THREE.BufferGeometryUtils: .mergeGeometries() failed with geometry at index 1. All geometries "
              "must have compatible attributes; make sure index attribute exists among all geometries, or in "
              "none of them.\n");
    }
    {
        auto other = triangle(true);
        other->deleteAttribute("normal");
        CaptureStderr capture;
        CHECK(BufferGeometryUtils::mergeGeometries({indexed, other}) == nullptr);
        CHECK(capture.text() ==
              "THREE.BufferGeometryUtils: .mergeGeometries() failed with geometry at index 1. Make sure all "
              "geometries have the same number of attributes.\n");
    }
    {
        auto other = triangle(true);
        other->setAttribute("uv", std::make_shared<Float32BufferAttribute>(
            std::vector<float>{0, 0, 1, 0, 0, 1}, 2));
        CaptureStderr capture;
        CHECK(BufferGeometryUtils::mergeGeometries({indexed, other}) == nullptr);
        CHECK(capture.text() ==
              "THREE.BufferGeometryUtils: .mergeGeometries() failed with geometry at index 1. All geometries "
              "must have compatible attributes; make sure \"uv\" attribute exists among all geometries, or in "
              "none of them.\n");
    }
    {
        auto morphed = triangle(true);
        morphed->morphAttributes["position"].push_back(std::make_shared<Float32BufferAttribute>(
            std::vector<float>{0, 0, 1, 0, 0, 1, 0, 0, 1}, 3));
        auto other = triangle(true);
        other->morphTargetsRelative = true;
        CaptureStderr capture;
        CHECK(BufferGeometryUtils::mergeGeometries({morphed, other}) == nullptr);
        CHECK(capture.text() ==
              "THREE.BufferGeometryUtils: .mergeGeometries() failed with geometry at index 1. "
              ".morphTargetsRelative must be consistent throughout all geometries.\n");
    }
    {
        auto morphed = triangle(true);
        morphed->morphAttributes["position"].push_back(std::make_shared<Float32BufferAttribute>(
            std::vector<float>{0, 0, 1, 0, 0, 1, 0, 0, 1}, 3));
        auto other = triangle(true);
        other->morphAttributes["normal"].push_back(std::make_shared<Float32BufferAttribute>(
            std::vector<float>{0, 0, 1, 0, 0, 1, 0, 0, 1}, 3));
        CaptureStderr capture;
        CHECK(BufferGeometryUtils::mergeGeometries({morphed, other}) == nullptr);
        CHECK(capture.text() ==
              "THREE.BufferGeometryUtils: .mergeGeometries() failed with geometry at index 1.  "
              ".morphAttributes must be consistent throughout all geometries.\n");
    }
    {
        auto morphed = triangle(true);
        morphed->morphAttributes["position"].push_back(std::make_shared<Float32BufferAttribute>(
            std::vector<float>{0, 0, 1, 0, 0, 1, 0, 0, 1}, 3));
        auto other = triangle(true);
        other->morphAttributes["position"].push_back(std::make_shared<Float32BufferAttribute>(
            std::vector<float>{0, 0, 1, 0, 0, 1, 0, 0, 1}, 3));
        other->morphAttributes["position"].push_back(std::make_shared<Float32BufferAttribute>(
            std::vector<float>{0, 0, 1, 0, 0, 1, 0, 0, 1}, 3));
        CaptureStderr capture;
        CHECK(BufferGeometryUtils::mergeGeometries({morphed, other}) == nullptr);
        CHECK(capture.text() ==
              "THREE.BufferGeometryUtils: .mergeGeometries() failed with geometry at index 1. All geometries "
              "must have the same number of morph targets for the \"position\" morph attribute.\n");
    }
    {
        // The attribute merge itself fails on inconsistent array types.
        auto other = triangle(true);
        other->setAttribute("position", std::make_shared<Float64BufferAttribute>(
            std::vector<double>{0, 0, 0, 1, 0, 0, 0, 1, 0}, 3));
        CaptureStderr capture;
        CHECK(BufferGeometryUtils::mergeGeometries({indexed, other}) == nullptr);
        CHECK(capture.text() ==
              "THREE.BufferGeometryUtils: .mergeAttributes() failed. BufferAttribute.array must be of "
              "consistent array types across matching attributes.\n"
              "THREE.BufferGeometryUtils: .mergeGeometries() failed while trying to merge the position "
              "attribute.\n");
    }
    CHECK_THROWS_AS(BufferGeometryUtils::mergeGeometries({}), JSTypeError);
}

TEST_CASE("BufferGeometryUtils mergeGeometries merges morph attributes and groups",
          "[buffergeometryutils]") {
    const auto morphed = triangle(false);
    morphed->morphAttributes["position"].push_back(std::make_shared<Float32BufferAttribute>(
        std::vector<float>{0, 0, 1, 0, 0, 2, 0, 0, 3}, 3));
    const auto other = triangle(false);
    auto otherMorph = other->clone();
    otherMorph->morphAttributes["position"].push_back(std::make_shared<Float32BufferAttribute>(
        std::vector<float>{0, 0, 4, 0, 0, 5, 0, 0, 6}, 3));

    const auto merged = BufferGeometryUtils::mergeGeometries({morphed, otherMorph}, true);
    REQUIRE(merged != nullptr);
    const auto& morphs = merged->morphAttributes.at("position");
    REQUIRE(morphs.size() == 1);
    CHECK(valuesOf(morphs[0]) == std::vector<double>{0, 0, 1, 0, 0, 2, 0, 0, 3, 0, 0, 4, 0, 0, 5, 0, 0, 6});
    CHECK(merged->morphTargetsRelative == false);

    // An empty morph target list is skipped instead of producing an empty entry. Every geometry
    // still has to declare the same morph attribute, or the merge fails like upstream.
    auto empty = triangle(false);
    empty->morphAttributes["position"] = {};
    auto otherEmpty = triangle(false);
    otherEmpty->morphAttributes["position"] = {};
    const auto skipped = BufferGeometryUtils::mergeGeometries({empty, otherEmpty});
    REQUIRE(skipped != nullptr);
    CHECK(skipped->morphAttributes.empty());
    {
        CaptureStderr capture;
        CHECK(BufferGeometryUtils::mergeGeometries({empty, triangle(false)}) == nullptr);
        CHECK(capture.text() ==
              "THREE.BufferGeometryUtils: .mergeGeometries() failed with geometry at index 1. Make sure all "
              "geometries have the same number of morph attributes.\n");
    }
}

TEST_CASE("BufferGeometryUtils mergeAttributes copies arrays, interleaved components and gpuType",
          "[buffergeometryutils]") {
    const auto first = std::make_shared<Float32BufferAttribute>(std::vector<float>{1, 2, 3, 4}, 2);
    const auto second = std::make_shared<Float32BufferAttribute>(std::vector<float>{5, 6}, 2);
    first->gpuType = 1016;
    second->gpuType = 1016;

    const auto merged = BufferGeometryUtils::mergeAttributes({first, second});
    REQUIRE(merged != nullptr);
    REQUIRE(std::dynamic_pointer_cast<BufferAttribute>(merged) != nullptr);
    CHECK(valuesOf(merged) == std::vector<double>{1, 2, 3, 4, 5, 6});
    CHECK(merged->getItemSize() == 2);
    CHECK(merged->getNormalized() == false);
    CHECK(merged->getProperty("gpuType").toNumber() == 1016);

    // Interleaved input is copied component by component. An InterleavedBufferAttribute has no
    // gpuType, so a mixed list only matches when the plain attribute has none either.
    InterleavedBuffer buffer(Float32Array({1, 2, 3, 4, 5, 6, 7, 8}), 4);
    const auto interleaved = std::make_shared<InterleavedBufferAttribute>(buffer.object(), JSValue(3.),
                                                                         JSValue(1.), JSValue(false));
    const auto plain = std::make_shared<Float32BufferAttribute>(std::vector<float>{9, 10, 11}, 3);
    {
        CaptureStderr capture;
        CHECK(BufferGeometryUtils::mergeAttributes({interleaved, plain}) == nullptr);
        CHECK(capture.text() ==
              "THREE.BufferGeometryUtils: .mergeAttributes() failed. BufferAttribute.gpuType must be "
              "consistent across matching attributes.\n");
    }
    plain->gpuType = JSValue();
    const auto mixed = BufferGeometryUtils::mergeAttributes({interleaved, plain});
    REQUIRE(mixed != nullptr);
    CHECK(valuesOf(mixed) == std::vector<double>{2, 3, 4, 6, 7, 8, 9, 10, 11});
    // The source gpuType is undefined, so the assignment is skipped and the fresh attribute
    // keeps the FloatType default, exactly like `new BufferAttribute( ... )`.
    CHECK(mixed->getProperty("gpuType").toNumber() == 1015);

    CHECK_THROWS_AS(BufferGeometryUtils::mergeAttributes({}), JSTypeError);
}

TEST_CASE("BufferGeometryUtils mergeAttributes reports inconsistent properties", "[buffergeometryutils]") {
    const auto base = std::make_shared<Float32BufferAttribute>(std::vector<float>{1, 2, 3}, 3);
    {
        CaptureStderr capture;
        CHECK(BufferGeometryUtils::mergeAttributes(
                  {base, std::make_shared<Float64BufferAttribute>(std::vector<double>{1, 2, 3}, 3)}) == nullptr);
        CHECK(capture.text() ==
              "THREE.BufferGeometryUtils: .mergeAttributes() failed. BufferAttribute.array must be of "
              "consistent array types across matching attributes.\n");
    }
    {
        CaptureStderr capture;
        CHECK(BufferGeometryUtils::mergeAttributes({base, std::make_shared<Float32BufferAttribute>(
                  std::vector<float>{1, 2}, 2)}) == nullptr);
        CHECK(capture.text() ==
              "THREE.BufferGeometryUtils: .mergeAttributes() failed. BufferAttribute.itemSize must be "
              "consistent across matching attributes.\n");
    }
    {
        auto normalized = std::make_shared<Float32BufferAttribute>(std::vector<float>{1, 2, 3}, 3, true);
        CaptureStderr capture;
        CHECK(BufferGeometryUtils::mergeAttributes({base, normalized}) == nullptr);
        CHECK(capture.text() ==
              "THREE.BufferGeometryUtils: .mergeAttributes() failed. BufferAttribute.normalized must be "
              "consistent across matching attributes.\n");
    }
    {
        auto typed = std::make_shared<Float32BufferAttribute>(std::vector<float>{1, 2, 3}, 3);
        typed->gpuType = 1016;
        CaptureStderr capture;
        CHECK(BufferGeometryUtils::mergeAttributes({base, typed}) == nullptr);
        CHECK(capture.text() ==
              "THREE.BufferGeometryUtils: .mergeAttributes() failed. BufferAttribute.gpuType must be "
              "consistent across matching attributes.\n");
    }
}

TEST_CASE("BufferGeometryUtils deepCloneAttribute clones buffers and interleaved attributes",
          "[buffergeometryutils]") {
    const auto plain = std::make_shared<Float32BufferAttribute>(std::vector<float>{1, 2, 3, 4}, 2);
    plain->name = "position";
    const auto cloned = BufferGeometryUtils::deepCloneAttribute(plain);
    REQUIRE(cloned != nullptr);
    REQUIRE(std::dynamic_pointer_cast<Float16BufferAttribute>(cloned) == nullptr);
    REQUIRE(std::dynamic_pointer_cast<InstancedBufferAttribute>(cloned) == nullptr);
    CHECK(cloned->getProperty("name").toString() == "position");
    CHECK(valuesOf(cloned) == std::vector<double>{1, 2, 3, 4});
    // The copy owns independent storage.
    std::dynamic_pointer_cast<BufferAttribute>(cloned)->setX(0, 99.);
    CHECK(plain->getX(0) == 1);

    const auto instanced = std::make_shared<InstancedBufferAttribute>(
        Float32Array({1, 2, 3, 4}), 2, false, JSValue(3.));
    const auto clonedInstanced = BufferGeometryUtils::deepCloneAttribute(instanced);
    const auto instancedResult = std::dynamic_pointer_cast<InstancedBufferAttribute>(clonedInstanced);
    REQUIRE(instancedResult != nullptr);
    CHECK(instancedResult->meshPerAttribute.toNumber() == 3);
    CHECK(valuesOf(clonedInstanced) == std::vector<double>{1, 2, 3, 4});

    InterleavedBuffer buffer(Float32Array({1, 2, 3, 4, 5, 6}), 2);
    const auto interleaved = std::make_shared<InterleavedBufferAttribute>(buffer.object(), JSValue(2.),
                                                                         JSValue(0.), JSValue(false));
    const auto deinterleaved = BufferGeometryUtils::deepCloneAttribute(interleaved);
    REQUIRE(std::dynamic_pointer_cast<InterleavedBufferAttribute>(deinterleaved) == nullptr);
    CHECK(valuesOf(deinterleaved) == std::vector<double>{1, 2, 3, 4, 5, 6});
}

TEST_CASE("BufferGeometryUtils interleaveAttributes shares one buffer and deinterleaves back",
          "[buffergeometryutils]") {
    const auto position = std::make_shared<Float32BufferAttribute>(std::vector<float>{1, 2, 3, 4, 5, 6}, 3);
    const auto uv = std::make_shared<Float32BufferAttribute>(std::vector<float>{7, 8, 9, 10}, 2);

    const auto interleaved = BufferGeometryUtils::interleaveAttributes({position, uv});
    REQUIRE(interleaved.size() == 2);
    const auto first = std::dynamic_pointer_cast<InterleavedBufferAttribute>(interleaved[0]);
    const auto second = std::dynamic_pointer_cast<InterleavedBufferAttribute>(interleaved[1]);
    REQUIRE(first != nullptr);
    REQUIRE(second != nullptr);
    REQUIRE(first->data.strictEquals(second->data));
    CHECK(first->getProperty("offset").toNumber() == 0);
    CHECK(second->getProperty("offset").toNumber() == 3);
    CHECK(first->data.get("stride").toNumber() == 5);
    CHECK(first->data.get("count").toNumber() == 2);
    CHECK(first->getX(0) == 1);
    CHECK(first->getZ(1) == 6);
    CHECK(second->getX(0) == 7);
    CHECK(second->getY(1) == 10);

    // Round trip: deinterleaving returns the original component values.
    const auto restored = BufferGeometryUtils::deinterleaveAttribute(first);
    REQUIRE(std::dynamic_pointer_cast<BufferAttribute>(restored) != nullptr);
    CHECK(valuesOf(restored) == std::vector<double>{1, 2, 3, 4, 5, 6});
    CHECK(restored->getItemSize() == 3);

    {
        CaptureStderr capture;
        CHECK(BufferGeometryUtils::interleaveAttributes({position, std::make_shared<Int16BufferAttribute>(
                  std::vector<std::int16_t>{1, 2}, 1)}).empty());
        CHECK(capture.text() == "AttributeBuffers of different types cannot be interleaved\n");
    }
    CHECK_THROWS_AS(BufferGeometryUtils::interleaveAttributes({}), JSTypeError);
}

TEST_CASE("BufferGeometryUtils deinterleaveGeometry replaces every interleaved attribute",
          "[buffergeometryutils]") {
    InterleavedBuffer data(Float32Array({0, 0, 0, 1, 0, 0, 0, 1, 0, 1, 1, 0}), 4);
    auto geometry = std::make_shared<BufferGeometry>();
    const auto position = std::make_shared<InterleavedBufferAttribute>(data.object(), JSValue(3.),
                                                                      JSValue(0.), JSValue(false));
    const auto normal = std::make_shared<InterleavedBufferAttribute>(data.object(), JSValue(1.),
                                                                    JSValue(3.), JSValue(false));
    geometry->setAttribute("position", position);
    geometry->setAttribute("normal", normal);
    geometry->morphAttributes["position"] = {position};
    geometry->setAttribute("uv", std::make_shared<Float32BufferAttribute>(
        std::vector<float>{0, 0, 1, 0, 0, 1, 1, 1}, 2));

    BufferGeometryUtils::deinterleaveGeometry(*geometry);
    const auto deinterleaved = geometry->getAttribute("position");
    REQUIRE(std::dynamic_pointer_cast<InterleavedBufferAttribute>(deinterleaved) == nullptr);
    // Three stride-4 rows: one position triplet and one normal component per row.
    CHECK(valuesOf(deinterleaved) == std::vector<double>{0, 0, 0, 0, 0, 0, 0, 1, 1});
    CHECK(valuesOf(geometry->getAttribute("normal")) == std::vector<double>{1, 1, 0});
    // The morph attribute shared the same interleaved attribute object, so it shares one copy.
    CHECK(geometry->morphAttributes.at("position")[0] == deinterleaved);
    CHECK(std::dynamic_pointer_cast<BufferAttribute>(geometry->getAttribute("uv")) != nullptr);
}

TEST_CASE("BufferGeometryUtils estimateBytesUsed counts attributes and the index", "[buffergeometryutils]") {
    const auto geometry = triangle(true);
    CHECK(BufferGeometryUtils::estimateBytesUsed(*geometry) == 3 * 3 * 4 + 3 * 3 * 4 + 3 * 2);

    auto interleavedGeometry = std::make_shared<BufferGeometry>();
    InterleavedBuffer data(Float32Array({0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 1}), 4);
    interleavedGeometry->setAttribute("position", std::make_shared<InterleavedBufferAttribute>(
        data.object(), JSValue(3.), JSValue(0.), JSValue(false)));
    // itemSize * count * BYTES_PER_ELEMENT, not the whole shared backing array.
    CHECK(BufferGeometryUtils::estimateBytesUsed(*interleavedGeometry) == 3 * 3 * 4);

    const BufferGeometry empty;
    CHECK(BufferGeometryUtils::estimateBytesUsed(empty) == 0);
}

TEST_CASE("BufferGeometryUtils mergeVertices welds vertices with and without an index",
          "[buffergeometryutils]") {
    auto geometry = std::make_shared<BufferGeometry>();
    geometry->setAttribute("position", std::make_shared<Float32BufferAttribute>(
        std::vector<float>{0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0}, 3));
    geometry->setAttribute("uv", std::make_shared<Float32BufferAttribute>(
        std::vector<float>{0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1}, 2));

    const auto welded = BufferGeometryUtils::mergeVertices(geometry);
    REQUIRE(welded != nullptr);
    REQUIRE(welded->getAttribute("position") != nullptr);
    CHECK(welded->getAttribute("position")->getCount() == 3);
    CHECK(valuesOf(welded->getAttribute("position")) == std::vector<double>{0, 0, 0, 1, 0, 0, 0, 1, 0});
    CHECK(indexValues(*welded) == std::vector<double>{0, 1, 2, 0, 1, 2});
    CHECK(valuesOf(welded->getAttribute("uv")) == std::vector<double>{0, 0, 1, 0, 0, 1});

    // An indexed geometry keeps the welded indices and the tolerance controls the hash grid.
    auto indexed = triangle(true);
    indexed->setIndex(std::vector<std::uint32_t>{0, 1, 2, 0, 1, 2});
    const auto indexedWelded = BufferGeometryUtils::mergeVertices(indexed);
    REQUIRE(indexedWelded != nullptr);
    CHECK(indexValues(*indexedWelded) == std::vector<double>{0, 1, 2, 0, 1, 2});

    auto near = std::make_shared<BufferGeometry>();
    near->setAttribute("position", std::make_shared<Float32BufferAttribute>(
        std::vector<float>{0, 0, 0, 0.00004f, 0, 0, 0, 1, 0}, 3));
    CHECK(BufferGeometryUtils::mergeVertices(near, 1e-4)->getAttribute("position")->getCount() == 2);
    CHECK(BufferGeometryUtils::mergeVertices(near, 1e-5)->getAttribute("position")->getCount() == 3);

    CaptureStderr capture;
    CHECK_THROWS_AS(BufferGeometryUtils::mergeVertices(std::make_shared<BufferGeometry>()), JSTypeError);
    try {
        BufferGeometryUtils::mergeVertices(std::make_shared<BufferGeometry>());
    } catch (const std::exception& error) {
        CHECK(std::string(error.what()) == "Cannot read properties of undefined (reading 'count')");
    }
}

TEST_CASE("BufferGeometryUtils mergeVertices keeps one copied interleaved buffer per source",
          "[buffergeometryutils]") {
    // position and normal view the same interleaved buffer, so the remapped result must reuse
    // the one copied buffer instead of copying the stride twice.
    InterleavedBuffer data(Float32Array({0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 1, 0, 1, 0, 0, 0, 1,
                                         0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 1, 0, 1, 0, 0, 0, 1}), 6);
    auto geometry = std::make_shared<BufferGeometry>();
    geometry->setAttribute("position", interleavedPair(data, 0));
    geometry->setAttribute("normal", interleavedPair(data, 3));
    geometry->setIndex(std::vector<std::uint32_t>{0, 1, 2, 3, 4, 5});

    const auto welded = BufferGeometryUtils::mergeVertices(geometry);
    REQUIRE(welded != nullptr);
    const auto position = std::dynamic_pointer_cast<InterleavedBufferAttribute>(
        welded->getAttribute("position"));
    const auto normal = std::dynamic_pointer_cast<InterleavedBufferAttribute>(welded->getAttribute("normal"));
    REQUIRE(position != nullptr);
    REQUIRE(normal != nullptr);
    CHECK(position->data.strictEquals(normal->data));
    CHECK(position->data.get("count").toNumber() == 3);
    CHECK(indexValues(*welded) == std::vector<double>{0, 1, 2, 0, 1, 2});
    CHECK(position->getX(0) == 0);
    CHECK(position->getZ(1) == 0);
    CHECK(normal->getZ(0) == 1);
}

TEST_CASE("BufferGeometryUtils toTrianglesDrawMode converts fans and strips", "[buffergeometryutils]") {
    auto fan = std::make_shared<BufferGeometry>();
    fan->setAttribute("position", std::make_shared<Float32BufferAttribute>(
        std::vector<float>{0, 0, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, -1, 1, 0}, 3));
    fan->setIndex(std::vector<std::uint32_t>{0, 1, 2, 3, 4});
    fan->addGroup(0, 3, 0);

    const auto converted = BufferGeometryUtils::toTrianglesDrawMode(fan, TriangleFanDrawMode);
    CHECK(converted == fan);
    CHECK(indexValues(*fan) == std::vector<double>{0, 1, 2, 0, 2, 3, 0, 3, 4});
    CHECK(fan->groups.empty());

    auto strip = std::make_shared<BufferGeometry>();
    strip->setAttribute("position", std::make_shared<Float32BufferAttribute>(
        std::vector<float>{0, 0, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, -1, 1, 0, -1, 0, 0}, 3));
    strip->setIndex(std::vector<std::uint32_t>{0, 1, 2, 3, 4, 5});
    BufferGeometryUtils::toTrianglesDrawMode(strip, TriangleStripDrawMode);
    CHECK(indexValues(*strip) == std::vector<double>{0, 1, 2, 3, 2, 1, 2, 3, 4, 5, 4, 3});

    // A non-indexed fan generates its own index first.
    auto generated = std::make_shared<BufferGeometry>();
    generated->setAttribute("position", std::make_shared<Float32BufferAttribute>(
        std::vector<float>{0, 0, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0}, 3));
    BufferGeometryUtils::toTrianglesDrawMode(generated, TriangleFanDrawMode);
    CHECK(indexValues(*generated) == std::vector<double>{0, 1, 2, 0, 2, 3});
}

TEST_CASE("BufferGeometryUtils toTrianglesDrawMode keeps the geometry and logs known modes",
          "[buffergeometryutils]") {
    const auto geometry = triangle(true);
    {
        CaptureStderr capture;
        CHECK(BufferGeometryUtils::toTrianglesDrawMode(geometry, TrianglesDrawMode) == geometry);
        CHECK(capture.text() ==
              "THREE.BufferGeometryUtils.toTrianglesDrawMode(): Geometry already defined as triangles.\n");
    }
    {
        CaptureStderr capture;
        CHECK(BufferGeometryUtils::toTrianglesDrawMode(geometry, 5.) == geometry);
        CHECK(capture.text() ==
              "THREE.BufferGeometryUtils.toTrianglesDrawMode(): Unknown draw mode: 5\n");
    }
    {
        const auto withoutPosition = std::make_shared<BufferGeometry>();
        CaptureStderr capture;
        CHECK(BufferGeometryUtils::toTrianglesDrawMode(withoutPosition, TriangleFanDrawMode) == withoutPosition);
        CHECK(capture.text() ==
              "THREE.BufferGeometryUtils.toTrianglesDrawMode(): Undefined position attribute. Processing not "
              "possible.\n");
    }
    {
        // A short index cannot produce the expected number of triangles.
        auto shortIndex = triangle(false);
        shortIndex->setIndex(std::vector<std::uint32_t>{0, 1});
        CaptureStderr capture;
        BufferGeometryUtils::toTrianglesDrawMode(shortIndex, TriangleFanDrawMode);
        CHECK(capture.text() == std::string());
        CHECK(indexValues(*shortIndex).empty());
    }
}

TEST_CASE("BufferGeometryUtils mergeGroups sorts and merges by material index", "[buffergeometryutils]") {
    auto geometry = std::make_shared<BufferGeometry>();
    geometry->setAttribute("position", std::make_shared<Float32BufferAttribute>(
        std::vector<float>{0, 0, 0, 1, 0, 0, 0, 1, 0, 2, 0, 0, 3, 0, 0, 2, 1, 0, 4, 0, 0, 5, 0, 0, 4, 1, 0}, 3));
    geometry->setIndex(std::vector<std::uint32_t>{0, 1, 2, 3, 4, 5, 6, 7, 8});
    geometry->addGroup(6, 3, 2);
    geometry->addGroup(0, 3, 0);
    geometry->addGroup(3, 3, 0);

    const auto merged = BufferGeometryUtils::mergeGroups(geometry);
    CHECK(merged == geometry);
    CHECK(indexValues(*geometry) == std::vector<double>{0, 1, 2, 3, 4, 5, 6, 7, 8});
    REQUIRE(geometry->groups.size() == 2);
    CHECK(geometry->groups[0].start == 0);
    CHECK(geometry->groups[0].count == 6);
    CHECK(geometry->groups[0].materialIndex == 0);
    CHECK(geometry->groups[1].start == 6);
    CHECK(geometry->groups[1].count == 3);
    CHECK(geometry->groups[1].materialIndex == 2);

    {
        const auto plain = std::make_shared<BufferGeometry>();
        CaptureStderr capture;
        CHECK(BufferGeometryUtils::mergeGroups(plain) == plain);
        CHECK(capture.text() ==
              "THREE.BufferGeometryUtils.mergeGroups(): No groups are defined. Nothing to merge.\n");
    }
    {
        // A non-indexed geometry with groups generates a triangle index first.
        auto generated = std::make_shared<BufferGeometry>();
        generated->setAttribute("position", std::make_shared<Float32BufferAttribute>(
            std::vector<float>{0, 0, 0, 1, 0, 0, 0, 1, 0, 2, 0, 0, 3, 0, 0, 2, 1, 0}, 3));
        generated->addGroup(3, 3, 1);
        generated->addGroup(0, 3, 0);
        BufferGeometryUtils::mergeGroups(generated);
        CHECK(indexValues(*generated) == std::vector<double>{0, 1, 2, 3, 4, 5});
        REQUIRE(generated->groups.size() == 2);
        CHECK(generated->groups[0].materialIndex == 0);
        CHECK(generated->groups[1].materialIndex == 1);
    }
}

TEST_CASE("BufferGeometryUtils toCreasedNormals keeps creases and de-indexes", "[buffergeometryutils]") {
    // Two triangles meeting at right angles along the x axis.
    auto geometry = std::make_shared<BufferGeometry>();
    geometry->setAttribute("position", std::make_shared<Float32BufferAttribute>(
        std::vector<float>{0, 0, 0, 1, 0, 0, 1, 1, 0, 0, 0, 0, 1, 0, 0, 1, 0, 1}, 3));

    const auto creased = BufferGeometryUtils::toCreasedNormals(geometry);
    CHECK(creased == geometry);
    const auto creasedNormals = valuesOf(geometry->getAttribute("normal"));
    REQUIRE(creasedNormals.size() == 18);
    CHECK(std::abs(creasedNormals[2] - 1.) < 1e-6);
    CHECK(std::abs(creasedNormals[10] + 1.) < 1e-6);

    // A crease angle above the face angle averages the two face normals.
    auto smoothed = std::make_shared<BufferGeometry>();
    smoothed->setAttribute("position", std::make_shared<Float32BufferAttribute>(
        std::vector<float>{0, 0, 0, 1, 0, 0, 1, 1, 0, 0, 0, 0, 1, 0, 0, 1, 0, 1}, 3));
    BufferGeometryUtils::toCreasedNormals(smoothed, 2.);
    const auto smoothedNormals = valuesOf(smoothed->getAttribute("normal"));
    const double diagonal = 1. / std::sqrt(2.);
    CHECK(std::abs(smoothedNormals[1] + diagonal) < 1e-6);
    CHECK(std::abs(smoothedNormals[2] - diagonal) < 1e-6);

    // An indexed geometry is de-indexed into a new geometry that keeps the groups.
    const auto indexed = triangle(true);
    const auto result = BufferGeometryUtils::toCreasedNormals(indexed);
    CHECK(result != indexed);
    CHECK(result->getIndex() == nullptr);
    CHECK(result->getAttribute("position")->getCount() == 3);
    CHECK(std::abs(valuesOf(result->getAttribute("normal"))[2] - 1.) < 1e-6);

    // A position attribute that is not a three-component float buffer is flattened first.
    auto flattened = std::make_shared<BufferGeometry>();
    flattened->setAttribute("position", std::make_shared<Float32BufferAttribute>(
        std::vector<float>{0, 0, 0, 1, 0, 0, 1, 1, 0, 0, 0, 0, 1, 1, 0, 0, 1, 0}, 3, true));
    const auto flattenedResult = BufferGeometryUtils::toCreasedNormals(flattened);
    CHECK(flattenedResult->getAttribute("normal")->getCount() == 6);
}

TEST_CASE("BufferGeometryUtils computeMorphedAttributes applies influences", "[buffergeometryutils]") {
    auto geometry = std::make_shared<BufferGeometry>();
    geometry->setAttribute("position", std::make_shared<Float32BufferAttribute>(
        std::vector<float>{0, 0, 0, 1, 0, 0, 0, 1, 0}, 3));
    geometry->setAttribute("normal", std::make_shared<Float32BufferAttribute>(
        std::vector<float>{0, 0, 1, 0, 0, 1, 0, 0, 1}, 3));
    geometry->morphAttributes["position"].push_back(std::make_shared<Float32BufferAttribute>(
        std::vector<float>{0, 0, 1, 0, 0, 2, 0, 0, 3}, 3));

    auto mesh = std::make_shared<Mesh>(geometry, std::make_shared<MeshBasicMaterial>());
    mesh->morphTargetInfluences = SharedArray<double>(std::vector<double>{0.5});
    const auto morphed = BufferGeometryUtils::computeMorphedAttributes(mesh);
    CHECK(morphed.positionAttribute == geometry->getAttribute("position"));
    CHECK(morphed.normalAttribute == geometry->getAttribute("normal"));
    // morphTargetsRelative is false, so every target contributes ( morph - base ) * influence.
    CHECK(valuesOf(morphed.morphedPositionAttribute) ==
          std::vector<double>{0, 0, 0.5, 0.5, 0, 1, 0, 0.5, 1.5});
    CHECK(valuesOf(morphed.morphedNormalAttribute) == std::vector<double>{0, 0, 1, 0, 0, 1, 0, 0, 1});
    CHECK(morphed.morphedPositionAttribute->getItemSize() == 3);

    // An influence of zero and an absent morph attribute keep the original values.
    mesh->morphTargetInfluences = SharedArray<double>(std::vector<double>{0.});
    CHECK(valuesOf(BufferGeometryUtils::computeMorphedAttributes(mesh).morphedPositionAttribute) ==
          std::vector<double>{0, 0, 0, 1, 0, 0, 0, 1, 0});

    const auto plain = triangle(false);
    const auto withoutMorphs = BufferGeometryUtils::computeMorphedAttributes(
        std::make_shared<Mesh>(plain, std::make_shared<MeshBasicMaterial>()));
    CHECK(valuesOf(withoutMorphs.morphedPositionAttribute) ==
          std::vector<double>{0, 0, 0, 1, 0, 0, 0, 1, 0});
    CHECK(withoutMorphs.morphedPositionAttribute != withoutMorphs.positionAttribute);
}

TEST_CASE("BufferGeometryUtils remapAttribute copies entries and shares interleaved buffers",
          "[buffergeometryutils]") {
    const auto source = std::make_shared<Float32BufferAttribute>(std::vector<float>{1, 2, 3, 4, 5, 6}, 3);
    source->name = "position";
    source->gpuType = 1016;
    const auto indices = std::make_shared<Float64BufferAttribute>(std::vector<double>{1, 0}, 1);
    std::unordered_map<const VertexAttribute*, std::unordered_map<double, double>> buffers;

    const auto remapped = BufferGeometryUtils::remapAttribute(source, indices, buffers);
    REQUIRE(remapped != nullptr);
    CHECK(valuesOf(remapped) == std::vector<double>{4, 5, 6, 1, 2, 3});
    CHECK(remapped->getProperty("name").toString() == "position");
    CHECK(remapped->getProperty("gpuType").toNumber() == 1016);
    CHECK(remapped->getProperty("usage").toNumber() == source->usage.toNumber());

    // Two attributes over one interleaved buffer share the single copied buffer of the pass.
    InterleavedBuffer data(Float32Array({1, 2, 3, 4, 5, 6, 7, 8, 9, 10}), 5);
    const auto position = std::make_shared<InterleavedBufferAttribute>(data.object(), JSValue(3.),
                                                                      JSValue(1.), JSValue(false));
    const auto uv = std::make_shared<InterleavedBufferAttribute>(data.object(), JSValue(1.),
                                                                JSValue(4.), JSValue(false));
    const auto pass = std::make_shared<Float64BufferAttribute>(std::vector<double>{1, 0}, 1);
    std::unordered_map<const VertexAttribute*, std::unordered_map<double, double>> shared;
    const auto remappedPosition = BufferGeometryUtils::remapAttribute(position, pass, shared);
    const auto remappedUv = BufferGeometryUtils::remapAttribute(uv, pass, shared);
    const auto positionResult = std::dynamic_pointer_cast<InterleavedBufferAttribute>(remappedPosition);
    const auto uvResult = std::dynamic_pointer_cast<InterleavedBufferAttribute>(remappedUv);
    REQUIRE(positionResult != nullptr);
    REQUIRE(uvResult != nullptr);
    CHECK(positionResult->data.strictEquals(uvResult->data));
    CHECK(positionResult->data.get("count").toNumber() == 2);
    // Source rows are [1,2,3,4,5] and [6,7,8,9,10]; position reads offset 1, uv reads offset 4.
    CHECK(positionResult->getX(0) == 7);
    CHECK(positionResult->getX(1) == 2);
    CHECK(uvResult->getX(0) == 10);
    CHECK(uvResult->getX(1) == 5);

    // A missing source index reads undefined, which a Float32Array stores as NaN.
    const auto missing = std::make_shared<Float64BufferAttribute>(std::vector<double>{4}, 1);
    std::unordered_map<const VertexAttribute*, std::unordered_map<double, double>> extra;
    CHECK(std::isnan(valuesOf(BufferGeometryUtils::remapAttribute(source, missing, extra))[0]));
}

TEST_CASE("BufferGeometryUtils computeMorphedAttributes applies bone transforms",
          "[buffergeometryutils]") {
    auto geometry = std::make_shared<BufferGeometry>();
    geometry->setAttribute("position", std::make_shared<Float32BufferAttribute>(
        std::vector<float>{0, 0, 0, 1, 0, 0, 0, 1, 0}, 3));
    geometry->setAttribute("normal", std::make_shared<Float32BufferAttribute>(
        std::vector<float>{0, 0, 1, 0, 0, 1, 0, 0, 1}, 3));
    geometry->setAttribute("skinIndex", std::make_shared<Uint16BufferAttribute>(
        std::vector<std::uint16_t>(12, 0), 4));
    geometry->setAttribute("skinWeight", std::make_shared<Float32BufferAttribute>(
        std::vector<float>{1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0}, 4));

    auto mesh = std::make_shared<SkinnedMesh>(geometry);
    auto bone = std::make_shared<Bone>();
    mesh->add(bone);
    mesh->bind(std::make_shared<Skeleton>(Skeleton::Bones{bone}));
    bone->position.x = 2.;
    mesh->updateMatrixWorld(true);

    const auto morphed = BufferGeometryUtils::computeMorphedAttributes(mesh);
    CHECK(valuesOf(morphed.morphedPositionAttribute) == std::vector<double>{2, 0, 0, 3, 0, 0, 2, 1, 0});
    // The source applies applyBoneTransform to the normal as well, so the translation shows up
    // in the morphed normal too. The port keeps that behaviour.
    CHECK(valuesOf(morphed.morphedNormalAttribute) == std::vector<double>{2, 0, 1, 2, 0, 1, 2, 0, 1});
}

TEST_CASE("BufferGeometryUtils mergeVertices hashes with the JavaScript number text",
          "[buffergeometryutils]") {
    // The hash of mergeVertices concatenates `${ Math.trunc( ... ) }` values, so the port
    // depends on the JavaScript Number to String conversion.
    CHECK(JSValue(1e21).toString() == "1e+21");
    CHECK(JSValue(-0.).toString() == "0");
    CHECK(JSValue(std::numeric_limits<double>::quiet_NaN()).toString() == "NaN");
    CHECK(JSValue(std::numeric_limits<double>::infinity()).toString() == "Infinity");
    CHECK(JSValue(12345.).toString() == "12345");
}

TEST_CASE("BufferGeometryUtils computeMikkTSpaceTangents reports the missing library",
          "[buffergeometryutils]") {
    REQUIRE_THROWS(BufferGeometryUtils::computeMikkTSpaceTangents(triangle(false)));
    try {
        BufferGeometryUtils::computeMikkTSpaceTangents(triangle(false));
    } catch (const std::exception& error) {
        const std::string message = error.what();
        CHECK(message.find("MikkTSpace") != std::string::npos);
    }
}
