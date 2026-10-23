#include "loaders/EXRLoader.h"

#include "catch.hpp"

#include <cstdint>
#include <cstddef>

TEST_CASE("EXRLoader places a sample in the output") {
    // The row picks a line, the column a pixel and the channel one of that pixel's places, so
    // the first sample of the first row is at the very start.
    REQUIRE(EXRLoader::scanlineWriteIndex(0, 0, 0, 4, 400) == 0);
    // One pixel along with all four channels of the pixel together: the second channel of the
    // second pixel is six places in.
    REQUIRE(EXRLoader::scanlineWriteIndex(0, 1, 2, 4, 400) == 6);
    REQUIRE(EXRLoader::scanlineWriteIndex(0, 1, 0, 4, 400) == 4);
    // A row's width depends only on the line, so the fourth row begins twelve hundred in.
    REQUIRE(EXRLoader::scanlineWriteIndex(3, 0, 0, 4, 400) == 1200);
    REQUIRE(EXRLoader::scanlineWriteIndex(3, 5, 1, 4, 400) == 1221);
}

TEST_CASE("EXRLoader places a sample in the block") {
    // Within a block the lines come first and each channel has a run of its own inside a line.
    // Four columns of a two byte sample: a line is eight bytes and a channel's run is four
    // samples.
    REQUIRE(EXRLoader::scanlineReadIndex(0, 0, 0, 4, 2) == 0);
    // The second channel begins four samples along, which is four bytes in.
    REQUIRE(EXRLoader::scanlineReadIndex(0, 1, 0, 4, 2) == 4);
    // The second line begins a whole line along.
    REQUIRE(EXRLoader::scanlineReadIndex(1, 0, 0, 4, 2) == 8);
    // The third column of the first channel is four bytes in, since each sample is two.
    REQUIRE(EXRLoader::scanlineReadIndex(0, 0, 2, 4, 2) == 4);
    REQUIRE(EXRLoader::scanlineReadIndex(0, 0, 3, 4, 2) == 6);
}

TEST_CASE("EXRLoader agrees the two placements differ") {
    // The same row, column and channel land in different places, because the output packs a
    // pixel's channels together while a block keeps each channel's samples in a run.
    const auto written = EXRLoader::scanlineWriteIndex(0, 2, 1, 4, 400);
    const auto read = EXRLoader::scanlineReadIndex(0, 1, 2, 4, 2);
    REQUIRE(written == 9);
    REQUIRE(read == 8);
    REQUIRE(written != read);
    // The output's place is in samples and the block's in bytes, which is why they cannot be
    // compared by value anyway.
    REQUIRE(EXRLoader::scanlineWriteIndex(0, 1, 0, 4, 400) == 4);
    REQUIRE(EXRLoader::scanlineReadIndex(0, 0, 1, 4, 2) == 2);
}
