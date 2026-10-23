#include "loaders/Zlib.h"

#include "catch.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace {
// A whole zlib stream holding the three lowercase letters: the header, one stored block and the
// checksum of the output written most significant byte first. The lowercase three have the well
// known checksum, which the uppercase ones do not.
std::vector<std::uint8_t> abcStream() {
    return {0x78, 0x01, 0x01, 0x03, 0x00, 0xFC, 0xFF, 'a', 'b', 'c', 0x02, 0x4D, 0x01, 0x27};
}
}  // namespace

TEST_CASE("Zlib reads a checksum from the far end") {
    // The four bytes of the checksum of the three letters, most significant first.
    REQUIRE(Zlib::adlerFromBigEndian(0x02, 0x4D, 0x01, 0x27) == 0x024D0127u);
    // All zero is zero whatever the order.
    REQUIRE(Zlib::adlerFromBigEndian(0, 0, 0, 0) == 0);
    // The first byte is the top one, which is what makes this different from every other number.
    REQUIRE(Zlib::adlerFromBigEndian(1, 0, 0, 0) == 0x01000000u);
    REQUIRE(Zlib::adlerFromBigEndian(0, 0, 0, 1) == 1u);
    // The same four bytes the other way round give a different answer.
    REQUIRE(Zlib::adlerFromBigEndian(0x02, 0x4D, 0x01, 0x27) !=
            Zlib::adlerFromBigEndian(0x27, 0x01, 0x4D, 0x02));
}

TEST_CASE("Zlib unpacks a whole stream") {
    // The three letters come back out of the stored block.
    const auto result = Zlib::unzlib(abcStream());
    REQUIRE(result.valid);
    REQUIRE(result.output.size() == 3);
    REQUIRE(result.output[0] == 'a');
    REQUIRE(result.output[1] == 'b');
    REQUIRE(result.output[2] == 'c');
}

TEST_CASE("Zlib refuses a stream whose checksum is wrong") {
    // The same stream with the last checksum byte changed.
    auto stream = abcStream();
    stream.back() = 0x28;
    const auto result = Zlib::unzlib(stream);
    REQUIRE_FALSE(result.valid);
    REQUIRE(result.error == Zlib::checksumError());
}

TEST_CASE("Zlib refuses a stream that is too short") {
    // Fewer bytes than a header and a checksum together.
    const auto result = Zlib::unzlib({0x78, 0x01, 0x00});
    REQUIRE_FALSE(result.valid);
    REQUIRE(result.error == Zlib::shortError());
    // An empty stream likewise.
    REQUIRE(Zlib::unzlib({}).error == Zlib::shortError());
}

TEST_CASE("Zlib refuses a stream that needs a dictionary") {
    // The second byte with its sixth bit set, still a valid pair.
    auto stream = abcStream();
    stream[1] = 0xBB;
    const auto result = Zlib::unzlib(stream);
    REQUIRE_FALSE(result.valid);
    REQUIRE(result.error == Zlib::dictionaryError());
}

TEST_CASE("Zlib passes a bad header through") {
    // A pair that fails the check reports the check's own error rather than one of its own.
    auto stream = abcStream();
    stream[1] = 0x00;
    const auto result = Zlib::unzlib(stream);
    REQUIRE_FALSE(result.valid);
    REQUIRE(result.error == Zlib::checkError());
}

TEST_CASE("Zlib names its three wrapper errors") {
    REQUIRE(Zlib::shortError() == "Zlib: the stream is too short.");
    REQUIRE(Zlib::checksumError() == "Zlib: the checksum does not match.");
    REQUIRE(Zlib::dictionaryError() == "Zlib: a dictionary is required.");
    REQUIRE(Zlib::shortError() != Zlib::checksumError());
    REQUIRE(Zlib::checksumError() != Zlib::dictionaryError());
}
