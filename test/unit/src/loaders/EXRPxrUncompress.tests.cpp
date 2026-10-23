#include "loaders/EXRLoader.h"

#include "catch.hpp"

#include <cstdint>
#include <cstddef>
#include <vector>

namespace {
// A real zlib stream holding four bytes: two planes of two columns for a half precision channel.
// The first plane is both zeroes and the second holds 0x12 then 0x34.
std::vector<std::uint8_t> fourBytes() {
    return {0x78, 0x01, 0x63, 0x60, 0x10, 0x32, 0x01, 0x00, 0x00, 0x5C, 0x00, 0x47};
}
}  // namespace

TEST_CASE("EXRLoader unpacks a pixel rate block of planes") {
    // Two columns of a half precision channel: the first difference is 0x0012 and the second
    // 0x0034, summed to 0x0012 and then 0x0046.
    const auto result = EXRLoader::uncompressPXR(fourBytes(), {1}, 2, 1);
    INFO("error: " << result.error);
    REQUIRE(result.valid);
    REQUIRE(result.values.size() == 2);
    REQUIRE(result.values[0] == 0x12);
    REQUIRE(result.values[1] == 0x46);
}

TEST_CASE("EXRLoader refuses a pixel rate block that is not a stream") {
    // Bytes that are not a zlib stream are turned away rather than guessed at.
    const auto result = EXRLoader::uncompressPXR({1, 2, 3, 4}, {1}, 2, 1);
    REQUIRE_FALSE(result.valid);
    REQUIRE_FALSE(result.error.empty());
    // An empty block likewise.
    REQUIRE_FALSE(EXRLoader::uncompressPXR({}, {1}, 2, 1).valid);
}

TEST_CASE("EXRLoader refuses a channel type it cannot read") {
    // Only half and full precision have planes; anything else has none and is refused.
    const auto result = EXRLoader::uncompressPXR(fourBytes(), {0}, 2, 1);
    REQUIRE_FALSE(result.valid);
    REQUIRE_FALSE(result.error.empty());
}
