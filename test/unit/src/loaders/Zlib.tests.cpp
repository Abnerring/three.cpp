#include "loaders/Zlib.h"

#include "catch.hpp"

#include <cstdint>
#include <string>
#include <vector>

TEST_CASE("Zlib reads a header") {
    // The commonest header: deflate, a window of thirty two thousand.
    const auto common = Zlib::readHeader(0x78, 0x9C);
    REQUIRE(common.valid);
    REQUIRE(common.header.method == 8);
    REQUIRE(common.header.windowBits == 15);
    REQUIRE(Zlib::windowSize(common.header.windowBits) == 32768);
    // A smaller window is named by a smaller exponent.
    const auto small = Zlib::readHeader(0x08, 0x1D);
    REQUIRE(small.valid);
    REQUIRE(small.header.windowBits == 8);
    REQUIRE(Zlib::windowSize(small.header.windowBits) == 256);
}

TEST_CASE("Zlib refuses a header whose check fails") {
    // The two bytes together must be a multiple of thirty one.
    const auto bad = Zlib::readHeader(0x78, 0x00);
    REQUIRE_FALSE(bad.valid);
    REQUIRE(bad.error == Zlib::checkError());
    // A header that does pass is accepted.
    const auto good = Zlib::readHeader(0x78, 0x01);
    REQUIRE(good.valid);
}

TEST_CASE("Zlib refuses a method that is not deflate") {
    // The low nibble must be eight. A nine fails the check first unless the second byte is
    // chosen to make the pair a multiple of thirty one.
    const auto other = Zlib::readHeader(0x79, 0x18);
    REQUIRE_FALSE(other.valid);
    REQUIRE(other.error == Zlib::methodError());
    // The checksum is done before the method, so a bad pair reports the check.
    const auto both = Zlib::readHeader(0x79, 0x00);
    REQUIRE_FALSE(both.valid);
    REQUIRE(both.error == Zlib::checkError());
}

TEST_CASE("Zlib reads the two flags of the second byte") {
    // The sixth bit says a dictionary is needed.
    REQUIRE_FALSE(Zlib::readHeader(0x78, 0x9C).header.needsDictionary);
    const auto dictionary = Zlib::readHeader(0x78, 0xBB);
    REQUIRE(dictionary.valid);
    REQUIRE(dictionary.header.needsDictionary);
    // The top two bits are the level hint, from zero to three.
    REQUIRE(Zlib::readHeader(0x78, 0x9C).header.levelHint == 2);
    REQUIRE(Zlib::readHeader(0x78, 0xBB).header.levelHint == 2);
}

TEST_CASE("Zlib checksums with two running sums") {
    // An empty stream checksums to one, the value the first sum starts at.
    REQUIRE(Zlib::adler32({}) == 1);
    // The three letters of the alphabet are a known answer.
    REQUIRE(Zlib::adler32({'a', 'b', 'c'}) == 0x024D0127u);
    // A single zero byte leaves the first sum at one but adds it to the second.
    REQUIRE(Zlib::adler32({0}) == 0x00010001u);
    // The first sum is two after adding the byte, and the second adds that two to itself.
    const std::uint32_t single = (2u << 16) | 2u;
    REQUIRE(Zlib::adler32({1}) == single);
    // Updating is the same as starting over only when the running value is one.
    REQUIRE(Zlib::adler32Update(1, {'a', 'b', 'c'}) == Zlib::adler32({'a', 'b', 'c'}));
    REQUIRE(Zlib::adler32Update(Zlib::adler32({'a'}), {'b', 'c'}) == Zlib::adler32({'a', 'b', 'c'}));
}

TEST_CASE("Zlib sizes a window from its exponent") {
    // The exponent is at least eight.
    REQUIRE(Zlib::windowSize(8) == 256);
    REQUIRE(Zlib::windowSize(9) == 512);
    REQUIRE(Zlib::windowSize(15) == 32768);
    // Below eight there is no window at all.
    REQUIRE(Zlib::windowSize(0) == 0);
    REQUIRE(Zlib::windowSize(7) == 0);
}
