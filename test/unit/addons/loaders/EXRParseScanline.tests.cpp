#include "loaders/EXRLoader.h"

#include "catch.hpp"

#include <cstdint>
#include <cstddef>
#include <string>
#include <vector>

namespace {
// A file of two rows and four columns, one half precision channel, uncompressed. Each block
// begins with its first line and its size, and the samples follow high byte first.
std::vector<std::uint8_t> twoByFour() {
    return {
        // Block for line nought: line number, then eight bytes.
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08,
        0x11, 0x11, 0x22, 0x22, 0x33, 0x33, 0x44, 0x44,
        // Block for line one.
        0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x08,
        0x55, 0x55, 0x66, 0x66, 0x77, 0x77, 0x88, 0x88,
    };
}
std::vector<EXRLoader::ScanlineChannel> oneChannel() {
    return {{"Y", 1, true}};
}
}  // namespace

TEST_CASE("EXRLoader reads two uncompressed rows") {
    const auto result = EXRLoader::parseScanline(twoByFour(), 0, 4, 2, 0, 1, oneChannel());
    INFO("error: " << result.error);
    REQUIRE(result.valid);
    REQUIRE(result.samples.size() == 8);
    // The first line of the file is written to the second row of the output, so the samples
    // read from line nought land in the last four places.
    REQUIRE(result.samples[4] == 0x1111);
    REQUIRE(result.samples[5] == 0x2222);
    REQUIRE(result.samples[6] == 0x3333);
    REQUIRE(result.samples[7] == 0x4444);
    // And line one lands in the first four.
    REQUIRE(result.samples[0] == 0x5555);
    REQUIRE(result.samples[1] == 0x6666);
    REQUIRE(result.samples[2] == 0x7777);
    REQUIRE(result.samples[3] == 0x8888);
}

TEST_CASE("EXRLoader refuses an output it cannot write") {
    // Only the half precision output is written; another type is refused rather than guessed.
    const auto result = EXRLoader::parseScanline(twoByFour(), 0, 4, 2, 0, 2, oneChannel());
    REQUIRE_FALSE(result.valid);
    REQUIRE(result.error == "THREE.EXRLoader: only half precision output is read");
}

TEST_CASE("EXRLoader refuses a compression it does not know") {
    // A number outside the ten has no block height and no reader.
    const auto result = EXRLoader::parseScanline(twoByFour(), 0, 4, 2, 99, 1, oneChannel());
    REQUIRE_FALSE(result.valid);
    REQUIRE_FALSE(result.error.empty());
}

TEST_CASE("EXRLoader reads through a block's own reader") {
    // A block whose size is smaller than its pixels is handed to the reader its compression
    // names. Nought names the uncompressed reader, which hands the bytes back unchanged, so a
    // block claiming four bytes when a line needs eight runs out while being read rather than
    // being refused before it starts.
    auto data = twoByFour();
    data[7] = 0x04;
    const auto result = EXRLoader::parseScanline(data, 0, 4, 2, 0, 1, oneChannel());
    REQUIRE_FALSE(result.valid);
    REQUIRE_FALSE(result.error.empty());
}

TEST_CASE("EXRLoader reads nothing at all when the height is under one block") {
    // A block compression takes thirty two lines at a time, and the block count is the height
    // divided by that with the remainder dropped. A file of two lines therefore has no blocks
    // at all, so nothing is read and the answer is an empty array rather than a failure. This
    // is the same dropped remainder recorded for the loop, seen from the outside.
    const auto result = EXRLoader::parseScanline(twoByFour(), 0, 4, 2, 6, 1, oneChannel());
    INFO("error: " << result.error);
    REQUIRE(result.valid);
    REQUIRE(result.samples.size() == 8);
    for (const auto sample : result.samples) REQUIRE(sample == 0);
    // The same file read as one line at a time does reach its blocks, which shows the
    // difference is the block height and nothing else.
    const auto lineByLine = EXRLoader::parseScanline(twoByFour(), 0, 4, 2, 0, 1, oneChannel());
    REQUIRE(lineByLine.valid);
    REQUIRE(lineByLine.samples[4] == 0x1111);
}

TEST_CASE("EXRLoader refuses a compression it has no reader for") {
    // The dispatcher is asked directly, since a file short enough to reach a block of this kind
    // would have to be at least thirty two lines tall.
    const auto result = EXRLoader::decompressBlock({1, 2, 3}, 6, 2, 1, 1, 1, {1});
    REQUIRE_FALSE(result.valid);
    REQUIRE(result.error == "THREE.EXRLoader: the block compression is not read yet");
}
