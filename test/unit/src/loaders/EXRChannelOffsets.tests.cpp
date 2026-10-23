#include "loaders/EXRLoader.h"

#include "catch.hpp"

#include <cstdint>
#include <cstddef>
#include <string>
#include <utility>
#include <vector>

TEST_CASE("EXRLoader sizes a channel's samples") {
    // The same doubling as the block compression: a half is two bytes, a full value four.
    REQUIRE(EXRLoader::channelSampleBytes(1) == 2);
    REQUIRE(EXRLoader::channelSampleBytes(2) == 4);
    REQUIRE(EXRLoader::channelSampleBytes(0) == 0);
}

TEST_CASE("EXRLoader gives only the wanted channels a place") {
    // Three channels of a half each: the wanted ones take two bytes apiece and begin where the
    // one before ended.
    const std::vector<std::pair<std::string, int>> channels{{"R", 1}, {"G", 1}, {"B", 1}};
    const auto offsets = EXRLoader::channelByteOffsets(channels, {"R", "G", "B"});
    REQUIRE(offsets.size() == 3);
    REQUIRE(offsets[0].second == 0);
    REQUIRE(offsets[1].second == 2);
    REQUIRE(offsets[2].second == 4);
    // Wanting only the last one gives it the place it would have had anyway, since the two
    // before it still take room.
    const auto last = EXRLoader::channelByteOffsets(channels, {"B"});
    REQUIRE(last.size() == 1);
    REQUIRE(last[0].first == "B");
    REQUIRE(last[0].second == 4);
    // Wanting none gives none.
    REQUIRE(EXRLoader::channelByteOffsets(channels, {}).empty());
}

TEST_CASE("EXRLoader adds up the bytes of a line") {
    // Three halves are six bytes and three full values twelve.
    REQUIRE(EXRLoader::totalBytesPerSample({1, 1, 1}) == 6);
    REQUIRE(EXRLoader::totalBytesPerSample({2, 2, 2}) == 12);
    // A mixture adds each on its own.
    REQUIRE(EXRLoader::totalBytesPerSample({1, 2}) == 6);
    REQUIRE(EXRLoader::totalBytesPerSample({}) == 0);
}

TEST_CASE("EXRLoader widens a line by the output's channels") {
    // A line of a hundred pixels of four channels is four hundred places wide.
    REQUIRE(EXRLoader::outputLineWidth(100, 4) == 400);
    REQUIRE(EXRLoader::outputLineWidth(100, 3) == 300);
    REQUIRE(EXRLoader::outputLineWidth(0, 4) == 0);
}

TEST_CASE("EXRLoader fills a missing alpha with opaque") {
    // The byte output holds one as the plain integer and the half output as its own form.
    REQUIRE(EXRLoader::kByteAlphaOpaque == 1);
    REQUIRE(EXRLoader::kHalfAlphaOpaque == 0x3C00);
    // The two are different numbers for the same value, which is the point of having both.
    REQUIRE(EXRLoader::kByteAlphaOpaque != EXRLoader::kHalfAlphaOpaque);
    // The half form has its exponent set and its mantissa clear, as a power of two does.
    REQUIRE((EXRLoader::kHalfAlphaOpaque & 0x03FF) == 0);
    REQUIRE((EXRLoader::kHalfAlphaOpaque >> 10) == 0x0F);
}