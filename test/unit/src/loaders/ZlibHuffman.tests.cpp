#include "loaders/Zlib.h"

#include "catch.hpp"

#include <cstdint>
#include <vector>

TEST_CASE("Zlib builds a table from code lengths") {
    // Two symbols of one bit each take the codes zero and one.
    const auto pair = Zlib::buildTable({1, 1});
    REQUIRE(pair.maxLength == 1);
    REQUIRE(pair.byLength[1].size() == 2);
    // A symbol that does not appear is not entered.
    const auto single = Zlib::buildTable({0, 1});
    REQUIRE(single.maxLength == 1);
    REQUIRE(single.byLength[1].size() == 1);
    REQUIRE(single.byLength[1][0].second == 1);
    // An empty alphabet has no entries and no length.
    const auto empty = Zlib::buildTable({});
    REQUIRE(empty.maxLength == 0);
    REQUIRE(empty.byLength[1].empty());
}

TEST_CASE("Zlib decodes a symbol from the stream") {
    // With two one bit codes a single zero bit names the first symbol.
    const auto table = Zlib::buildTable({1, 1});
    const std::vector<std::uint8_t> zero{0x00};
    Zlib::BitReader a{&zero, 0, 0};
    const auto first = Zlib::decodeSymbol(a, table);
    REQUIRE(first.found);
    REQUIRE(first.symbol == 0);
    REQUIRE(first.bitsUsed == 1);
    // A single one bit names the second.
    const std::vector<std::uint8_t> one{0x01};
    Zlib::BitReader b{&one, 0, 0};
    const auto second = Zlib::decodeSymbol(b, table);
    REQUIRE(second.found);
    REQUIRE(second.symbol == 1);
    REQUIRE(second.bitsUsed == 1);
}

TEST_CASE("Zlib turns a code round when it builds the table") {
    // Four symbols of two bits take the codes zero, one, two and three, but each is stored with
    // its bits turned round, so symbol two's code is one in the stream.
    const auto table = Zlib::buildTable({2, 2, 2, 2});
    REQUIRE(table.maxLength == 2);
    // The stream bits 1 then 0 build the value one, which names symbol two.
    const std::vector<std::uint8_t> bits{0b01};
    Zlib::BitReader reader{&bits, 0, 0};
    const auto result = Zlib::decodeSymbol(reader, table);
    REQUIRE(result.found);
    REQUIRE(result.symbol == 2);
    REQUIRE(result.bitsUsed == 2);
}

TEST_CASE("Zlib consumes only the bits a symbol needs") {
    // One bit for the first symbol leaves seven in the byte.
    const auto table = Zlib::buildTable({1, 1});
    const std::vector<std::uint8_t> bytes{0x00};
    Zlib::BitReader reader{&bytes, 0, 0};
    Zlib::decodeSymbol(reader, table);
    REQUIRE(reader.used == 1);
    REQUIRE(reader.offset == 0);
    // Seven more symbols use up the rest of the byte.
    for (int i = 0; i < 7; ++i) Zlib::decodeSymbol(reader, table);
    REQUIRE(reader.used == 0);
    REQUIRE(reader.offset == 1);
}

TEST_CASE("Zlib gives up on an impossible stream") {
    // A table that only accepts a two bit code cannot read from an empty stream.
    const auto table = Zlib::buildTable({2, 2});
    Zlib::BitReader empty;
    const auto none = Zlib::decodeSymbol(empty, table);
    REQUIRE_FALSE(none.found);
    REQUIRE(none.symbol == -1);
    // A single three bit code has the code zero, so three one bits cannot name it.
    const auto lone = Zlib::buildTable({0, 3});
    const std::vector<std::uint8_t> ones{0x07};
    Zlib::BitReader reader{&ones, 0, 0};
    REQUIRE_FALSE(Zlib::decodeSymbol(reader, lone).found);
}
