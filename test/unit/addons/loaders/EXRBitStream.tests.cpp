#include "loaders/EXRLoader.h"

#include "catch.hpp"

#include <cstdint>
#include <vector>

TEST_CASE("EXRLoader takes a whole byte of bits") {
    // Eight bits out of one byte leaves nothing live.
    std::size_t offset = 0;
    const auto result = EXRLoader::getBits(8, {}, {0xAB}, offset);
    REQUIRE(result.value == 0xAB);
    REQUIRE(result.state.liveBits == 0);
    REQUIRE(offset == 1);
}

TEST_CASE("EXRLoader takes half a byte twice") {
    // Four bits at a time gives the top half first and the bottom half second.
    const std::vector<std::uint8_t> bytes{0xAB};
    std::size_t offset = 0;
    const auto first = EXRLoader::getBits(4, {}, bytes, offset);
    REQUIRE(first.value == 0xA);
    REQUIRE(first.state.liveBits == 4);
    const auto second = EXRLoader::getBits(4, first.state, bytes, offset);
    REQUIRE(second.value == 0xB);
    REQUIRE(second.state.liveBits == 0);
    // Only one byte was read for the pair.
    REQUIRE(offset == 1);
}

TEST_CASE("EXRLoader takes bits across a byte boundary") {
    // Twelve bits need two bytes and leave four live.
    std::size_t offset = 0;
    const auto result = EXRLoader::getBits(12, {}, {0x12, 0x34}, offset);
    REQUIRE(result.value == 0x123);
    REQUIRE(result.state.liveBits == 4);
    REQUIRE(offset == 2);
    // The four left over are the low half of the second byte.
    const auto rest = EXRLoader::getBits(4, result.state, {0x12, 0x34}, offset);
    REQUIRE(rest.value == 0x4);
}

TEST_CASE("EXRLoader reads one byte at a time") {
    // Two characters make sixteen live bits.
    std::size_t offset = 0;
    const auto first = EXRLoader::getChar({}, {0x12, 0x34}, offset);
    REQUIRE(first.liveBits == 8);
    const auto second = EXRLoader::getChar(first, {0x12, 0x34}, offset);
    REQUIRE(second.liveBits == 16);
    // The accumulate word holds both bytes with the first at the top.
    REQUIRE(second.accumulate == 0x1234);
    REQUIRE(offset == 2);
}

TEST_CASE("EXRLoader reads a run from a full word") {
    // Four characters fill the accumulate word, whose top bit is then set. The runs that follow
    // take from the top of it and are masked, so the sign the shift brings in does not reach the
    // answer.
    const std::vector<std::uint8_t> bytes{0xFF, 0xFF, 0xFF, 0xFF, 0x00};
    std::size_t offset = 0;
    auto state = EXRLoader::getChar({}, bytes, offset);
    state = EXRLoader::getChar(state, bytes, offset);
    state = EXRLoader::getChar(state, bytes, offset);
    state = EXRLoader::getChar(state, bytes, offset);
    REQUIRE(state.liveBits == 32);
    // The word is all ones, which as a signed value is minus one.
    REQUIRE(state.accumulate == -1);

    // Taking eight bits from it gives all ones, not a sign extension.
    const auto top = EXRLoader::getBits(8, state, bytes, offset);
    REQUIRE(top.value == 0xFF);
    REQUIRE(top.state.liveBits == 24);
    // And again, down to the last byte.
    const auto next = EXRLoader::getBits(8, top.state, bytes, offset);
    REQUIRE(next.value == 0xFF);
    REQUIRE(next.state.liveBits == 16);
    // No further byte was read for either run.
    REQUIRE(offset == 4);
}
TEST_CASE("EXRLoader reads past the end as zero") {
    // A stream that runs out contributes zeros rather than failing.
    std::size_t offset = 0;
    const auto result = EXRLoader::getBits(8, {}, {}, offset);
    REQUIRE(result.value == 0);
    REQUIRE(result.state.liveBits == 0);
}
