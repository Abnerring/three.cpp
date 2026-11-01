#include "loaders/EXRLoader.h"

#include "catch.hpp"

#include <cstdint>
#include <cstddef>
#include <vector>

namespace {
// A table with a single one bit code of zero naming symbol five, so every zero bit read gives
// that symbol and nothing else can be read at all.
EXRLoader::DecTableResult singleCodeTable(std::vector<int>& codes) {
    codes.assign(EXRLoader::kHufEncSize, 0);
    codes[5] = (0 << 6) | 1;
    return EXRLoader::hufBuildDecTable(codes, 5, 5);
}
}  // namespace

TEST_CASE("EXRLoader decodes a block of zero bits into one symbol") {
    // Eight zero bits, each a whole code, give eight of the symbol.
    std::vector<int> codes;
    const auto built = singleCodeTable(codes);
    REQUIRE(built.valid);
    std::vector<std::int32_t> out(8, 0);
    const auto result = EXRLoader::hufDecode(codes, built.entries, {0x00}, 0, 0, 0, 8, -1, out, 8);
    INFO("error: " << result.error);
    REQUIRE(result.valid);
    REQUIRE(result.written == 8);
    for (const auto value : out) REQUIRE(value == 5);
}

TEST_CASE("EXRLoader stops when the room is full") {
    // The same eight bits but only three places to write, so the fourth symbol has nowhere to
    // go and the loop says so rather than writing past the end.
    std::vector<int> codes;
    const auto built = singleCodeTable(codes);
    REQUIRE(built.valid);
    std::vector<std::int32_t> out(3, 0);
    const auto result = EXRLoader::hufDecode(codes, built.entries, {0x00}, 0, 0, 0, 8, -1, out, 3);
    REQUIRE_FALSE(result.valid);
    REQUIRE(result.error == EXRLoader::hufDecodeMessage());
}

TEST_CASE("EXRLoader gathers the bits of several bytes") {
    // Sixteen zero bits give sixteen symbols, which needs two bytes gathered rather than one.
    std::vector<int> codes;
    const auto built = singleCodeTable(codes);
    std::vector<std::int32_t> out(16, 0);
    const auto result =
        EXRLoader::hufDecode(codes, built.entries, {0x00, 0x00}, 0, 0, 0, 16, -1, out, 16);
    INFO("error: " << result.error);
    REQUIRE(result.valid);
    REQUIRE(result.written == 16);
    for (const auto value : out) REQUIRE(value == 5);
}

TEST_CASE("EXRLoader takes the run marker as a repeat") {
    // A code naming the marker itself, with the marker given as that same value, makes every
    // read a request to repeat rather than a value of its own.
    std::vector<int> codes(EXRLoader::kHufEncSize, 0);
    codes[5] = (0 << 6) | 1;
    const auto built = EXRLoader::hufBuildDecTable(codes, 5, 5);
    REQUIRE(built.valid);
    std::vector<std::int32_t> out(8, 0);
    // A run of nothing fits anywhere, so a block of zero bits writes nothing at all.
    const auto result = EXRLoader::hufDecode(codes, built.entries, {0x00}, 0, 0, 0, 8, 5, out, 8);
    REQUIRE(result.valid);
    REQUIRE(result.written == 0);
}