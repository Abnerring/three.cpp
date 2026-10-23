#include "loaders/EXRLoader.h"

#include "catch.hpp"

#include <vector>

TEST_CASE("EXRLoader writes coefficients from the second slot") {
    // The first coefficient is written separately, so the scan begins at one.
    const auto scan = EXRLoader::unRleAC({5, 6}, 0);
    REQUIRE(scan.values[0] == 0);
    REQUIRE(scan.values[1] == 5);
    REQUIRE(scan.values[2] == 6);
    REQUIRE(scan.values[3] == 0);
    REQUIRE(scan.finalPosition == 3);
    REQUIRE(scan.wordsRead == 2);
}

TEST_CASE("EXRLoader ends a block on the ending word") {
    // The ending word stops the scan as if the block were full.
    const auto scan = EXRLoader::unRleAC({0xff00, 7}, 0);
    REQUIRE(scan.finalPosition == 64);
    // Only the ending word was read, so the seven after it was never reached.
    REQUIRE(scan.wordsRead == 1);
    REQUIRE(scan.values[1] == 0);
}

TEST_CASE("EXRLoader skips the number of coefficients a word names") {
    // A skip of three puts the next coefficient in the fifth slot, counting the first as one.
    const auto scan = EXRLoader::unRleAC({0xff03, 7}, 0);
    REQUIRE(scan.values[4] == 7);
    REQUIRE(scan.values[2] == 0);
    REQUIRE(scan.finalPosition == 5);
    // A skip of zero leaves the position where it was, so the next word fills the same slot.
    // The ending word again, showing the scan stops before reaching what follows.
    const auto ending = EXRLoader::unRleAC({0xff00, 0}, 0);
    REQUIRE(ending.wordsRead == 1);
}

TEST_CASE("EXRLoader stops when a skip runs past the block") {
    // A skip of a hundred and twenty seven carries the position well past the end.
    const auto scan = EXRLoader::unRleAC({0xff7f, 9}, 0);
    REQUIRE(scan.finalPosition > 64);
    REQUIRE(scan.wordsRead == 1);
    // The coefficient after the skip is never written.
    REQUIRE(scan.values[63] == 0);
}

TEST_CASE("EXRLoader starts where it is told") {
    // The starting word skips the first one.
    const auto scan = EXRLoader::unRleAC({99, 5}, 1);
    REQUIRE(scan.values[1] == 5);
    REQUIRE(scan.wordsRead == 1);
    // An empty buffer leaves the position at its start.
    const auto none = EXRLoader::unRleAC({}, 0);
    REQUIRE(none.finalPosition == 1);
    REQUIRE(none.wordsRead == 0);
}
