#include "loaders/Zlib.h"

#include "catch.hpp"

#include <cstdint>
#include <vector>

TEST_CASE("Zlib splits the fixed literal alphabet into four runs") {
    // The first hundred and forty four use eight bits.
    REQUIRE(Zlib::fixedLiteralLength(0) == 8);
    REQUIRE(Zlib::fixedLiteralLength(143) == 8);
    // The next hundred and twelve use nine.
    REQUIRE(Zlib::fixedLiteralLength(144) == 9);
    REQUIRE(Zlib::fixedLiteralLength(255) == 9);
    // The next twenty four use seven.
    REQUIRE(Zlib::fixedLiteralLength(256) == 7);
    REQUIRE(Zlib::fixedLiteralLength(279) == 7);
    // The last eight use eight again.
    REQUIRE(Zlib::fixedLiteralLength(280) == 8);
    REQUIRE(Zlib::fixedLiteralLength(287) == 8);
    // Outside the alphabet there is no length.
    REQUIRE(Zlib::fixedLiteralLength(288) == 0);
    REQUIRE(Zlib::fixedLiteralLength(-1) == 0);
}

TEST_CASE("Zlib turns a code round") {
    // One in three bits becomes the top bit.
    REQUIRE(Zlib::reverseBits(0b001u, 3) == 0b100u);
    REQUIRE(Zlib::reverseBits(0b100u, 3) == 0b001u);
    // Zero stays zero however wide.
    REQUIRE(Zlib::reverseBits(0u, 8) == 0u);
    // All ones stay all ones.
    REQUIRE(Zlib::reverseBits(0xFFu, 8) == 0xFFu);
    // The first eight bit code is thirty, which turns round into twelve.
    REQUIRE(Zlib::reverseBits(0x30u, 8) == 0x0Cu);
    // A width of zero takes nothing.
    REQUIRE(Zlib::reverseBits(0xFFu, 0) == 0u);
}

TEST_CASE("Zlib numbers the fixed distance codes") {
    // Thirty of them, all five bits.
    const auto codes = Zlib::fixedDistanceCodes();
    REQUIRE(codes.size() == 30);
    REQUIRE(Zlib::kFixedDistanceLength == 5);
    // The first is zero and the second is one turned round into five bits.
    REQUIRE(codes[0] == 0);
    REQUIRE(codes[1] == Zlib::reverseBits(1u, 5));
    REQUIRE(codes[1] == 16);
    // Each is the previous with the counting turned round, so none repeats.
    for (std::size_t i = 1; i < codes.size(); ++i) {
        REQUIRE(codes[i] != codes[i - 1]);
    }
}

TEST_CASE("Zlib assigns the fixed literal codes canonically") {
    const auto codes = Zlib::fixedLiteralCodes();
    REQUIRE(codes.size() == 288);
    // The first eight bit code is thirty, which the stream carries reversed.
    REQUIRE(codes[0] == 0x0C);
    REQUIRE(codes[143] == Zlib::reverseBits(0xBFu, 8));
    // The nine bit run begins at the next value the eight bit run leaves off at.
    REQUIRE(codes[144] == Zlib::reverseBits(0x190u, 9));
    // Every code fits inside its own length.
    for (int symbol = 0; symbol < 288; ++symbol) {
        const int length = Zlib::fixedLiteralLength(symbol);
        const std::uint32_t top = (length >= 32) ? 0xFFFFFFFFu : ((1u << length) - 1u);
        REQUIRE(codes[static_cast<std::size_t>(symbol)] <= top);
    }
}