#include "loaders/EXRLoader.h"

#include "catch.hpp"

#include <cstdint>
#include <vector>

TEST_CASE("EXRLoader reads a PIZ buffer row by row across channels") {
    // Two channels of a two by two window, one word wide: each channel's samples take four
    // places, so the second begins at four and each row is two long.
    const auto offsets = EXRLoader::pizReadOffsets(2, 2, 1, 2);
    REQUIRE(offsets.size() == 4);
    // The first row takes from the first channel and then the second.
    REQUIRE(offsets[0] == 0);
    REQUIRE(offsets[1] == 4);
    // The second row follows on from each.
    REQUIRE(offsets[2] == 2);
    REQUIRE(offsets[3] == 6);
}

TEST_CASE("EXRLoader keeps a single channel contiguous") {
    // One channel of a three by two window, one word wide: the reads march straight through.
    const auto offsets = EXRLoader::pizReadOffsets(3, 2, 1, 1);
    REQUIRE(offsets.size() == 2);
    REQUIRE(offsets[0] == 0);
    REQUIRE(offsets[1] == 3);
    // Three rows of a one by three window likewise.
    const auto tall = EXRLoader::pizReadOffsets(1, 3, 1, 1);
    REQUIRE(tall.size() == 3);
    REQUIRE(tall[0] == 0);
    REQUIRE(tall[1] == 1);
    REQUIRE(tall[2] == 2);
}

TEST_CASE("EXRLoader widens each read when a channel is wider") {
    // A channel two words wide doubles both the row length and the channel size, so with two
    // channels the second begins at eight and each row is four long.
    const auto offsets = EXRLoader::pizReadOffsets(2, 2, 2, 2);
    REQUIRE(offsets.size() == 4);
    REQUIRE(offsets[0] == 0);
    REQUIRE(offsets[1] == 8);
    REQUIRE(offsets[2] == 4);
    REQUIRE(offsets[3] == 12);
}

TEST_CASE("EXRLoader gives no reads for an empty window or no channels") {
    // No rows means nothing is read.
    REQUIRE(EXRLoader::pizReadOffsets(4, 0, 1, 2).empty());
    // No channels likewise.
    REQUIRE(EXRLoader::pizReadOffsets(4, 4, 1, 0).empty());
    // One read per row when there is a single channel of one row.
    const auto single = EXRLoader::pizReadOffsets(1, 1, 1, 1);
    REQUIRE(single.size() == 1);
    REQUIRE(single[0] == 0);
}