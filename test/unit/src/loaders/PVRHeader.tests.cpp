#include "loaders/PVRLoader.h"

#include "catch.hpp"

#include <cstdint>
#include <vector>

namespace {
void putUint32(std::vector<std::uint8_t>& out, std::size_t at, std::uint32_t value) {
    for (int i = 0; i < 4; ++i) out[at + i] = std::uint8_t((value >> (i * 8)) & 0xff);
}
// A third version header with the given pixel format and dimensions.
std::vector<std::uint8_t> version3(std::uint32_t format, std::uint32_t width, std::uint32_t height,
                                   std::uint32_t faces, std::uint32_t mips, std::uint32_t metaLen) {
    std::vector<std::uint8_t> out(13 * 4 + metaLen, 0);
    putUint32(out, 0, 0x03525650);
    putUint32(out, 2 * 4, format);
    putUint32(out, 6 * 4, height);
    putUint32(out, 7 * 4, width);
    putUint32(out, 10 * 4, faces);
    putUint32(out, 11 * 4, mips);
    putUint32(out, 12 * 4, metaLen);
    return out;
}
}  // namespace

TEST_CASE("PVRLoader reads the third version header") {
    const auto buffer = version3(2, 256, 128, 1, 7, 0);
    const auto header = PVRLoader::parseVersion3(buffer);
    REQUIRE(header.pixelFormat == 2);
    // Format two is four bits per pixel RGB.
    REQUIRE(header.bitsPerPixel == 4);
    REQUIRE(header.formatName == "RGB_PVRTC_4BPPV1");
    REQUIRE(header.width == 256);
    REQUIRE(header.height == 128);
    REQUIRE(header.numFaces == 1);
    REQUIRE(header.numMipmaps == 7);
    REQUIRE_FALSE(header.isCubemap);
    // The payload starts right after the thirteen header words.
    REQUIRE(header.dataOffset == 52);
}

TEST_CASE("PVRLoader names every format it knows") {
    // Two bits for the first two formats and four for the others.
    REQUIRE(PVRLoader::parseVersion3(version3(0, 1, 1, 1, 1, 0)).bitsPerPixel == 2);
    REQUIRE(PVRLoader::parseVersion3(version3(0, 1, 1, 1, 1, 0)).formatName == "RGB_PVRTC_2BPPV1");
    REQUIRE(PVRLoader::parseVersion3(version3(1, 1, 1, 1, 1, 0)).formatName == "RGBA_PVRTC_2BPPV1");
    REQUIRE(PVRLoader::parseVersion3(version3(3, 1, 1, 1, 1, 0)).formatName == "RGBA_PVRTC_4BPPV1");
}

TEST_CASE("PVRLoader skips the metadata before the payload") {
    const auto buffer = version3(1, 8, 8, 1, 1, 64);
    const auto header = PVRLoader::parseVersion3(buffer);
    // Fifty two plus sixty four.
    REQUIRE(header.metadataLength == 64);
    REQUIRE(header.dataOffset == 116);
}

TEST_CASE("PVRLoader marks six faces as a cubemap") {
    REQUIRE(PVRLoader::parseVersion3(version3(3, 4, 4, 6, 1, 0)).isCubemap);
    REQUIRE_FALSE(PVRLoader::parseVersion3(version3(3, 4, 4, 2, 1, 0)).isCubemap);
}

TEST_CASE("PVRLoader rejects an unknown pixel format") {
    REQUIRE_THROWS_WITH(PVRLoader::parseVersion3(version3(9, 1, 1, 1, 1, 0)),
                        "THREE.PVRLoader: Unsupported PVR format: 9");
}
