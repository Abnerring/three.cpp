#include "loaders/EXRLoader.h"

#include "catch.hpp"

#include <cstdint>

TEST_CASE("EXRLoader doubles a B44 channel's sample count") {
    // A half counts as one and a full precision value as two, so the bytes are doubled.
    REQUIRE(EXRLoader::b44PixelSize(1) == 2);
    REQUIRE(EXRLoader::b44PixelSize(2) == 4);
    // A type the format does not use gives nothing.
    REQUIRE(EXRLoader::b44PixelSize(0) == 0);
    // The two block constants.
    REQUIRE(EXRLoader::kB44BlockExtent == 4);
    REQUIRE(EXRLoader::kB44BlockBytes == 14);
}

TEST_CASE("EXRLoader rounds a sampled channel up") {
    // Every sample, every second and every fourth.
    REQUIRE(EXRLoader::b44ChannelExtent(8, 1) == 8);
    REQUIRE(EXRLoader::b44ChannelExtent(8, 2) == 4);
    REQUIRE(EXRLoader::b44ChannelExtent(8, 4) == 2);
    // A size that does not divide evenly is rounded up, so the last sample survives.
    REQUIRE(EXRLoader::b44ChannelExtent(9, 2) == 5);
    REQUIRE(EXRLoader::b44ChannelExtent(7, 4) == 2);
    // A sampling of zero would divide by nothing, so the size is left as it is.
    REQUIRE(EXRLoader::b44ChannelExtent(8, 0) == 8);
}

TEST_CASE("EXRLoader tells a full resolution channel from a sampled one") {
    // Only both at every pixel counts.
    REQUIRE(EXRLoader::b44IsFullResolution(1, 1));
    REQUIRE_FALSE(EXRLoader::b44IsFullResolution(2, 1));
    REQUIRE_FALSE(EXRLoader::b44IsFullResolution(1, 2));
    REQUIRE_FALSE(EXRLoader::b44IsFullResolution(2, 2));
}

TEST_CASE("EXRLoader compresses only half precision channels") {
    // A full precision channel is stored as it lies rather than through the blocks.
    REQUIRE(EXRLoader::b44IsCompressed(1));
    REQUIRE_FALSE(EXRLoader::b44IsCompressed(2));
    REQUIRE_FALSE(EXRLoader::b44IsCompressed(0));
}

TEST_CASE("EXRLoader counts the blocks of a side") {
    // Four samples to a block, rounded up.
    REQUIRE(EXRLoader::b44BlockCount(4) == 1);
    REQUIRE(EXRLoader::b44BlockCount(5) == 2);
    REQUIRE(EXRLoader::b44BlockCount(8) == 2);
    REQUIRE(EXRLoader::b44BlockCount(9) == 3);
    REQUIRE(EXRLoader::b44BlockCount(0) == 0);
    // A single sample still needs a whole block, which is why the rounding is up.
    REQUIRE(EXRLoader::b44BlockCount(1) == 1);
}

TEST_CASE("EXRLoader allows a flat block only in the second kind") {
    // The second of the two block kinds permits a three byte block for a flat patch.
    REQUIRE(EXRLoader::b44AllowsFlat(true));
    REQUIRE_FALSE(EXRLoader::b44AllowsFlat(false));
}