#include "loaders/EXRLoader.h"

#include "catch.hpp"

#include <cstdint>
#include <cstddef>
#include <vector>

namespace {
const char* kBitmapMessage = "THREE.EXRLoader: Something is wrong with PIZ_COMPRESSION BITMAP_SIZE";

// The first four bytes of a wavelet block: the two ends of the bitmap range, each two bytes
// high first.
std::vector<std::uint8_t> range(std::uint16_t minNonZero, std::uint16_t maxNonZero) {
    return {static_cast<std::uint8_t>(minNonZero >> 8),
            static_cast<std::uint8_t>(minNonZero & 0xFF),
            static_cast<std::uint8_t>(maxNonZero >> 8),
            static_cast<std::uint8_t>(maxNonZero & 0xFF)};
}
}  // namespace

TEST_CASE("EXRLoader refuses a bitmap range past the bitmap") {
    // The second end must lie inside the eight thousand one hundred and ninety two bytes the
    // bitmap takes, so one at that value is refused before anything else is read.
    const auto beyond = EXRLoader::uncompressPIZ(range(0, 8192), 0, 4, 1, 1, 1, 1);
    REQUIRE_FALSE(beyond.valid);
    REQUIRE(beyond.error == kBitmapMessage);
}

TEST_CASE("EXRLoader refuses a block that ends inside its bitmap") {
    // The last value that fits gets past the range check but the block holds none of the bytes
    // the bitmap needs, so it is refused further on.
    const auto fits = EXRLoader::uncompressPIZ(range(0, 8191), 0, 4, 1, 1, 1, 1);
    REQUIRE_FALSE(fits.valid);
    REQUIRE_FALSE(fits.error.empty());
    // A range of four positions with an empty block likewise.
    const auto short_ = EXRLoader::uncompressPIZ(range(0, 3), 0, 4, 1, 1, 1, 1);
    REQUIRE_FALSE(short_.valid);
    REQUIRE_FALSE(short_.error.empty());
}

TEST_CASE("EXRLoader accepts an empty range without reading a bitmap") {
    // A first end past the second writes no bitmap at all, so the read goes straight on to the
    // encoded table's length, which is missing and leaves the block refused a step later.
    const auto none = EXRLoader::uncompressPIZ(range(5, 4), 0, 4, 1, 1, 1, 1);
    REQUIRE_FALSE(none.valid);
    REQUIRE_FALSE(none.error.empty());
}