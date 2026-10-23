#include "loaders/EXRLoader.h"

#include "catch.hpp"

#include <cstdint>
#include <vector>

TEST_CASE("EXRLoader expands a run of literals") {
    // A negative count introduces that many bytes as they are: minus one gives one byte.
    const auto one = EXRLoader::decodeRunLength({0xFF, 0x01, 0x02});
    REQUIRE(one.size() == 1);
    REQUIRE(one[0] == 1);
    // Minus two gives two.
    const auto two = EXRLoader::decodeRunLength({0xFE, 2, 3, 0xFF, 4});
    REQUIRE(two.size() == 3);
    REQUIRE(two[0] == 2);
    REQUIRE(two[2] == 4);
}

TEST_CASE("EXRLoader repeats a byte a count plus one times") {
    // A count of zero gives one copy, not none, which is the asymmetry worth pinning.
    const auto once = EXRLoader::decodeRunLength({0x00, 0x07});
    REQUIRE(once.size() == 1);
    REQUIRE(once[0] == 7);
    // A count of one gives two copies.
    const auto twice = EXRLoader::decodeRunLength({0x01, 0x07});
    REQUIRE(twice.size() == 2);
    REQUIRE(twice[0] == 7);
    REQUIRE(twice[1] == 7);
    // A count of three gives four.
    const auto four = EXRLoader::decodeRunLength({0x03, 0x09});
    REQUIRE(four.size() == 4);
    REQUIRE(four[3] == 9);
}

TEST_CASE("EXRLoader unpacks an RLE compressed block") {
    // Three literals one two three, then the predictor and the interleave, exactly as the zipped
    // path does. The predictor gives one, one hundred and thirty one and six, and the halves are
    // the first two and the last one, so the answer is one, six, one hundred and thirty one.
    const auto result = EXRLoader::uncompressRLE({0xFF, 0x01, 0xFE, 0x02, 0x03});
    REQUIRE(result.valid);
    REQUIRE(result.bytes.size() == 3);
    REQUIRE(result.bytes[0] == 1);
    REQUIRE(result.bytes[1] == 6);
    REQUIRE(result.bytes[2] == 131);
}

TEST_CASE("EXRLoader runs out of an RLE block gracefully") {
    // An empty block expands to nothing and the rest is identity, rather than reading past the
    // end.
    const auto none = EXRLoader::uncompressRLE({});
    REQUIRE(none.valid);
    REQUIRE(none.bytes.empty());
    // A single literal byte likewise.
    const auto one = EXRLoader::uncompressRLE({0xFF, 0x05});
    REQUIRE(one.valid);
    REQUIRE(one.bytes.size() == 1);
    REQUIRE(one.bytes[0] == 5);
}
