#include "loaders/EXRLoader.h"

#include "catch.hpp"

#include <cstdint>
#include <vector>

TEST_CASE("EXRLoader begins the wavelet at the largest power of two") {
    // Two gives a previous of two and a first step of one.
    const auto two = EXRLoader::pizWaveletStart(2);
    REQUIRE(two.previous == 2);
    REQUIRE(two.step == 1);
    // Eight gives four and eight.
    const auto eight = EXRLoader::pizWaveletStart(8);
    REQUIRE(eight.previous == 8);
    REQUIRE(eight.step == 4);
    // One gives a previous of one and no step at all, since one doubled already passes it.
    const auto one = EXRLoader::pizWaveletStart(1);
    REQUIRE(one.previous == 1);
    REQUIRE(one.step == 0);
    // Five fits four, so the previous is four and the step is two.
    const auto five = EXRLoader::pizWaveletStart(5);
    REQUIRE(five.previous == 4);
    REQUIRE(five.step == 2);
}

TEST_CASE("EXRLoader leaves a block of zeroes alone") {
    // The wide arithmetic turns a pair of zeroes into zero, so a block of zeroes stays as it
    // was however many steps it takes. The narrow one would not: it turns zero into the offset.
    std::vector<std::int32_t> block(4, 0);
    EXRLoader::wav2Decode(block, 0, 2, 1, 2, 2, 20000);
    for (const auto value : block) REQUIRE(value == 0);
    // A larger block likewise.
    std::vector<std::int32_t> larger(16, 0);
    EXRLoader::wav2Decode(larger, 0, 4, 1, 4, 4, 20000);
    for (const auto value : larger) REQUIRE(value == 0);
}

TEST_CASE("EXRLoader decodes a two by two block") {
    // Four samples in a square, read in the order the four places are numbered: the top left,
    // the top right below it, and so on. With the wide arithmetic a pair of zeroes gives zero.
    std::vector<std::int32_t> block{0, 0, 0, 0};
    EXRLoader::wav2Decode(block, 0, 2, 1, 2, 2, 20000);
    for (const auto value : block) REQUIRE(value == 0);
    // One step is what a two by two needs, so nothing is left for a second.
    const auto start = EXRLoader::pizWaveletStart(2);
    REQUIRE(start.step == 1);
}

TEST_CASE("EXRLoader decodes a single sample") {
    // A one by one block has no step to take, so the value passes through untouched.
    std::vector<std::int32_t> block{7};
    EXRLoader::wav2Decode(block, 0, 1, 1, 1, 1, 20000);
    REQUIRE(block.size() == 1);
    REQUIRE(block[0] == 7);
    // A block of one sample keeps whatever it held, whatever the range says.
    std::vector<std::int32_t> wide{9};
    EXRLoader::wav2Decode(wide, 0, 1, 1, 1, 1, 60000);
    REQUIRE(wide[0] == 9);
}
