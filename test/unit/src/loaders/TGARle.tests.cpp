#include "loaders/TGALoader.h"

#include "catch.hpp"

#include <cstdint>
#include <vector>

TEST_CASE("TGALoader expands a repeat packet") {
    // High bit set, low bits two, so three copies of one four byte pixel.
    const std::vector<std::uint8_t> data{0x82, 10, 20, 30, 40};
    const auto pixels = TGALoader::decodeRle(data, 0, 12, 4);
    REQUIRE(pixels.size() == 12);
    for (int i = 0; i < 3; ++i) {
        REQUIRE(pixels[static_cast<std::size_t>(i) * 4 + 0] == 10);
        REQUIRE(pixels[static_cast<std::size_t>(i) * 4 + 1] == 20);
        REQUIRE(pixels[static_cast<std::size_t>(i) * 4 + 3] == 40);
    }
}

TEST_CASE("TGALoader copies a raw packet") {
    // High bit clear, low bits two, so three separate pixels follow.
    const std::vector<std::uint8_t> data{0x02, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12};
    const auto pixels = TGALoader::decodeRle(data, 0, 12, 4);
    REQUIRE(pixels.size() == 12);
    REQUIRE(pixels[0] == 1);
    REQUIRE(pixels[4] == 5);
    REQUIRE(pixels[8] == 9);
    REQUIRE(pixels[11] == 12);
}

TEST_CASE("TGALoader mixes both packet kinds") {
    // One three byte pixel, then two raw three byte pixels.
    const std::vector<std::uint8_t> data{0x80, 7, 8, 9, 0x01, 1, 2, 3, 4, 5, 6};
    const auto pixels = TGALoader::decodeRle(data, 0, 9, 3);
    REQUIRE(pixels.size() == 9);
    REQUIRE(pixels[0] == 7);
    REQUIRE(pixels[2] == 9);
    REQUIRE(pixels[3] == 1);
    REQUIRE(pixels[8] == 6);
}

TEST_CASE("TGALoader reports truncated run length data") {
    // A repeat of four pixels that never supplies the pixel bytes.
    const std::vector<std::uint8_t> data{0x83};
    REQUIRE_THROWS_WITH(TGALoader::decodeRle(data, 0, 16, 4),
                        "THREE.TGALoader: Unexpected end of pixel data");
}

TEST_CASE("TGALoader rejects pixel depths it cannot decode") {
    TGALoader::Header header;
    header.imageType = TGALoader::kTypeRgb;
    header.width = 2;
    header.height = 2;
    // Fifteen bits is not one of the four supported depths.
    header.pixelSize = 15;
    REQUIRE_THROWS_WITH(TGALoader::checkHeader(header), "THREE.TGALoader: Invalid pixel size 15");
    for (const int depth : {8, 16, 24, 32}) {
        header.pixelSize = depth;
        REQUIRE_NOTHROW(TGALoader::checkHeader(header));
    }
}
