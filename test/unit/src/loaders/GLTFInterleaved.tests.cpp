#include "loaders/GLTFLoader.h"

#include "catch.hpp"

TEST_CASE("GLTFLoader tells an interleaved layout") {
    // No stride at all is the plain layout.
    REQUIRE_FALSE(GLTFLoader::isInterleaved(0, 12));
    // A stride equal to one element is the plain layout too.
    REQUIRE_FALSE(GLTFLoader::isInterleaved(12, 12));
    // A wider stride means other attributes sit between the elements.
    REQUIRE(GLTFLoader::isInterleaved(16, 12));
    REQUIRE(GLTFLoader::isInterleaved(32, 12));
}

TEST_CASE("GLTFLoader finds the slice a byte offset starts in") {
    // An offset of one stride lands in the second slice.
    REQUIRE(GLTFLoader::interleavedSlice(16, 16) == 1);
    REQUIRE(GLTFLoader::interleavedSlice(0, 16) == 0);
    // Part way into a slice still belongs to it.
    REQUIRE(GLTFLoader::interleavedSlice(20, 16) == 1);
    // A stride of zero cannot be divided by.
    REQUIRE(GLTFLoader::interleavedSlice(20, 0) == 0);
}

TEST_CASE("GLTFLoader finds where an attribute begins within an element") {
    // Four bytes into a sixteen byte element is one component of four bytes.
    REQUIRE(GLTFLoader::interleavedAttributeOffset(4, 16, 4) == 1);
    // An offset of zero begins at the first component.
    REQUIRE(GLTFLoader::interleavedAttributeOffset(0, 16, 4) == 0);
    // Eight bytes in is the third component.
    REQUIRE(GLTFLoader::interleavedAttributeOffset(8, 16, 4) == 2);
    // A zero stride or a zero element cannot be divided by.
    REQUIRE(GLTFLoader::interleavedAttributeOffset(4, 0, 4) == 0);
    REQUIRE(GLTFLoader::interleavedAttributeOffset(4, 16, 0) == 0);
}

TEST_CASE("GLTFLoader converts the stride to components") {
    // Sixteen bytes of four byte components is a stride of four.
    REQUIRE(GLTFLoader::interleavedStride(16, 4) == 4);
    // One two byte component runs at half the rate.
    REQUIRE(GLTFLoader::interleavedStride(16, 2) == 8);
    // A zero element cannot be divided by.
    REQUIRE(GLTFLoader::interleavedStride(16, 0) == 0);
}

TEST_CASE("GLTFLoader walks a two attribute layout") {
    // Two attributes of three floats each share a stride of twenty four bytes: the first
    // begins at zero and the second twelve bytes in.
    const std::size_t stride = 24;
    const std::size_t itemBytes = 12;
    REQUIRE(GLTFLoader::isInterleaved(stride, itemBytes));
    // The first attribute begins at the start of each element.
    REQUIRE(GLTFLoader::interleavedAttributeOffset(0, stride, 4) == 0);
    // The second begins three components in.
    REQUIRE(GLTFLoader::interleavedAttributeOffset(12, stride, 4) == 3);
    // The stride is six components.
    REQUIRE(GLTFLoader::interleavedStride(stride, 4) == 6);
    // The third element begins at byte forty eight, which is the second slice.
    REQUIRE(GLTFLoader::interleavedSlice(48, stride) == 2);
}