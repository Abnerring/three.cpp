#include "loaders/EXRLoader.h"

#include "catch.hpp"

#include <cstdint>
#include <cstddef>
#include <vector>

TEST_CASE("EXRLoader unpacks three plain code lengths") {
    // Three symbols of six bits each, most significant first: zero, one and two make the bit
    // string 000000 000001 000010, which pads out to the three bytes below.
    const std::vector<std::uint8_t> data{0x00, 0x10, 0x80};
    std::size_t offset = 0;
    int liveBits = 0;
    std::uint32_t accumulator = 0;
    const auto result = EXRLoader::hufUnpackEncTable(data, offset, liveBits, accumulator, 3, 0, 2);
    INFO("error: " << result.error);
    REQUIRE(result.valid);
    REQUIRE(result.lengths.size() == static_cast<std::size_t>(EXRLoader::kHufEncSize));
    // The three lengths are what was written.
    REQUIRE(result.lengths[0] == 0);
    REQUIRE(result.lengths[1] == 1);
    REQUIRE(result.lengths[2] == 2);
    // Everything outside the range stays zero.
    REQUIRE(result.lengths[3] == 0);
    REQUIRE(result.lengths[100] == 0);
}

TEST_CASE("EXRLoader refuses a table that runs out") {
    // The same three lengths but only one byte, so the second symbol cannot be read.
    const std::vector<std::uint8_t> data{0x00};
    std::size_t offset = 0;
    int liveBits = 0;
    std::uint32_t accumulator = 0;
    const auto result = EXRLoader::hufUnpackEncTable(data, offset, liveBits, accumulator, 1, 0, 2);
    REQUIRE_FALSE(result.valid);
    // An empty stream likewise.
    std::size_t offset2 = 0;
    int liveBits2 = 0;
    std::uint32_t accumulator2 = 0;
    REQUIRE_FALSE(
        EXRLoader::hufUnpackEncTable({}, offset2, liveBits2, accumulator2, 1, 0, 0).valid);
}

TEST_CASE("EXRLoader names the table error") {
    REQUIRE(EXRLoader::hufTableMessage() ==
            "THREE.EXRLoader: Something wrong with hufUnpackEncTable");
}