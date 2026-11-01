#include "loaders/EXRLoader.h"

#include "catch.hpp"

#include <cstdint>
#include <vector>

TEST_CASE("EXRLoader always lists zero in a reverse table") {
    // A bitmap with nothing set still lists zero.
    const auto empty = EXRLoader::reverseLutFromBitmap({});
    REQUIRE(empty.lut[0] == 0);
    // The count is one less than the listing, so a lone zero gives a last index of zero.
    REQUIRE(empty.lastIndex == 0);
}

TEST_CASE("EXRLoader lists the values whose bit is set") {
    // The first bit of the first byte stands for the value zero, which is listed anyway, so this
    // leaves only the one entry.
    const auto onlyZero = EXRLoader::reverseLutFromBitmap({0x01});
    REQUIRE(onlyZero.lut[0] == 0);
    REQUIRE(onlyZero.lut[1] == 0);
    REQUIRE(onlyZero.lastIndex == 0);
    // The second bit stands for the value one, which is added after the zero.
    const auto zeroAndOne = EXRLoader::reverseLutFromBitmap({0x02});
    REQUIRE(zeroAndOne.lut[0] == 0);
    REQUIRE(zeroAndOne.lut[1] == 1);
    REQUIRE(zeroAndOne.lastIndex == 1);
    // The highest bit of the first byte stands for seven.
    const auto withSeven = EXRLoader::reverseLutFromBitmap({0x80});
    REQUIRE(withSeven.lut[1] == 7);
    REQUIRE(withSeven.lastIndex == 1);
}

TEST_CASE("EXRLoader replaces values through a table") {
    const std::vector<std::uint16_t> lut{10, 20, 30};
    std::vector<std::uint16_t> data{0, 1, 2};
    EXRLoader::applyLut(lut, data);
    REQUIRE(data[0] == 10);
    REQUIRE(data[1] == 20);
    REQUIRE(data[2] == 30);
    // A value outside the table is left alone rather than reading past it.
    std::vector<std::uint16_t> wide{5};
    EXRLoader::applyLut(lut, wide);
    REQUIRE(wide[0] == 5);
}

TEST_CASE("EXRLoader undoes a differencing pass") {
    // Each value becomes the running total with one hundred and twenty eight off per step.
    std::vector<std::uint16_t> data{200, 150, 100};
    EXRLoader::predictor(data);
    REQUIRE(data[0] == 200);
    REQUIRE(data[1] == 222);
    REQUIRE(data[2] == 194);
    // A single value has nothing to add to it.
    std::vector<std::uint16_t> one{42};
    EXRLoader::predictor(one);
    REQUIRE(one[0] == 42);
}

TEST_CASE("EXRLoader deals the two halves out alternately") {
    // An even run takes one from each half in turn.
    std::vector<std::uint16_t> out(4);
    EXRLoader::interleaveScalar({1, 2, 3, 4}, out);
    REQUIRE(out[0] == 1);
    REQUIRE(out[1] == 3);
    REQUIRE(out[2] == 2);
    REQUIRE(out[3] == 4);
    // An odd run leaves the first half one longer, which the second start accounts for.
    std::vector<std::uint16_t> odd(3);
    EXRLoader::interleaveScalar({1, 2, 3}, odd);
    REQUIRE(odd[0] == 1);
    REQUIRE(odd[1] == 3);
    REQUIRE(odd[2] == 2);
    // An empty run writes nothing.
    std::vector<std::uint16_t> none;
    EXRLoader::interleaveScalar({}, none);
    REQUIRE(none.empty());
}