#include "loaders/EXRLoader.h"

#include "catch.hpp"

#include <cstdint>
#include <vector>

TEST_CASE("EXRLoader reads a run count from the live bits") {
    // With nothing consumed the count is the low byte of the word.
    REQUIRE(EXRLoader::runLengthCount(0x05, 0) == 5);
    REQUIRE(EXRLoader::runLengthCount(0xFF, 0) == 255);
    // Four bits consumed drop the low nibble.
    REQUIRE(EXRLoader::runLengthCount(0x10, 4) == 1);
    REQUIRE(EXRLoader::runLengthCount(0xF0, 4) == 15);
}

TEST_CASE("EXRLoader cuts a run count to a byte") {
    // A count that would not fit a byte is taken modulo two hundred and fifty six, which is what
    // putting the shifted word through a one byte array does.
    REQUIRE(EXRLoader::runLengthCount(0x105, 0) == 5);
    REQUIRE(EXRLoader::runLengthCount(0x1FF, 0) == 255);
    REQUIRE(EXRLoader::runLengthCount(0x100, 0) == 0);
    // A negative count is not possible from a word, but a negative live count answers zero rather
    // than shifting the wrong way.
    REQUIRE(EXRLoader::runLengthCount(0x10, -1) == 0);
}

TEST_CASE("EXRLoader checks a run against the room left") {
    // A run that exactly fills the space fits.
    REQUIRE(EXRLoader::runFits(0, 10, 10));
    REQUIRE(EXRLoader::runFits(5, 5, 10));
    // One more does not.
    REQUIRE_FALSE(EXRLoader::runFits(0, 11, 10));
    REQUIRE_FALSE(EXRLoader::runFits(6, 5, 10));
    // A run of zero always fits, since it writes nothing.
    REQUIRE(EXRLoader::runFits(10, 0, 10));
    REQUIRE(EXRLoader::runFits(100, 0, 10));
}

TEST_CASE("EXRLoader writes a run of the last byte") {
    // The byte repeated is the one already written, not a new one.
    std::vector<std::uint8_t> out{1, 2};
    EXRLoader::writeRun(out, 3);
    REQUIRE(out.size() == 5);
    REQUIRE(out[2] == 2);
    REQUIRE(out[4] == 2);
    // A count of zero writes nothing at all.
    std::vector<std::uint8_t> same{7};
    EXRLoader::writeRun(same, 0);
    REQUIRE(same.size() == 1);
    // An empty output has nothing to repeat.
    std::vector<std::uint8_t> empty;
    EXRLoader::writeRun(empty, 3);
    REQUIRE(empty.empty());
}