#include "loaders/Zlib.h"

#include "catch.hpp"

#include <array>
#include <cstdint>

TEST_CASE("Zlib reads the code lengths in their own order") {
    // Nineteen entries, none of them repeated.
    const auto& order = Zlib::codeLengthOrder();
    REQUIRE(order.size() == 19);
    // The first three are the repeat codes, which are numbered highest.
    REQUIRE(order[0] == 16);
    REQUIRE(order[1] == 17);
    REQUIRE(order[2] == 18);
    // Zero comes fourth rather than first, which is the part easiest to get wrong.
    REQUIRE(order[3] == 0);
    // The last is fifteen.
    REQUIRE(order[18] == 15);
    // Every position from zero to eighteen appears exactly once.
    std::array<bool, 19> seen{};
    for (const int value : order) {
        REQUIRE(value >= 0);
        REQUIRE(value < 19);
        REQUIRE_FALSE(seen[static_cast<std::size_t>(value)]);
        seen[static_cast<std::size_t>(value)] = true;
    }
}

TEST_CASE("Zlib removes the bias from the three counts") {
    // All zero fields give the smallest counts the format allows.
    const auto smallest = Zlib::dynamicCounts(0, 0, 0);
    REQUIRE(smallest.literalCount == 257);
    REQUIRE(smallest.distanceCount == 1);
    REQUIRE(smallest.codeLengthCount == 4);
    // The largest fields give the largest counts, which are thirty two values of five bits minus
    // one plus the bias.
    const auto largest = Zlib::dynamicCounts(31, 31, 15);
    REQUIRE(largest.literalCount == 288);
    REQUIRE(largest.distanceCount == 32);
    REQUIRE(largest.codeLengthCount == 19);
    // A mixed set adds each bias on its own.
    const auto mixed = Zlib::dynamicCounts(3, 2, 1);
    REQUIRE(mixed.literalCount == 260);
    REQUIRE(mixed.distanceCount == 3);
    REQUIRE(mixed.codeLengthCount == 5);
}

TEST_CASE("Zlib counts what each repeat code stands for") {
    // Sixteen copies the previous length three to six times, so it lands in that range.
    REQUIRE(Zlib::repeatCount(16, 0) == 3);
    REQUIRE(Zlib::repeatCount(16, 3) == 6);
    // Seventeen repeats a zero three to ten times.
    REQUIRE(Zlib::repeatCount(17, 0) == 3);
    REQUIRE(Zlib::repeatCount(17, 7) == 10);
    // Eighteen repeats a zero eleven to a hundred and thirty eight times.
    REQUIRE(Zlib::repeatCount(18, 0) == 11);
    REQUIRE(Zlib::repeatCount(18, 127) == 138);
    // The first two ranges do overlap, since sixteen reaches six and seventeen starts at
    // three; only the third is clear of them.
    REQUIRE(Zlib::repeatCount(16, 3) == 6);
    REQUIRE(Zlib::repeatCount(17, 0) == 3);
    REQUIRE(Zlib::repeatCount(17, 7) == 10);
    REQUIRE(Zlib::repeatCount(18, 0) == 11);
    // Any other code repeats nothing.
    REQUIRE(Zlib::repeatCount(15, 0) == 0);
    REQUIRE(Zlib::repeatCount(0, 5) == 0);
}
