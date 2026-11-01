#include "loaders/EXRLoader.h"

#include "catch.hpp"

#include <cstdint>
#include <vector>

namespace {
// A real zlib stream holding the four bytes 1 2 3 4, with the header and checksum added to a
// stream a standard compressor produced.
std::vector<std::uint8_t> fourBytes() {
    return {0x78, 0x01, 0x63, 0x64, 0x62, 0x66, 0x01, 0x00, 0x00, 0x18, 0x00, 0x0B};
}
}  // namespace

TEST_CASE("EXRLoader unpacks a zip compressed block") {
    // The four bytes come back through the inflate, the predictor and the interleave.
    const auto result = EXRLoader::uncompressZIP(fourBytes());
    INFO("error: " << result.error);
    REQUIRE(result.valid);
    REQUIRE(result.bytes.size() == 4);
    // The predictor leaves 1, 131, 6 and 138, and the interleave puts them in the order
    // 1, 6, 131, 138 because the halves are the first two and the last two.
    REQUIRE(result.bytes[0] == 1);
    REQUIRE(result.bytes[1] == 6);
    REQUIRE(result.bytes[2] == 131);
    REQUIRE(result.bytes[3] == 138);
}

TEST_CASE("EXRLoader refuses a block that is not a stream") {
    // Bytes that are not a zlib stream are turned away rather than guessed at.
    const auto result = EXRLoader::uncompressZIP({1, 2, 3, 4});
    REQUIRE_FALSE(result.valid);
    REQUIRE_FALSE(result.error.empty());
    REQUIRE(result.bytes.empty());
    // An empty block likewise.
    REQUIRE_FALSE(EXRLoader::uncompressZIP({}).valid);
}