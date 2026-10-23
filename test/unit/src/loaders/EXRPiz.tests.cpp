#include "loaders/EXRLoader.h"

#include "catch.hpp"

#include <cstdint>
#include <vector>

TEST_CASE("EXRLoader sizes the PIZ bitmap") {
    // Sixty five thousand five hundred and thirty six values as bits is eight thousand one
    // hundred and ninety two bytes.
    REQUIRE(EXRLoader::kPizBitmapSize == 8192);
    REQUIRE(EXRLoader::kPizBitmapSize * 8 == 65536);
}

TEST_CASE("EXRLoader counts the bytes of a bitmap range") {
    // Both ends count, so one position is one byte.
    REQUIRE(EXRLoader::pizBitmapRunLength(0, 0) == 1);
    REQUIRE(EXRLoader::pizBitmapRunLength(5, 5) == 1);
    REQUIRE(EXRLoader::pizBitmapRunLength(0, 9) == 10);
    // A first end past the second writes nothing, rather than writing a range of zeroes.
    REQUIRE(EXRLoader::pizBitmapRunLength(5, 4) == 0);
    REQUIRE(EXRLoader::pizBitmapRunLength(0, -1) == 0);
    // The largest range the bitmap can hold still fits.
    REQUIRE(EXRLoader::pizBitmapRunLength(0, 8191) == 8192);
}

TEST_CASE("EXRLoader accumulates each PIZ channel's size") {
    // A four by three window with a two word channel is twenty four samples.
    REQUIRE(EXRLoader::pizChannelSize(4, 3, 2) == 24);
    REQUIRE(EXRLoader::pizChannelSize(1, 1, 1) == 1);
    // Each channel begins where the last ended, so three channels of the same size start at
    // nought, twenty four and forty eight.
    const auto starts = EXRLoader::pizChannelStarts(4, 3, 2, 3);
    REQUIRE(starts.size() == 3);
    REQUIRE(starts[0] == 0);
    REQUIRE(starts[1] == 24);
    REQUIRE(starts[2] == 48);
    // No channels means no starts.
    REQUIRE(EXRLoader::pizChannelStarts(4, 3, 2, 0).empty());
}

TEST_CASE("EXRLoader refuses a bitmap that is too long") {
    // The last position that fits is one less than the length.
    REQUIRE(EXRLoader::pizBitmapFits(0));
    REQUIRE(EXRLoader::pizBitmapFits(8191));
    REQUIRE_FALSE(EXRLoader::pizBitmapFits(8192));
    REQUIRE_FALSE(EXRLoader::pizBitmapFits(70000));
}