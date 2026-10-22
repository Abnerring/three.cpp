#include "loaders/LUT3dlLoader.h"

#include "catch.hpp"

#include <stdexcept>
#include <string>
#include <vector>

namespace {
const char* kLut =
    "0 255\n"
    "0 0 0\n"
    "255 0 0\n"
    "0 255 0\n"
    "255 255 0\n"
    "0 0 255\n"
    "255 0 255\n"
    "0 255 255\n"
    "255 255 255\n";
}

TEST_CASE("LUT3dlLoader derives the size from the grid line") {
    LUT3dlLoader loader;
    const auto result = loader.parse(kLut);
    // Two positions on the grid line means a 2x2x2 LUT.
    REQUIRE(result.size == 2);
    REQUIRE(result.texture3D != nullptr);
    const auto& image = *result.texture3D->image.get();
    REQUIRE(image.width == 2);
    REQUIRE(image.height == 2);
    REQUIRE(image.depth == 2);
    REQUIRE(std::get<Uint8Array>(image.data).size() == 32);
}

TEST_CASE("LUT3dlLoader reorders to blue green red and scales by bit depth") {
    LUT3dlLoader loader;
    const auto result = loader.parse(kLut);
    const auto& bytes = std::get<Uint8Array>(result.texture3D->image.get()->data);
    // Upstream scales by the next power of two above the largest sample, so 255 becomes
    // 255 / 256 * 255 which truncates to 254.
    const std::vector<std::uint8_t> expected{
        0,   0,   0,   255, 0,   0,   254, 255, 0,   254, 0,   255, 0,   254, 254, 255,
        254, 0,   0,   255, 254, 0,   254, 255, 254, 254, 0,   255, 254, 254, 254, 255};
    REQUIRE(bytes.size() == expected.size());

    for (std::size_t i = 0; i < expected.size(); ++i)
        REQUIRE(bytes[i] == expected[i]);
}

TEST_CASE("LUT3dlLoader reports the upstream errors") {
    LUT3dlLoader loader;
    // A line of numbers is a valid grid line, so the missing grid case needs input that has
    // no digits-and-spaces line at all.
    REQUIRE_THROWS_WITH(loader.parse("no grid here\n"),
                        "THREE.LUT3dlLoader: Missing grid information");
    REQUIRE_THROWS_WITH(loader.parse("0 128 300\n0 0 0\n"),
                        "THREE.LUT3dlLoader: Inconsistent grid size");
}
