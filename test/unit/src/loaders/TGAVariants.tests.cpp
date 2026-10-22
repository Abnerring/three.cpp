#include "loaders/TGALoader.h"

#include "catch.hpp"

#include <cstdint>
#include <vector>

TEST_CASE("TGALoader expands eight bit grey") {
    const std::vector<std::uint8_t> pixels{100, 200};
    const auto rgba = TGALoader::toRGBAFromGrey8(pixels);
    REQUIRE(rgba.size() == 8);
    // A grey sample feeds all three channels and the pixel is opaque.
    REQUIRE(rgba[0] == 100);
    REQUIRE(rgba[1] == 100);
    REQUIRE(rgba[2] == 100);
    REQUIRE(rgba[3] == 255);
    REQUIRE(rgba[4] == 200);
}

TEST_CASE("TGALoader expands sixteen bit grey from the high byte") {
    const std::vector<std::uint8_t> pixels{0x11, 0x80};
    const auto rgba = TGALoader::toRGBAFromGrey16(pixels);
    REQUIRE(rgba.size() == 4);
    REQUIRE(rgba[0] == 0x80);
    REQUIRE(rgba[3] == 255);
}

TEST_CASE("TGALoader expands sixteen bit five five five colour") {
    // All channels at full gives 0x7fff, written little endian.
    const std::vector<std::uint8_t> pixels{0xff, 0x7f};
    const auto rgba = TGALoader::toRGBAFrom16(pixels);
    REQUIRE(rgba.size() == 4);
    // Five bits expanded by a shift of three, so thirty one becomes two hundred and forty eight.
    REQUIRE(rgba[0] == 248);
    REQUIRE(rgba[1] == 248);
    REQUIRE(rgba[2] == 248);

    // Red at full and the rest at zero.
    const std::vector<std::uint8_t> red{0x00, 0x7c};
    const auto redRgba = TGALoader::toRGBAFrom16(red);
    REQUIRE(redRgba[0] == 248);
    REQUIRE(redRgba[1] == 0);
    REQUIRE(redRgba[2] == 0);
}

TEST_CASE("TGALoader flips rows for an upper origin") {
    // A two by two image: row zero is red, row one is blue.
    std::vector<std::uint8_t> pixels{255, 0, 0, 255, 255, 0, 0, 255,
                                     0, 0, 255, 255, 0, 0, 255, 255};
    TGALoader::flipToOrigin(pixels, 2, 2, TGALoader::kOriginTopLeft);
    // The blue row is now first.
    REQUIRE(pixels[0] == 0);
    REQUIRE(pixels[2] == 255);
    REQUIRE(pixels[8] == 255);
    REQUIRE(pixels[10] == 0);
}

TEST_CASE("TGALoader flips columns for a right origin") {
    std::vector<std::uint8_t> pixels{255, 0, 0, 255, 0, 255, 0, 255};
    TGALoader::flipToOrigin(pixels, 2, 1, TGALoader::kOriginBottomRight);
    // Green is now first because the columns swapped.
    REQUIRE(pixels[0] == 0);
    REQUIRE(pixels[1] == 255);
    REQUIRE(pixels[4] == 255);
    REQUIRE(pixels[6] == 0);
}

TEST_CASE("TGALoader leaves a bottom left image alone") {
    const std::vector<std::uint8_t> original{1, 2, 3, 4, 5, 6, 7, 8};
    auto pixels = original;
    TGALoader::flipToOrigin(pixels, 2, 1, TGALoader::kOriginBottomLeft);
    REQUIRE(pixels == original);
}
