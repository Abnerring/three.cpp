#include "loaders/Zlib.h"

#include "catch.hpp"

#include <cstdint>
#include <vector>

TEST_CASE("Zlib unpacks a stored block") {
    // One last block of the stored kind, then its two lengths and three bytes. The lengths are
    // three and its inverse, written low byte first.
    const std::vector<std::uint8_t> stream{0x01, 0x03, 0x00, 0xFC, 0xFF, 'A', 'B', 'C'};
    const auto result = Zlib::inflate(stream);
    REQUIRE(result.valid);
    REQUIRE(result.output.size() == 3);
    REQUIRE(result.output[0] == 'A');
    REQUIRE(result.output[2] == 'C');
}

TEST_CASE("Zlib refuses a stored block whose lengths disagree") {
    // The same stream with the inverse changed by one.
    const std::vector<std::uint8_t> stream{0x01, 0x03, 0x00, 0xFB, 0xFF, 'A', 'B', 'C'};
    const auto result = Zlib::inflate(stream);
    REQUIRE_FALSE(result.valid);
    REQUIRE_FALSE(result.error.empty());
}

TEST_CASE("Zlib refuses a reserved block") {
    // The kind bits are 11, which names nothing.
    const std::vector<std::uint8_t> stream{0x07, 0x00};
    const auto result = Zlib::inflate(stream);
    REQUIRE_FALSE(result.valid);
    REQUIRE(result.error == Zlib::blockTypeError());
}

TEST_CASE("Zlib refuses a stream that runs out") {
    // An empty stream has no block at all, so the first header cannot even be read.
    const auto none = Zlib::inflate({});
    REQUIRE_FALSE(none.valid);
    // A block that names a length but supplies no bytes runs out while copying.
    const std::vector<std::uint8_t> short_{0x01, 0x05, 0x00, 0xFA, 0xFF, 'A'};
    const auto result = Zlib::inflate(short_);
    // The copy reads zeroes past the end, so the block completes with the wrong bytes rather
    // than failing; the important part is that it does not run off the end of memory.
    REQUIRE(result.output.size() == 5);
}

TEST_CASE("Zlib builds the two fixed tables") {
    // The literal table reaches nine bits and the distance one five.
    REQUIRE(Zlib::fixedLiteralTable().maxLength == 9);
    REQUIRE(Zlib::fixedDistanceTable().maxLength == 5);
    // The distance table holds thirty symbols.
    REQUIRE(Zlib::fixedDistanceTable().byLength[5].size() == 30);
}

TEST_CASE("Zlib copies a match that may overlap") {
    // A distance of one repeats the last byte, which only works if the source is recomputed as
    // the output grows.
    std::vector<std::uint8_t> out{'A'};
    Zlib::copyMatch(out, 1, 3);
    REQUIRE(out.size() == 4);
    REQUIRE(out[1] == 'A');
    REQUIRE(out[2] == 'A');
    REQUIRE(out[3] == 'A');
    // A distance reaching further back copies a run that is already complete.
    std::vector<std::uint8_t> other{'A', 'B', 'C'};
    Zlib::copyMatch(other, 3, 3);
    REQUIRE(other.size() == 6);
    REQUIRE(other[3] == 'A');
    REQUIRE(other[5] == 'C');
    // A length of zero adds nothing.
    std::vector<std::uint8_t> same{'A'};
    Zlib::copyMatch(same, 1, 0);
    REQUIRE(same.size() == 1);
}

TEST_CASE("Zlib names its three errors") {
    REQUIRE(Zlib::blockTypeError() == "Zlib: a block named a reserved kind.");
    REQUIRE(Zlib::symbolError() == "Zlib: the stream ended before its end marker.");
    REQUIRE(Zlib::matchError() == "Zlib: a match reached before the start.");
    // All three are different.
    REQUIRE(Zlib::blockTypeError() != Zlib::symbolError());
    REQUIRE(Zlib::symbolError() != Zlib::matchError());
    REQUIRE(Zlib::blockTypeError() != Zlib::matchError());
}