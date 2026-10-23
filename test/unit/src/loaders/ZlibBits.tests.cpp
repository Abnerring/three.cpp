#include "loaders/Zlib.h"

#include "catch.hpp"

#include <cstdint>
#include <vector>

TEST_CASE("Zlib reads bits lowest first") {
    // Ten is 0000 1010, so its bits come out 0, 1, 0, 1, 0, 0, 0, 0.
    const std::vector<std::uint8_t> bytes{0x0A};
    Zlib::BitReader reader{&bytes, 0, 0};
    int taken[8] = {};
    for (int i = 0; i < 8; ++i) taken[i] = Zlib::readBit(reader);
    REQUIRE(taken[0] == 0);
    REQUIRE(taken[1] == 1);
    REQUIRE(taken[2] == 0);
    REQUIRE(taken[3] == 1);
    REQUIRE(taken[4] == 0);
    REQUIRE(taken[7] == 0);
    // All eight are gone, so the reader has moved on to the next byte.
    REQUIRE(reader.offset == 1);
    REQUIRE(reader.used == 0);
}

TEST_CASE("Zlib reads several bits at once") {
    // Four bits of all ones is fifteen.
    const std::vector<std::uint8_t> ones{0xFF};
    Zlib::BitReader a{&ones, 0, 0};
    REQUIRE(Zlib::readBits(a, 4) == 15);
    // The next four are still fifteen.
    REQUIRE(Zlib::readBits(a, 4) == 15);
    // Two bytes read low one first give the value the two spell when the low one is first.
    const std::vector<std::uint8_t> pair{0x34, 0x12};
    Zlib::BitReader b{&pair, 0, 0};
    REQUIRE(Zlib::readBits(b, 16) == 0x1234u);
    // A read of nothing takes nothing.
    Zlib::BitReader c{&pair, 0, 0};
    REQUIRE(Zlib::readBits(c, 0) == 0);
    REQUIRE(c.offset == 0);
}

TEST_CASE("Zlib lines up to a byte") {
    // Three bits taken leave five behind, which are dropped.
    const std::vector<std::uint8_t> bytes{0xFF, 0x0F};
    Zlib::BitReader reader{&bytes, 0, 0};
    Zlib::readBits(reader, 3);
    REQUIRE(reader.used == 3);
    Zlib::alignToByte(reader);
    REQUIRE(reader.used == 0);
    REQUIRE(reader.offset == 1);
    // A reader already lined up is left alone.
    Zlib::alignToByte(reader);
    REQUIRE(reader.offset == 1);
}

TEST_CASE("Zlib reads a block header") {
    // Three is 0000 0011: the last flag set and the kind bits 01, which is a fixed block.
    const std::vector<std::uint8_t> fixed{0x03};
    Zlib::BitReader a{&fixed, 0, 0};
    const auto first = Zlib::readBlockHeader(a);
    REQUIRE(first.last);
    REQUIRE(first.typeBits == 1);
    REQUIRE(first.type == Zlib::BlockType::Fixed);

    // Zero is a stored block and not the last.
    const std::vector<std::uint8_t> stored{0x00};
    Zlib::BitReader b{&stored, 0, 0};
    const auto second = Zlib::readBlockHeader(b);
    REQUIRE_FALSE(second.last);
    REQUIRE(second.type == Zlib::BlockType::Stored);

    // Four is 0000 0100: the kind bits are 10, which is dynamic.
    const std::vector<std::uint8_t> dynamic{0x04};
    Zlib::BitReader c{&dynamic, 0, 0};
    REQUIRE(Zlib::readBlockHeader(c).type == Zlib::BlockType::Dynamic);

    // Six is 0000 0110: the kind bits are 11, which is reserved.
    const std::vector<std::uint8_t> reserved{0x06};
    Zlib::BitReader d{&reserved, 0, 0};
    REQUIRE(Zlib::readBlockHeader(d).type == Zlib::BlockType::Reserved);
}

TEST_CASE("Zlib checks a stored block's two lengths") {
    // Ten and its inverse, written low byte first: ten is 0x000A and the inverse is 0xFFF5.
    const std::vector<std::uint8_t> good{0x0A, 0x00, 0xF5, 0xFF};
    Zlib::BitReader a{&good, 0, 0};
    const auto header = Zlib::readStoredHeader(a);
    REQUIRE(header.length == 10);
    REQUIRE(header.inverse == 0xFFF5u);
    REQUIRE(header.valid);
    // A second length that is not the inverse is refused.
    const std::vector<std::uint8_t> bad{0x0A, 0x00, 0xF4, 0xFF};
    Zlib::BitReader b{&bad, 0, 0};
    REQUIRE_FALSE(Zlib::readStoredHeader(b).valid);
}

TEST_CASE("Zlib knows when it has run out") {
    // An empty stream has nothing.
    Zlib::BitReader none;
    REQUIRE_FALSE(Zlib::hasMoreBits(none));
    // One byte has something until it is consumed.
    const std::vector<std::uint8_t> bytes{0xFF};
    Zlib::BitReader reader{&bytes, 0, 0};
    REQUIRE(Zlib::hasMoreBits(reader));
    Zlib::readBits(reader, 8);
    REQUIRE_FALSE(Zlib::hasMoreBits(reader));
}