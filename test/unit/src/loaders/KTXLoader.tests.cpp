#include "loaders/KTXLoader.h"

#include "catch.hpp"

#include <cstdint>
#include <vector>

namespace {
void putWord(std::vector<std::uint8_t>& out, int index, std::uint32_t value, bool little) {
    const std::size_t at = 12 + static_cast<std::size_t>(index) * 4;
    for (int i = 0; i < 4; ++i) {
        const int shift = little ? i * 8 : (3 - i) * 8;
        out[at + i] = std::uint8_t((value >> shift) & 0xff);
    }
}
// A complete little endian header with sensible fields.
std::vector<std::uint8_t> ktxHeader() {
    std::vector<std::uint8_t> out(64, 0);
    const std::uint8_t magic[12] = {0xAB, 0x4B, 0x54, 0x58, 0x20, 0x31,
                                    0x31, 0xBB, 0x0D, 0x0A, 0x1A, 0x0A};
    for (int i = 0; i < 12; ++i) out[static_cast<std::size_t>(i)] = magic[i];
    putWord(out, 0, 0x04030201, true);
    putWord(out, 1, 0, true);
    putWord(out, 2, 1, true);
    putWord(out, 3, 0, true);
    putWord(out, 4, 0x8D64, true);
    putWord(out, 5, 0x1908, true);
    putWord(out, 6, 128, true);
    putWord(out, 7, 64, true);
    putWord(out, 8, 0, true);
    putWord(out, 9, 0, true);
    putWord(out, 10, 1, true);
    putWord(out, 11, 3, true);
    putWord(out, 12, 48, true);
    return out;
}
}  // namespace

TEST_CASE("KTXLoader reads a valid header") {
    const auto header = KTXLoader::readHeader(ktxHeader());
    REQUIRE(header.valid);
    REQUIRE(header.littleEndian);
    REQUIRE(header.glType == 0);
    REQUIRE(header.glTypeSize == 1);
    REQUIRE(header.glInternalFormat == 0x8D64);
    REQUIRE(header.glBaseInternalFormat == 0x1908);
    REQUIRE(header.pixelWidth == 128);
    REQUIRE(header.pixelHeight == 64);
    REQUIRE(header.numberOfFaces == 1);
    REQUIRE(header.numberOfMipmapLevels == 3);
    REQUIRE(header.bytesOfKeyValueData == 48);
}

TEST_CASE("KTXLoader rejects a file without the identifier") {
    auto buffer = ktxHeader();
    buffer[0] = 0;
    const auto header = KTXLoader::readHeader(buffer);
    REQUIRE_FALSE(header.valid);
    REQUIRE(header.error == "texture missing KTX identifier");
    // A buffer too short for the identifier is rejected the same way.
    REQUIRE_FALSE(KTXLoader::readHeader({1, 2, 3}).valid);
}

TEST_CASE("KTXLoader rejects an uncompressed texture") {
    auto buffer = ktxHeader();
    putWord(buffer, 1, 0x1401, true);
    const auto header = KTXLoader::readHeader(buffer);
    REQUIRE_FALSE(header.valid);
    REQUIRE(header.error == "only compressed formats currently supported");
}

TEST_CASE("KTXLoader rejects a three dimensional texture") {
    auto buffer = ktxHeader();
    putWord(buffer, 8, 4, true);
    const auto header = KTXLoader::readHeader(buffer);
    REQUIRE_FALSE(header.valid);
    REQUIRE(header.error == "only 2D textures currently supported");
}

TEST_CASE("KTXLoader rejects an array texture") {
    auto buffer = ktxHeader();
    putWord(buffer, 9, 2, true);
    REQUIRE(KTXLoader::readHeader(buffer).error == "texture arrays not currently supported");
}

TEST_CASE("KTXLoader rejects the wrong number of faces") {
    auto buffer = ktxHeader();
    putWord(buffer, 10, 6, true);
    // One face was expected but the file declares six.
    const auto header = KTXLoader::readHeader(buffer);
    REQUIRE_FALSE(header.valid);
    REQUIRE(header.error == "number of faces expected1, but found 6");
    // Asking for six accepts it.
    REQUIRE(KTXLoader::readHeader(buffer, 6).valid);
}

TEST_CASE("KTXLoader assumes at least one level") {
    auto buffer = ktxHeader();
    putWord(buffer, 11, 0, true);
    REQUIRE(KTXLoader::readHeader(buffer).numberOfMipmapLevels == 1);
}

TEST_CASE("KTXLoader reads a big endian header") {
    std::vector<std::uint8_t> buffer = ktxHeader();
    // The endianness word is always read little endian, so a different value means the rest
    // of the header is big endian.
    putWord(buffer, 0, 0x01020304, true);
    putWord(buffer, 6, 256, false);
    putWord(buffer, 7, 512, false);
    const auto header = KTXLoader::readHeader(buffer);
    REQUIRE_FALSE(header.littleEndian);
    REQUIRE(header.pixelWidth == 256);
    REQUIRE(header.pixelHeight == 512);
}
