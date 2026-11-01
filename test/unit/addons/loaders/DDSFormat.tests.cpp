#include "loaders/DDSLoader.h"

#include "catch.hpp"

#include <cstdint>
#include <vector>

namespace {
void putUint32(std::vector<std::uint8_t>& out, std::size_t at, std::uint32_t value) {
    for (int i = 0; i < 4; ++i) out[at + i] = std::uint8_t((value >> (i * 8)) & 0xff);
}
std::uint32_t packFourCC(const char* text) {
    std::uint32_t packed = 0;
    for (int i = 0; i < 4; ++i) packed |= static_cast<std::uint32_t>(text[i]) << (i * 8);
    return packed;
}
// A buffer with the given four character code, and room for an extended header.
std::vector<std::uint8_t> coded(const char* fourCC) {
    std::vector<std::uint8_t> out(36 * 4, 0);
    putUint32(out, 0, 0x20534444);
    putUint32(out, 1 * 4, 124);
    putUint32(out, 21 * 4, packFourCC(fourCC));
    return out;
}
DDSLoader::Header headerFor(const std::vector<std::uint8_t>& buffer) {
    return DDSLoader::readHeader(buffer);
}
}  // namespace

TEST_CASE("DDSLoader resolves the pixel format") {
    // The four block compressed codes.
    const auto dxt1 = coded("DXT1");
    REQUIRE(DDSLoader::resolveFormat(headerFor(dxt1), dxt1).blockBytes == 8);
    REQUIRE(DDSLoader::resolveFormat(headerFor(dxt1), dxt1).name == "RGB_S3TC_DXT1");
    const auto dxt5 = coded("DXT5");
    REQUIRE(DDSLoader::resolveFormat(headerFor(dxt5), dxt5).blockBytes == 16);
    REQUIRE(DDSLoader::resolveFormat(headerFor(dxt5), dxt5).name == "RGBA_S3TC_DXT5");
    const auto etc1 = coded("ETC1");
    REQUIRE(DDSLoader::resolveFormat(headerFor(etc1), etc1).name == "RGB_ETC1");
}

TEST_CASE("DDSLoader follows the extended header for DX10") {
    // The extended format sits one word after the fixed header.
    auto unsigned16 = coded("DX10");
    putUint32(unsigned16, 31 * 4, 95);
    REQUIRE(DDSLoader::resolveFormat(headerFor(unsigned16), unsigned16).name ==
            "RGB_BPTC_UNSIGNED");
    REQUIRE(DDSLoader::resolveFormat(headerFor(unsigned16), unsigned16).blockBytes == 16);

    auto signed16 = coded("DX10");
    putUint32(signed16, 31 * 4, 96);
    REQUIRE(DDSLoader::resolveFormat(headerFor(signed16), signed16).name == "RGB_BPTC_SIGNED");
}

TEST_CASE("DDSLoader reports an unknown extended format") {
    auto buffer = coded("DX10");
    putUint32(buffer, 31 * 4, 42);
    REQUIRE_THROWS_WITH(DDSLoader::resolveFormat(headerFor(buffer), buffer),
                        "THREE.DDSLoader.parse: Unsupported DXGI_FORMAT code 42");
}

TEST_CASE("DDSLoader reads an uncompressed format from the masks") {
    // A thirty two bit pixel with all four masks is RGBA.
    auto rgba = coded("    ");
    putUint32(rgba, 22 * 4, 32);
    putUint32(rgba, 23 * 4, 0xff0000);
    putUint32(rgba, 24 * 4, 0xff00);
    putUint32(rgba, 25 * 4, 0xff);
    putUint32(rgba, 26 * 4, 0xff000000);
    const auto resolved = DDSLoader::resolveFormat(headerFor(rgba), rgba);
    REQUIRE(resolved.name == "RGBA");
    REQUIRE(resolved.bytesPerPixel == 4);
    REQUIRE(resolved.blockBytes == 0);

    // Twenty four bits without an alpha mask is RGB.
    auto rgb = coded("    ");
    putUint32(rgb, 22 * 4, 24);
    putUint32(rgb, 23 * 4, 0xff0000);
    putUint32(rgb, 24 * 4, 0xff00);
    putUint32(rgb, 25 * 4, 0xff);
    const auto plain = DDSLoader::resolveFormat(headerFor(rgb), rgb);
    REQUIRE(plain.name == "RGB");
    REQUIRE(plain.bytesPerPixel == 3);
}

TEST_CASE("DDSLoader rejects a mask pattern it cannot name") {
    // The right bit count but the wrong mask placement is not a format it knows.
    auto buffer = coded("    ");
    putUint32(buffer, 22 * 4, 32);
    putUint32(buffer, 23 * 4, 0x00ff0000);
    REQUIRE_THROWS_WITH(DDSLoader::resolveFormat(headerFor(buffer), buffer),
                        "THREE.DDSLoader.parse: Unsupported DDS format.");
}
