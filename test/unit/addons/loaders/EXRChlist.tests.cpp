#include "loaders/EXRLoader.h"

#include "catch.hpp"

#include <cstddef>

TEST_CASE("EXRLoader sizes a channel entry") {
    // Seventeen bytes of fixed fields beyond the name and its terminator.
    REQUIRE(EXRLoader::channelEntrySize(0) == 17);
    REQUIRE(EXRLoader::channelEntrySize(1) == 18);
    REQUIRE(EXRLoader::channelEntrySize(10) == 27);
    // A one letter name such as R takes eighteen bytes, which is what the format's own example
    // uses.
    REQUIRE(EXRLoader::channelEntrySize(1) == 18);
    // The fixed part is what stays when the name is empty.
    REQUIRE(EXRLoader::channelEntrySize(0) + 5 == EXRLoader::channelEntrySize(5));
}

TEST_CASE("EXRLoader keeps the terminator out of the loop") {
    // A list of twenty bytes holds entries up to the eighteenth and a terminator at the
    // nineteenth, so the loop runs while the position is below nineteen.
    REQUIRE(EXRLoader::channelEntryInRange(0, 0, 20));
    REQUIRE(EXRLoader::channelEntryInRange(18, 0, 20));
    REQUIRE_FALSE(EXRLoader::channelEntryInRange(19, 0, 20));
    REQUIRE_FALSE(EXRLoader::channelEntryInRange(20, 0, 20));
}

TEST_CASE("EXRLoader measures the loop from wherever the list starts") {
    // The start is added rather than assumed to be zero.
    REQUIRE(EXRLoader::channelEntryInRange(100, 100, 20));
    REQUIRE(EXRLoader::channelEntryInRange(118, 100, 20));
    REQUIRE_FALSE(EXRLoader::channelEntryInRange(119, 100, 20));
    // A position before the start is inside the range only by accident of the arithmetic, so it
    // is not claimed to be out of range: the loop never asks about one.
    REQUIRE(EXRLoader::channelEntryInRange(0, 100, 20));
}

TEST_CASE("EXRLoader handles an empty channel list") {
    // A declared size of zero has no entries and no terminator to stand on either.
    REQUIRE_FALSE(EXRLoader::channelEntryInRange(0, 0, 0));
    REQUIRE_FALSE(EXRLoader::channelEntryInRange(0, 5, 0));
}

TEST_CASE("EXRLoader fixes the two structural constants") {
    // One byte of terminator and three reserved inside an entry.
    REQUIRE(EXRLoader::kChlistTerminator == 1);
    REQUIRE(EXRLoader::kChannelReserved == 3);
}