#include "loaders/EXRLoader.h"

#include "catch.hpp"

#include <cstdint>
#include <vector>

TEST_CASE("EXRLoader undoes the byte predictor") {
    // Each byte adds the one before it with a constant taken off. Ten and twenty give minus
    // ninety eight, which kept to eight bits is one hundred and fifty eight.
    std::vector<std::uint8_t> pair{10, 20};
    EXRLoader::revertPredictor(pair);
    REQUIRE(pair[0] == 10);
    REQUIRE(pair[1] == 158);
    // The first byte is never touched.
    std::vector<std::uint8_t> single{200};
    EXRLoader::revertPredictor(single);
    REQUIRE(single[0] == 200);
    // An empty block stays empty.
    std::vector<std::uint8_t> none;
    EXRLoader::revertPredictor(none);
    REQUIRE(none.empty());
    // The sum wraps rather than being clamped, so a large predecessor plus a large byte comes
    // back round to a small number rather than to two hundred and fifty five.
    std::vector<std::uint8_t> wrap{200, 200};
    EXRLoader::revertPredictor(wrap);
    REQUIRE(wrap[1] == 16);
    REQUIRE(wrap[1] != 255);
}

TEST_CASE("EXRLoader chains the predictor through the block") {
    // The third byte uses the second one as it has already become, not as it started.
    std::vector<std::uint8_t> run{0, 128, 128};
    EXRLoader::revertPredictor(run);
    // The first step gives zero, then that zero plus one hundred and twenty eight less the
    // constant gives zero again.
    REQUIRE(run[0] == 0);
    REQUIRE(run[1] == 0);
    REQUIRE(run[2] == 0);
    // A run of zeroes shifts every later byte down by one hundred and twenty eight each time.
    std::vector<std::uint8_t> zeros{0, 0, 0};
    EXRLoader::revertPredictor(zeros);
    REQUIRE(zeros[1] == 128);
    REQUIRE(zeros[2] == 0);
}

TEST_CASE("EXRLoader splits a block in half rounded up") {
    // The second half begins at halfway rounded up, so an odd length gives a shorter first half.
    REQUIRE(EXRLoader::interleaveSplit(0) == 0);
    REQUIRE(EXRLoader::interleaveSplit(1) == 1);
    REQUIRE(EXRLoader::interleaveSplit(2) == 1);
    REQUIRE(EXRLoader::interleaveSplit(3) == 2);
    REQUIRE(EXRLoader::interleaveSplit(4) == 2);
    REQUIRE(EXRLoader::interleaveSplit(5) == 3);
    REQUIRE(EXRLoader::interleaveSplit(6) == 3);
}

TEST_CASE("EXRLoader interleaves the two halves") {
    // Four bytes: the halves are one two and three four, taking one from each in turn.
    const auto four = EXRLoader::interleaveScalar({1, 2, 3, 4});
    REQUIRE(four.size() == 4);
    REQUIRE(four[0] == 1);
    REQUIRE(four[1] == 3);
    REQUIRE(four[2] == 2);
    REQUIRE(four[3] == 4);
    // Five bytes: the halves are one two three and four five, so the last comes from the shorter
    // first half.
    const auto five = EXRLoader::interleaveScalar({1, 2, 3, 4, 5});
    REQUIRE(five.size() == 5);
    REQUIRE(five[0] == 1);
    REQUIRE(five[1] == 4);
    REQUIRE(five[2] == 2);
    REQUIRE(five[3] == 5);
    REQUIRE(five[4] == 3);
    // The result holds every byte the source did, none lost and none added.
    REQUIRE(four.size() == 4);
    REQUIRE(five.size() == 5);
    // An empty block gives an empty block.
    REQUIRE(EXRLoader::interleaveScalar({}).empty());
    // One byte is itself.
    const auto one = EXRLoader::interleaveScalar({7});
    REQUIRE(one.size() == 1);
    REQUIRE(one[0] == 7);
}