#include "loaders/TGALoader.h"

#include "catch.hpp"

#include <cstdint>
#include <vector>

TEST_CASE("TGALoader expands indexes through the colour map") {
    // Two entries in blue green red order: red first, then green.
    const std::vector<std::uint8_t> palette{0, 0, 255, 0, 255, 0};
    const std::vector<std::uint8_t> pixels{0, 1};
    const auto rgba = TGALoader::toRGBAFromIndexed(pixels, palette);
    REQUIRE(rgba.size() == 8);
    // Index zero is the red entry.
    REQUIRE(rgba[0] == 255);
    REQUIRE(rgba[1] == 0);
    REQUIRE(rgba[2] == 0);
    REQUIRE(rgba[3] == 255);
    // Index one is the green entry.
    REQUIRE(rgba[4] == 0);
    REQUIRE(rgba[5] == 255);
    REQUIRE(rgba[6] == 0);
}

TEST_CASE("TGALoader keeps an out of range index black") {
    const std::vector<std::uint8_t> palette{0, 0, 255};
    // Index two is past the single entry.
    const std::vector<std::uint8_t> pixels{2};
    const auto rgba = TGALoader::toRGBAFromIndexed(pixels, palette);
    REQUIRE(rgba.size() == 4);
    REQUIRE(rgba[0] == 0);
    REQUIRE(rgba[1] == 0);
    REQUIRE(rgba[2] == 0);
    REQUIRE(rgba[3] == 255);
}

TEST_CASE("TGALoader handles an empty palette") {
    const std::vector<std::uint8_t> pixels{0, 1, 2};
    const auto rgba = TGALoader::toRGBAFromIndexed(pixels, {});
    REQUIRE(rgba.size() == 12);
    // With no entries every pixel is black but still opaque.
    for (std::size_t i = 0; i < 3; ++i) {
        REQUIRE(rgba[i * 4 + 0] == 0);
        REQUIRE(rgba[i * 4 + 3] == 255);
    }
}
