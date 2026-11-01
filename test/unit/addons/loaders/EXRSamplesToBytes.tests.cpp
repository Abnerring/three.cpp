#include "loaders/EXRLoader.h"

#include "catch.hpp"

#include <cstdint>
#include <vector>

TEST_CASE("EXRLoader writes samples out high byte first") {
    // Two samples become four bytes, each sample's higher byte first.
    const auto bytes = EXRLoader::samplesToBytes({0x1234, 0xABCD});
    REQUIRE(bytes.size() == 4);
    REQUIRE(bytes[0] == 0x12);
    REQUIRE(bytes[1] == 0x34);
    REQUIRE(bytes[2] == 0xAB);
    REQUIRE(bytes[3] == 0xCD);
    // An empty run gives nothing.
    REQUIRE(EXRLoader::samplesToBytes({}).empty());
    // One sample gives two bytes.
    const auto one = EXRLoader::samplesToBytes({0x0001});
    REQUIRE(one.size() == 2);
    REQUIRE(one[0] == 0x00);
    REQUIRE(one[1] == 0x01);
}

TEST_CASE("EXRLoader writes words out high byte first") {
    // One word becomes four bytes, highest first.
    const auto bytes = EXRLoader::wordsToBytes({0x12345678});
    REQUIRE(bytes.size() == 4);
    REQUIRE(bytes[0] == 0x12);
    REQUIRE(bytes[1] == 0x34);
    REQUIRE(bytes[2] == 0x56);
    REQUIRE(bytes[3] == 0x78);
    // A word whose bytes are all the same gives four of them.
    const auto flat = EXRLoader::wordsToBytes({0xAAAAAAAA});
    REQUIRE(flat.size() == 4);
    for (const auto byte : flat) REQUIRE(byte == 0xAA);
    REQUIRE(EXRLoader::wordsToBytes({}).empty());
}

TEST_CASE("EXRLoader keeps a word's high half where a sample would be") {
    // The first two bytes of a word are what a caller reading samples would see, so a word of
    // 0x12345678 reads as the sample 0x1234 there.
    const auto bytes = EXRLoader::wordsToBytes({0x12345678});
    const auto asSample = static_cast<std::uint16_t>((bytes[0] << 8) | bytes[1]);
    REQUIRE(asSample == 0x1234);
    // And the low half sits in the two bytes after it, which a sample reader never touches.
    const auto lowHalf = static_cast<std::uint16_t>((bytes[2] << 8) | bytes[3]);
    REQUIRE(lowHalf == 0x5678);
}