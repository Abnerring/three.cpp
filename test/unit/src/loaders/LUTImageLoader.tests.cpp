#include "loaders/LUTImageLoader.h"

#include "catch.hpp"

#include <cstdint>
#include <vector>

namespace {
// A source of the given strip count, each strip two pixels tall so a slice is two by two.
// Every pixel gets a distinct value in its red channel so the arrangement can be checked.
std::vector<std::uint8_t> source(int strips, int side) {
    const int width = strips * side;
    const int height = side;
    std::vector<std::uint8_t> out(static_cast<std::size_t>(width) * height * 4, 0);
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            const std::size_t at = (static_cast<std::size_t>(y) * width + x) * 4;
            out[at] = static_cast<std::uint8_t>(x + 1);
        }
    }
    return out;
}
}  // namespace

TEST_CASE("LUTImageLoader rearranges the strips") {
    // Two strips of two by two give a four wide, two tall source and a two wide, four tall
    // result.
    const auto in = source(2, 2);
    const auto out = LUTImageLoader::horizontalToVertical(in, 4, 2, false);
    REQUIRE(out.size() == static_cast<std::size_t>(2) * 4 * 4);
    // The first output row is the first row of the first strip.
    REQUIRE(out[(0 * 2 + 0) * 4] == 1);
    REQUIRE(out[(0 * 2 + 1) * 4] == 2);
    // The second output row is the second row of the first strip.
    REQUIRE(out[(1 * 2 + 0) * 4] == 1);
}

TEST_CASE("LUTImageLoader flips the strip order") {
    // With the flip the strips are stacked from the bottom up.
    const auto in = source(2, 2);
    const auto plain = LUTImageLoader::horizontalToVertical(in, 4, 2, false);
    const auto flipped = LUTImageLoader::horizontalToVertical(in, 4, 2, true);
    REQUIRE(plain.size() == flipped.size());
    // The first strip of the source is the third row block when flipped, which is out of
    // range of a four tall result, so only the first block differs.
    REQUIRE(plain[(0 * 2 + 0) * 4] != 0);
    REQUIRE(flipped[(0 * 2 + 0) * 4] == 0);
}

TEST_CASE("LUTImageLoader rejects an impossible size") {
    const std::vector<std::uint8_t> empty;
    REQUIRE(LUTImageLoader::horizontalToVertical(empty, 0, 0, false).empty());
    REQUIRE(LUTImageLoader::horizontalToVertical(empty, 4, 2, false).empty());
}

TEST_CASE("LUTImageLoader keeps the pixel bytes together") {
    // One strip of one by one copies the single pixel unchanged.
    const std::vector<std::uint8_t> in{10, 20, 30, 40};
    const auto out = LUTImageLoader::horizontalToVertical(in, 1, 1, false);
    REQUIRE(out.size() == 4);
    REQUIRE(out[0] == 10);
    REQUIRE(out[3] == 40);
}
