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

TEST_CASE("EXRLoader refuses a single line compressed block") {
    // A block whose size is smaller than its pixels is compressed, and the readers that work on
    // bytes have not been wired into this loop, so it says so rather than reading rubbish.
    auto data = twoByFour();
    // The first block claims four bytes rather than eight, which reads as compressed.
    data[7] = 0x04;
    const auto result = EXRLoader::parseScanline(data, 0, 4, 2, 0, 1, oneChannel());
    REQUIRE_FALSE(result.valid);
    REQUIRE(result.error == "THREE.EXRLoader: a compressed block is not read yet");
}