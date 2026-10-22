#include "loaders/TGALoader.h"

#include "catch.hpp"

#include <cstdint>
#include <vector>

TEST_CASE("TGALoader reads the origin from the descriptor") {
    // The two bits at four and five give bottom left, bottom right, top left and top right.
    REQUIRE(TGALoader::originOf(0x00) == TGALoader::kOriginBottomLeft);
    REQUIRE(TGALoader::originOf(0x10) == TGALoader::kOriginBottomRight);
    REQUIRE(TGALoader::originOf(0x20) == TGALoader::kOriginTopLeft);
    REQUIRE(TGALoader::originOf(0x30) == TGALoader::kOriginTopRight);
    // Other bits are ignored, so an alpha depth of eight does not change the origin.
    REQUIRE(TGALoader::originOf(0x28) == TGALoader::kOriginTopLeft);
}

TEST_CASE("TGALoader converts 24 bit blue green red to red green blue") {
    // One pixel, blue ten, green twenty, red thirty.
    const std::vector<std::uint8_t> pixels{10, 20, 30};
    const auto rgba = TGALoader::toRGBA(pixels, 24);
    REQUIRE(rgba.size() == 4);
    REQUIRE(rgba[0] == 30);
    REQUIRE(rgba[1] == 20);
    REQUIRE(rgba[2] == 10);
    // A 24 bit pixel has no alpha, so it becomes opaque.
    REQUIRE(rgba[3] == 255);
}

TEST_CASE("TGALoader converts 32 bit blue green red alpha") {
    const std::vector<std::uint8_t> pixels{10, 20, 30, 40};
    const auto rgba = TGALoader::toRGBA(pixels, 32);
    REQUIRE(rgba.size() == 4);
    REQUIRE(rgba[0] == 30);
    REQUIRE(rgba[1] == 20);
    REQUIRE(rgba[2] == 10);
    // The stored alpha survives.
    REQUIRE(rgba[3] == 40);
}

TEST_CASE("TGALoader converts several pixels") {
    const std::vector<std::uint8_t> pixels{1, 2, 3, 4, 5, 6, 7, 8};
    const auto rgba = TGALoader::toRGBA(pixels, 32);
    REQUIRE(rgba.size() == 8);
    REQUIRE(rgba[0] == 3);
    REQUIRE(rgba[4] == 7);
    REQUIRE(rgba[7] == 8);
}

TEST_CASE("TGALoader refuses depths it cannot expand") {
    const std::vector<std::uint8_t> pixels{1, 2};
    // Eight and sixteen bit pixels do not have three channels, so nothing comes back.
    REQUIRE(TGALoader::toRGBA(pixels, 16).empty());
}
