#include "loaders/EXRLoader.h"

#include "catch.hpp"

#include <cstdint>
#include <cstddef>
#include <vector>

namespace {
// A header of three numbers, each four bytes high first and each padded with four more: the
// first symbol, the last symbol and the bit count.
// The header is three numbers of four bytes each, high first, with four bytes of padding after
// the second so that the third begins at twelve, which is where the reader looks for it.
std::vector<std::uint8_t> header(std::uint32_t im, std::uint32_t iM, std::uint32_t nBits) {
    std::vector<std::uint8_t> bytes;
    const auto put = [&](std::uint32_t value) {
        for (int i = 3; i >= 0; --i) bytes.push_back(static_cast<std::uint8_t>((value >> (8 * i)) & 0xFFu));
    };
    put(im);
    put(iM);
    for (int i = 0; i < 4; ++i) bytes.push_back(0);
    put(nBits);
    return bytes;
}
}  // namespace

TEST_CASE("EXRLoader refuses a symbol range outside the alphabet") {
    // The far end must lie inside the alphabet, which has sixty five thousand five hundred and
    // thirty six entries, so a number beyond that is refused before anything is read.
    const auto beyond = EXRLoader::hufUncompress(header(0, 70000, 0), 0, 16, 4);
    REQUIRE_FALSE(beyond.valid);
    REQUIRE(beyond.error == EXRLoader::hufRangeMessage());
    // The same for the near end.
    const auto below = EXRLoader::hufUncompress(header(70000, 0, 0), 0, 16, 4);
    REQUIRE_FALSE(below.valid);
    REQUIRE(below.error == EXRLoader::hufRangeMessage());
    // A range that fits gets past this check, whatever happens later.
    const auto fine = EXRLoader::hufUncompress(header(0, 0, 0), 0, 16, 4);
    REQUIRE(fine.error != EXRLoader::hufRangeMessage());
}

TEST_CASE("EXRLoader refuses more bits than the block holds") {
    // The bit count is checked against the bytes left after the header, so a huge one cannot fit
    // in a block that ends at the header.
    const auto tooMany = EXRLoader::hufUncompress(header(0, 0, 1000), 0, 16, 4);
    REQUIRE_FALSE(tooMany.valid);
    REQUIRE(tooMany.error == EXRLoader::hufBitsMessage());
    // Nought bits fits an empty remainder, so this check lets it through.
    const auto none = EXRLoader::hufUncompress(header(0, 0, 0), 0, 16, 4);
    REQUIRE(none.error != EXRLoader::hufBitsMessage());
}

TEST_CASE("EXRLoader refuses a block shorter than its header") {
    // An empty block cannot even supply the three numbers.
    const auto empty = EXRLoader::hufUncompress({}, 0, 0, 4);
    REQUIRE_FALSE(empty.valid);
    REQUIRE_FALSE(empty.error.empty());
}