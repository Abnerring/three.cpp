#include "loaders/EXRLoader.h"

#include "catch.hpp"

#include <cstdint>
#include <cstddef>
#include <string>
#include <vector>

namespace {
// A file one line tall and thirty two columns wide, one half precision channel, compressed with
// the run length kind, whose blocks are a line at a time. The block's payload is two bytes: a
// count of sixty three and a value, which expands to sixty four copies of that value. Two bytes
// against the sixty four a line takes marks the block compressed.
std::vector<std::uint8_t> oneLine() {
    return {
        // The block's first line and its size.
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02,
        // The run: sixty three means sixty four copies.
        0x3F, 0x12,
    };
}
std::vector<EXRLoader::ScanlineChannel> oneChannel() { return {{"Y", 1, true}}; }
}  // namespace

TEST_CASE("EXRLoader reads a block through the run length reader") {
    // The block is handed to the run length reader, which expands it to sixty four bytes, and
    // those are read as thirty two samples.
    const auto result = EXRLoader::parseScanline(oneLine(), 0, 32, 1, 1, 1, oneChannel());
    INFO("error: " << result.error);
    REQUIRE(result.valid);
    REQUIRE(result.samples.size() == 32);
}

TEST_CASE("EXRLoader's run length reader leaves the samples varied") {
    // The payload was one value repeated, but the reader also undoes a predictor and then puts
    // the halves back together, so the samples are not all that value. That the samples differ
    // from one another is what shows those two steps ran rather than the bytes passing through.
    const auto result = EXRLoader::parseScanline(oneLine(), 0, 32, 1, 1, 1, oneChannel());
    REQUIRE(result.valid);
    bool allSame = true;
    for (const auto sample : result.samples) {
        if (sample != result.samples[0]) allSame = false;
    }
    REQUIRE_FALSE(allSame);
    // The first sample is two bytes: the value itself, and the one after it, which the
    // predictor makes by adding the two values and taking one hundred and twenty eight
    // off. Thirty six less one hundred and twenty eight is minus ninety two, which in
    // eight bits is one hundred and sixty four.
    REQUIRE(result.samples[0] == 0x12A4);
}

TEST_CASE("EXRLoader's run length reader answers the same as the reader directly") {
    // Reading the same payload through the dispatcher gives the bytes the reader itself gives,
    // which shows the scanline loop adds nothing of its own on the way.
    const auto bytes = EXRLoader::uncompressRLE({0x3F, 0x12});
    REQUIRE(bytes.valid);
    REQUIRE(bytes.bytes.size() == 64);
    const auto through = EXRLoader::decompressBlock({0x3F, 0x12}, 1, 32, 1, 2, 1, {1});
    INFO("error: " << through.error);
    REQUIRE(through.valid);
    REQUIRE(through.bytes == bytes.bytes);
}
