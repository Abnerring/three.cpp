#include "loaders/Zlib.h"

#include "catch.hpp"

#include <cstdint>
#include <vector>

TEST_CASE("Zlib reads the code length lengths in their order") {
    // Four values of three bits each, low one first: one, two, three, four. They are written into
    // the positions the order names, which begin sixteen, seventeen, eighteen and zero.
    // One, two, three and four packed as three bit fields, lowest field first: field n
    // starts at bit three n. That makes 0xD1 and 0x08.
    const std::vector<std::uint8_t> bytes{0xD1, 0x08};
    Zlib::BitReader reader{&bytes, 0, 0};
    const auto lengths = Zlib::readCodeLengthLengths(reader, 4);
    REQUIRE(lengths.size() == 19);
    REQUIRE(lengths[16] == 1);
    REQUIRE(lengths[17] == 2);
    REQUIRE(lengths[18] == 3);
    REQUIRE(lengths[0] == 4);
    // The positions not sent stay zero.
    REQUIRE(lengths[1] == 0);
    REQUIRE(lengths[15] == 0);
}

TEST_CASE("Zlib splits the lengths into two alphabets") {
    // The first count belongs to the literal table and the rest to the distance one.
    const auto split = Zlib::splitLengths({1, 2, 3, 4, 5}, 3, 2);
    REQUIRE(split.literal.size() == 3);
    REQUIRE(split.literal[0] == 1);
    REQUIRE(split.literal[2] == 3);
    REQUIRE(split.distance.size() == 2);
    REQUIRE(split.distance[0] == 4);
    REQUIRE(split.distance[1] == 5);
    // A vector shorter than the counts asked for gives only what is there.
    const auto short_ = Zlib::splitLengths({1, 2}, 3, 2);
    REQUIRE(short_.literal.size() == 2);
    REQUIRE(short_.distance.empty());
    // An empty vector gives two empty halves.
    const auto none = Zlib::splitLengths({}, 3, 2);
    REQUIRE(none.literal.empty());
    REQUIRE(none.distance.empty());
}

TEST_CASE("Zlib refuses to expand lengths from nothing") {
    // A table with one symbol of one bit, and an empty stream to read with.
    const auto table = Zlib::buildTable({1, 1});
    Zlib::BitReader empty;
    const auto result = Zlib::expandLengths(empty, table, 4);
    REQUIRE_FALSE(result.valid);
    REQUIRE_FALSE(result.error.empty());
    REQUIRE(result.lengths.empty());
}

TEST_CASE("Zlib reports the two length errors") {
    REQUIRE(Zlib::repeatFirstError() == "Zlib: a length run began before any length.");
    REQUIRE(Zlib::overrunError() == "Zlib: a length run reached past the count.");
    // The two are different, so a test cannot pass by getting either.
    REQUIRE(Zlib::repeatFirstError() != Zlib::overrunError());
}

TEST_CASE("Zlib expands a run of plain lengths") {
    // A code length table with two symbols of one bit, so a one bit names the second of them.
    // That symbol is the number one, which is also the length it names.
    const auto table = Zlib::buildTable({1, 1});
    const std::vector<std::uint8_t> ones{0xFF, 0xFF, 0xFF};
    Zlib::BitReader reader{&ones, 0, 0};
    const auto result = Zlib::expandLengths(reader, table, 4);
    REQUIRE(result.valid);
    REQUIRE(result.lengths.size() == 4);
    REQUIRE(result.lengths[0] == 1);
    REQUIRE(result.lengths[3] == 1);
}
