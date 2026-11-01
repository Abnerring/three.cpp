#include "loaders/PVRLoader.h"

#include "catch.hpp"

#include <cstdint>
#include <vector>

namespace {
void putUint32(std::vector<std::uint8_t>& out, std::size_t at, std::uint32_t value) {
    for (int i = 0; i < 4; ++i) out[at + i] = std::uint8_t((value >> (i * 8)) & 0xff);
}
// A second version header with the given fields.
std::vector<std::uint8_t> version2(std::uint32_t headerLength, std::uint32_t width,
                                   std::uint32_t height, std::uint32_t mips, std::uint32_t flags,
                                   std::uint32_t alphaMask, std::uint32_t surfaces) {
    std::vector<std::uint8_t> out(13 * 4, 0);
    putUint32(out, 0, headerLength);
    putUint32(out, 1 * 4, height);
    putUint32(out, 2 * 4, width);
    putUint32(out, 3 * 4, mips);
    putUint32(out, 4 * 4, flags);
    putUint32(out, 10 * 4, alphaMask);
    putUint32(out, 11 * 4, 0x21525650);
    putUint32(out, 12 * 4, surfaces);
    return out;
}
}  // namespace

TEST_CASE("PVRLoader reads the second version header") {
    const auto buffer = version2(52, 128, 64, 3, 25, 1, 1);
    const auto header = PVRLoader::parseVersion2(buffer);
    // Twenty five is the four bit format and the alpha mask is set.
    REQUIRE(header.bitsPerPixel == 4);
    REQUIRE(header.formatName == "RGBA_PVRTC_4BPPV1");
    REQUIRE(header.width == 128);
    REQUIRE(header.height == 64);
    // The stored mipmap count is one less than the number of levels.
    REQUIRE(header.numMipmaps == 4);
    // The payload starts at the recorded header length.
    REQUIRE(header.dataOffset == 52);
    REQUIRE_FALSE(header.isCubemap);
}

TEST_CASE("PVRLoader picks the alpha format from the bitmask") {
    // The same format without an alpha mask is the RGB variant.
    REQUIRE(PVRLoader::parseVersion2(version2(52, 8, 8, 0, 25, 0, 1)).formatName ==
            "RGB_PVRTC_4BPPV1");
    REQUIRE(PVRLoader::parseVersion2(version2(52, 8, 8, 0, 24, 0, 1)).formatName ==
            "RGB_PVRTC_2BPPV1");
    REQUIRE(PVRLoader::parseVersion2(version2(52, 8, 8, 0, 24, 0xff, 1)).formatName ==
            "RGBA_PVRTC_2BPPV1");
}

TEST_CASE("PVRLoader masks the format out of the flags") {
    // Only the low byte names the format, so higher bits do not change it.
    const auto buffer = version2(52, 8, 8, 0, 24 | 0x100, 0, 1);
    REQUIRE(PVRLoader::parseVersion2(buffer).formatName == "RGB_PVRTC_2BPPV1");
}

TEST_CASE("PVRLoader takes the payload offset from the header") {
    // A header length of one hundred moves the payload there.
    REQUIRE(PVRLoader::parseVersion2(version2(100, 8, 8, 0, 24, 0, 1)).dataOffset == 100);
}

TEST_CASE("PVRLoader marks six surfaces as a cubemap and rejects an unknown format") {
    REQUIRE(PVRLoader::parseVersion2(version2(52, 4, 4, 0, 25, 1, 6)).isCubemap);
    REQUIRE_THROWS_WITH(PVRLoader::parseVersion2(version2(52, 1, 1, 0, 7, 0, 1)),
                        "THREE.PVRLoader: Unknown PVR format: 7");
}
