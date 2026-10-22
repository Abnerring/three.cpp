#include "loaders/KSPLATLoader.h"

#include "catch.hpp"

#include <cstdint>
#include <vector>

TEST_CASE("KSPLATLoader writes a harmonic band in index order") {
    // Nine half float components, the third slot holding minus a half and the fourth a half.
    std::vector<std::uint8_t> source(9 * 2, 0);
    // Index three is minus a half: 0xb800 as a half float.
    source[3 * 2 + 0] = 0x00;
    source[3 * 2 + 1] = 0xb8;
    // Index zero is a half: 0x3800.
    source[0] = 0x00;
    source[1] = 0x38;

    const std::vector<int> indexes{0, 3, 6, 1, 4, 7, 2, 5, 8};
    std::vector<std::uint8_t> target(12, 0);
    KSPLATLoader::writeHarmonicBand(target, 0, 9, 12, indexes, source, 0, 2, -1.5, 1.5);

    // Slot zero reads source index zero, which is a half, so 0.5 * 128 + 128 = 192.
    REQUIRE(target[0] == 192);
    // Slot one reads source index three, which is minus a half, so 64.
    REQUIRE(target[1] == 64);
    // The remaining slots read zero, which lands on the midpoint.
    REQUIRE(target[2] == 128);
}

TEST_CASE("KSPLATLoader writes at the splat offset") {
    std::vector<std::uint8_t> source(9 * 2, 0);
    source[0] = 0x00;
    source[1] = 0x38;
    const std::vector<int> indexes{0, 3, 6, 1, 4, 7, 2, 5, 8};
    // Three splats of twelve bytes each.
    std::vector<std::uint8_t> target(36, 0);
    KSPLATLoader::writeHarmonicBand(target, 2, 9, 12, indexes, source, 0, 2, -1.5, 1.5);
    // The third splat starts at twenty four.
    REQUIRE(target[24] == 192);
    REQUIRE(target[0] == 0);
}

TEST_CASE("KSPLATLoader caps a component at the top of the range") {
    std::vector<std::uint8_t> source(9 * 2, 0);
    // A half float of one lands on two hundred and fifty six, which a byte truncates to zero.
    source[0] = 0x00;
    source[1] = 0x3c;
    const std::vector<int> indexes{0, 3, 6, 1, 4, 7, 2, 5, 8};
    std::vector<std::uint8_t> target(12, 0);
    KSPLATLoader::writeHarmonicBand(target, 0, 9, 12, indexes, source, 0, 2, -1.5, 1.5);
    REQUIRE(target[0] == 0);
}

TEST_CASE("KSPLATLoader leaves a band that does not fit alone") {
    std::vector<std::uint8_t> source(18, 0);
    const std::vector<int> indexes{0, 3, 6, 1, 4, 7, 2, 5, 8};
    // A target with only four bytes cannot hold a nine component band.
    std::vector<std::uint8_t> target(4, 0);
    KSPLATLoader::writeHarmonicBand(target, 0, 9, 12, indexes, source, 0, 2, -1.5, 1.5);
    for (const auto value : target) REQUIRE(value == 0);
}