#include "loaders/EXRLoader.h"

#include "catch.hpp"

#include <cstdint>
#include <vector>

TEST_CASE("EXRLoader copies a verbatim run") {
    // A header of minus two is followed by two bytes taken as they are.
    const auto out = EXRLoader::decodeRunLength({0xFE, 0x0A, 0x0B});
    REQUIRE(out.size() == 2);
    REQUIRE(out[0] == 0x0A);
    REQUIRE(out[1] == 0x0B);
    // A header of minus one copies a single byte.
    const auto one = EXRLoader::decodeRunLength({0xFF, 0x2A});
    REQUIRE(one.size() == 1);
    REQUIRE(one[0] == 0x2A);
}

TEST_CASE("EXRLoader repeats a run") {
    // A header of two writes one more than that, so three copies of the byte.
    const auto three = EXRLoader::decodeRunLength({0x02, 0x07});
    REQUIRE(three.size() == 3);
    REQUIRE(three[0] == 0x07);
    REQUIRE(three[2] == 0x07);
    // A header of zero still writes once rather than nothing.
    const auto once = EXRLoader::decodeRunLength({0x00, 0x09});
    REQUIRE(once.size() == 1);
    REQUIRE(once[0] == 0x09);
}

TEST_CASE("EXRLoader reads several runs in one stream") {
    // One repeat of one byte, then a verbatim run of two.
    const auto out = EXRLoader::decodeRunLength({0x00, 0x01, 0xFE, 0x02, 0x03});
    REQUIRE(out.size() == 3);
    REQUIRE(out[0] == 0x01);
    REQUIRE(out[1] == 0x02);
    REQUIRE(out[2] == 0x03);
}

TEST_CASE("EXRLoader stops when the stream runs out") {
    // An empty stream writes nothing.
    REQUIRE(EXRLoader::decodeRunLength({}).empty());
    // A verbatim header that promises more than the buffer holds copies only what is there.
    const auto short_ = EXRLoader::decodeRunLength({0xFD, 0x01});
    REQUIRE(short_.size() == 1);
    REQUIRE(short_[0] == 0x01);
    // A repeat header with no value byte writes nothing.
    REQUIRE(EXRLoader::decodeRunLength({0x02}).empty());
}