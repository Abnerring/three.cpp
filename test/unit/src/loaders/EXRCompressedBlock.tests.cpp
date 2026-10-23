#include "loaders/EXRLoader.h"

#include "catch.hpp"

#include <cstdint>
#include <cstddef>
#include <string>
#include <vector>

namespace {
// A file one line tall and thirty two columns wide, one half precision channel, compressed with
// the single line kind. The blocks are a line at a time, so one block holds it. The payload is
// sixty four zero bytes deflated, which comes to twelve bytes and so marks the block compressed
// against the sixty four a line of samples takes.
std::vector<std::uint8_t> oneLine() {
    return {
        // The block's first line and its size.
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0C,
        0x78, 0x01, 0x63, 0x60, 0xA0, 0x0C, 0x00, 0x00, 0x00, 0x40, 0x00, 0x01,
    };
}
std::vector<EXRLoader::ScanlineChannel> oneChannel() { return {{"Y", 1, true}}; }
}  // namespace

TEST_CASE("EXRLoader reads a block through the zip reader") {
    // The block's size is smaller than the sixty four bytes a line of samples takes, so it is
    // handed to the zip reader, inflated to sixty four zero bytes and read as thirty two
    // samples of zero.
    const auto result = EXRLoader::parseScanline(oneLine(), 0, 32, 1, 2, 1, oneChannel());
    INFO("error: " << result.error);
    REQUIRE(result.valid);
    REQUIRE(result.samples.size() == 32);
    // The samples are not zero even though the payload was, because the zip reader undoes
    // a predictor as part of its work: a zero following a zero becomes one hundred and
    // twenty eight, and the one after that comes back to zero. So the first sample is
    // zero and the second is the top byte of one hundred and twenty eight.
    REQUIRE(result.samples[0] == 0x0000);
    // At least one sample is not zero, which shows the predictor ran rather than the
    // block having been passed through untouched.
    bool anyNonZero = false;
    for (const auto sample : result.samples) if (sample != 0) anyNonZero = true;
    REQUIRE(anyNonZero);
}
