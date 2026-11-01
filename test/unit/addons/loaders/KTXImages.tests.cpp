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
// A valid little endian header followed by one size word and its data per level.
std::vector<std::uint8_t> fileWith(std::uint32_t levels, std::uint32_t faces, std::uint32_t width,
                                   std::uint32_t height, std::uint32_t keyBytes,
                                   const std::vector<std::uint32_t>& sizes) {
    std::size_t total = 64 + keyBytes;
    for (const auto size : sizes) {
        const std::size_t padding = 3 - ((static_cast<std::size_t>(size) + 3) % 4);
        total += 4 + (size + padding) * faces;
    }
    std::vector<std::uint8_t> out(total, 0);
    const std::uint8_t magic[12] = {0xAB, 0x4B, 0x54, 0x58, 0x20, 0x31,
                                    0x31, 0xBB, 0x0D, 0x0A, 0x1A, 0x0A};
    for (int i = 0; i < 12; ++i) out[static_cast<std::size_t>(i)] = magic[i];
    putWord(out, 0, 0x04030201, true);
    putWord(out, 2, 1, true);
    putWord(out, 4, 0x8D64, true);
    putWord(out, 6, width, true);
    putWord(out, 7, height, true);
    putWord(out, 10, faces, true);
    putWord(out, 11, levels, true);
    putWord(out, 12, keyBytes, true);

    // Each level writes its size word at the cursor, then the images that follow.
    std::size_t at = 64 + keyBytes;
    for (const auto size : sizes) {
        for (int i = 0; i < 4; ++i)
            out[at + static_cast<std::size_t>(i)] = std::uint8_t((size >> (i * 8)) & 0xff);
        at += 4;
        const std::size_t padding = 3 - ((static_cast<std::size_t>(size) + 3) % 4);
        at += (size + padding) * faces;
    }
    return out;
}
}  // namespace

TEST_CASE("KTXLoader walks the image list") {
    // Two levels of one face, a hundred bytes then fifty six.
    const auto buffer = fileWith(2, 1, 128, 64, 0, {100, 56});
    const auto header = KTXLoader::readHeader(buffer);
    REQUIRE(header.valid);
    const auto images = KTXLoader::readImages(buffer, header);
    REQUIRE(images.size() == 2);
    // The data starts after the header and the size word that introduces the level.
    REQUIRE(images[0].offset == 68);
    REQUIRE(images[0].imageSize == 100);
    REQUIRE(images[0].width == 128);
    // A hundred is a multiple of four, so no padding follows.
    REQUIRE(images[0].padding == 0);
    // The second level halves both dimensions and follows the first image.
    REQUIRE(images[1].width == 64);
    REQUIRE(images[1].height == 32);
    REQUIRE(images[1].offset == 172);
}

TEST_CASE("KTXLoader pads an image to four bytes") {
    // A size of one hundred and one needs three bytes of padding.
    const auto buffer = fileWith(2, 1, 8, 8, 0, {101, 100});
    const auto header = KTXLoader::readHeader(buffer);
    const auto images = KTXLoader::readImages(buffer, header);
    REQUIRE(images[0].padding == 3);
    REQUIRE(images[1].offset == 176);
}

TEST_CASE("KTXLoader skips the key value pairs") {
    // Sixteen bytes of metadata move the first size word along.
    const auto buffer = fileWith(1, 1, 4, 4, 16, {32});
    const auto header = KTXLoader::readHeader(buffer);
    const auto images = KTXLoader::readImages(buffer, header);
    REQUIRE(images.size() == 1);
    REQUIRE(images[0].offset == 84);
}

TEST_CASE("KTXLoader reports one image per face") {
    // Six faces at one level, each sixteen bytes and already aligned.
    const auto buffer = fileWith(1, 6, 4, 4, 0, {16});
    const auto header = KTXLoader::readHeader(buffer, 6);
    REQUIRE(header.valid);
    const auto images = KTXLoader::readImages(buffer, header);
    REQUIRE(images.size() == 6);
    REQUIRE(images[0].offset == 68);
    REQUIRE(images[1].offset == 84);
    REQUIRE(images[5].offset == 148);
}

TEST_CASE("KTXLoader honours the mipmap switch") {
    const auto buffer = fileWith(3, 1, 8, 8, 0, {64, 32, 16});
    const auto header = KTXLoader::readHeader(buffer);
    // With the switch off only the first level is reported.
    REQUIRE(KTXLoader::readImages(buffer, header, false).size() == 1);
    REQUIRE(KTXLoader::readImages(buffer, header, true).size() == 3);
}

TEST_CASE("KTXLoader returns nothing for an invalid header") {
    const std::vector<std::uint8_t> buffer(64, 0);
    const auto header = KTXLoader::readHeader(buffer);
    REQUIRE_FALSE(header.valid);
    REQUIRE(KTXLoader::readImages(buffer, header).empty());
}
