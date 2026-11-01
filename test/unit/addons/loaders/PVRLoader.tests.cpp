#include "loaders/PVRLoader.h"

#include "catch.hpp"

#include <cstdint>
#include <vector>

namespace {
void putUint32(std::vector<std::uint8_t>& out, std::size_t at, std::uint32_t value) {
    for (int i = 0; i < 4; ++i) out[at + i] = std::uint8_t((value >> (i * 8)) & 0xff);
}
// A header sized buffer, thirteen words.
std::vector<std::uint8_t> headerBuffer() {
    return std::vector<std::uint8_t>(13 * 4, 0);
}
}  // namespace

TEST_CASE("PVRLoader detects the container version") {
    // The third layout leads with its signature at word zero.
    auto third = headerBuffer();
    putUint32(third, 0, 0x03525650);
    REQUIRE(PVRLoader::detectVersion(third) == 3);

    // The second keeps it at word eleven instead.
    auto second = headerBuffer();
    putUint32(second, 11 * 4, 0x21525650);
    REQUIRE(PVRLoader::detectVersion(second) == 2);
}

TEST_CASE("PVRLoader reports an unknown container") {
    // A header full of zeros has neither signature.
    REQUIRE(PVRLoader::detectVersion(headerBuffer()) == 0);
    // A buffer too short for the words reads as zero rather than out of bounds.
    const std::vector<std::uint8_t> tiny{1, 2};
    REQUIRE(PVRLoader::detectVersion(tiny) == 0);
}

TEST_CASE("PVRLoader reads a little endian header word") {
    auto buffer = headerBuffer();
    putUint32(buffer, 0, 0x12345678);
    REQUIRE(PVRLoader::headerWord(buffer, 0) == 0x12345678);
    // The last word of the header is index twelve.
    putUint32(buffer, 12 * 4, 0xabcdef01);
    REQUIRE(PVRLoader::headerWord(buffer, 12) == 0xabcdef01);
    // A word past the end is zero.
    REQUIRE(PVRLoader::headerWord(buffer, 40) == 0);
}
