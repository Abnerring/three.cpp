#include "loaders/DDSLoader.h"

#include "catch.hpp"

#include <cstdint>
#include <vector>

namespace {
void putUint32(std::vector<std::uint8_t>& out, std::size_t at, std::uint32_t value) {
    for (int i = 0; i < 4; ++i) out[at + i] = std::uint8_t((value >> (i * 8)) & 0xff);
}
// Packs four characters four in the low byte upwards, which is how the format name is stored.
std::uint32_t packFourCC(const char* text) {
    std::uint32_t packed = 0;
    for (int i = 0; i < 4; ++i) packed |= static_cast<std::uint32_t>(text[i]) << (i * 8);
    return packed;
}
// A header of thirty one words with the given fields.
std::vector<std::uint8_t> ddsHeader(std::uint32_t width, std::uint32_t height, std::uint32_t mips,
                                    const char* fourCC, std::uint32_t caps2) {
    std::vector<std::uint8_t> out(124, 0);
    putUint32(out, 0, 0x20534444);
    putUint32(out, 1 * 4, 124);
    putUint32(out, 2 * 4, 0x1007);
    putUint32(out, 3 * 4, height);
    putUint32(out, 4 * 4, width);
    putUint32(out, 7 * 4, mips);
    putUint32(out, 21 * 4, packFourCC(fourCC));
    putUint32(out, 22 * 4, 32);
    putUint32(out, 28 * 4, caps2);
    return out;
}
}  // namespace

TEST_CASE("DDSLoader reads the header") {
    const auto buffer = ddsHeader(256, 128, 5, "DXT1", 0x1000);
    const auto header = DDSLoader::readHeader(buffer);
    REQUIRE(header.width == 256);
    REQUIRE(header.height == 128);
    REQUIRE(header.mipmapCount == 5);
    REQUIRE(header.rgbBitCount == 32);
    // The four characters come back in order.
    REQUIRE(header.fourCCName == "DXT1");
    REQUIRE(header.fourCC == packFourCC("DXT1"));
    // The payload starts after the header size and the magic.
    REQUIRE(header.dataOffset == 128);
    REQUIRE_FALSE(header.isCubemap);
}

TEST_CASE("DDSLoader packs the four character codes little endian") {
    // The characters are stored four in the low byte upwards, so D comes first.
    REQUIRE(packFourCC("DXT1") == 0x31545844);
    REQUIRE(DDSLoader::readHeader(ddsHeader(1, 1, 1, "DXT5", 0)).fourCCName == "DXT5");
    REQUIRE(DDSLoader::readHeader(ddsHeader(1, 1, 1, "DX10", 0)).fourCCName == "DX10");
}

TEST_CASE("DDSLoader marks a complete cube map") {
    // The bit alone is not enough; all six faces have to be set too.
    REQUIRE_FALSE(DDSLoader::readHeader(ddsHeader(4, 4, 1, "DXT1", 0x200)).isCubemap);
    REQUIRE(DDSLoader::readHeader(ddsHeader(4, 4, 1, "DXT1", 0x200 | 0xfc00)).isCubemap);
    // The face bits without the cube bit are not a cube map either.
    REQUIRE_FALSE(DDSLoader::readHeader(ddsHeader(4, 4, 1, "DXT1", 0xfc00)).isCubemap);
}

TEST_CASE("DDSLoader rejects a file with the wrong magic") {
    auto buffer = ddsHeader(1, 1, 1, "DXT1", 0);
    putUint32(buffer, 0, 0x12345678);
    REQUIRE_THROWS_WITH(DDSLoader::readHeader(buffer),
                        "THREE.DDSLoader.parse: Invalid magic number in DDS header.");
}

TEST_CASE("DDSLoader reads a word past the end as zero") {
    // The four magic bytes alone are a valid header start, with every field after it zero.
    const std::vector<std::uint8_t> tiny{0x44, 0x44, 0x53, 0x20};
    REQUIRE(DDSLoader::headerWord(tiny, 5) == 0);
    const auto header = DDSLoader::readHeader(tiny);
    // The declared size is zero, so the payload would start just after the magic.
    REQUIRE(header.size == 0);
    REQUIRE(header.dataOffset == 4);
    REQUIRE(header.width == 0);
    REQUIRE(header.mipmapCount == 0);
    // With no printable four characters the name stays empty.
    REQUIRE(header.fourCCName.empty());
}
