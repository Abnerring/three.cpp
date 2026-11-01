#include "loaders/EXRLoader.h"

#include "catch.hpp"

#include <cstdint>
#include <cstddef>

TEST_CASE("EXRLoader gives a PXR channel its planes") {
    // A half precision channel takes two planes; a full precision one three, since its lowest
    // byte is dropped.
    REQUIRE(EXRLoader::pxrPlaneCount(1) == 2);
    REQUIRE(EXRLoader::pxrPlaneCount(2) == 3);
    // Any other type has none.
    REQUIRE(EXRLoader::pxrPlaneCount(0) == 0);
    REQUIRE(EXRLoader::pxrPlaneCount(3) == 0);
}

TEST_CASE("EXRLoader builds a half precision difference") {
    // Two bytes, the first the higher.
    REQUIRE(EXRLoader::pxrDifference(1, 0x12, 0x34, 0) == 0x1234);
    REQUIRE(EXRLoader::pxrDifference(1, 0, 0, 0) == 0);
    REQUIRE(EXRLoader::pxrDifference(1, 0xFF, 0xFF, 0) == 0xFFFF);
    // The third is ignored for this type.
    REQUIRE(EXRLoader::pxrDifference(1, 0x12, 0x34, 0xFF) == 0x1234);
}

TEST_CASE("EXRLoader builds a full precision difference with a zero bottom byte") {
    // Three bytes go into the top three positions and the bottom stays empty, which is the
    // truncation that makes the compression lossy.
    REQUIRE(EXRLoader::pxrDifference(2, 0x12, 0x34, 0x56) == 0x12345600);
    REQUIRE(EXRLoader::pxrDifference(2, 0xFF, 0xFF, 0xFF) == 0xFFFFFF00);
    // The bottom eight bits are always zero, so no value can have them set.
    REQUIRE((EXRLoader::pxrDifference(2, 0xFF, 0xFF, 0xFF) & 0xFF) == 0);
    REQUIRE((EXRLoader::pxrDifference(2, 0x12, 0x34, 0x56) & 0xFF) == 0);
}

TEST_CASE("EXRLoader lays out a PXR row") {
    // Four columns of a half precision channel is two planes of four bytes.
    REQUIRE(EXRLoader::pxrRowBytes(4, 1) == 8);
    REQUIRE(EXRLoader::pxrRowBytes(4, 2) == 12);
    REQUIRE(EXRLoader::pxrRowBytes(0, 1) == 0);
    // The planes of a row beginning at ten, four columns wide.
    const auto half = EXRLoader::pxrPlaneStarts(10, 4, 1);
    REQUIRE(half.size() == 2);
    REQUIRE(half[0] == 10);
    REQUIRE(half[1] == 14);
    // Three planes for a full precision channel.
    const auto full = EXRLoader::pxrPlaneStarts(0, 4, 2);
    REQUIRE(full.size() == 3);
    REQUIRE(full[0] == 0);
    REQUIRE(full[1] == 4);
    REQUIRE(full[2] == 8);
}