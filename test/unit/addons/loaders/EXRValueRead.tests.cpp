#include "loaders/EXRLoader.h"

#include "catch.hpp"

#include <cstdint>
#include <string>
#include <vector>

TEST_CASE("EXRLoader reads a rational with two signs") {
    // The count is signed and the divisor unsigned, so the same bit pattern means different
    // things in the two halves.
    const auto positive = EXRLoader::rationalFrom(5, 7);
    REQUIRE(positive.numerator == 5);
    REQUIRE(positive.denominator == 7);
    // A negative count is possible.
    const auto negative = EXRLoader::rationalFrom(-5, 7);
    REQUIRE(negative.numerator == -5);
    REQUIRE(negative.denominator == 7);
    // A divisor uses its whole range, so the top bit is not a sign here.
    const auto wide = EXRLoader::rationalFrom(1, 0xFFFFFFFFu);
    REQUIRE(wide.denominator == 0xFFFFFFFFu);
}

TEST_CASE("EXRLoader reads a fixed length string without stopping at a zero") {
    // The bytes are taken as stated, so an embedded zero stays in the middle.
    const std::vector<std::uint8_t> bytes{'a', 0, 'b', 'c'};
    const std::string text = EXRLoader::fixedLengthString(bytes, 0, 4);
    REQUIRE(text.size() == 4);
    REQUIRE(text[0] == 'a');
    REQUIRE(text[1] == '\0');
    REQUIRE(text[3] == 'c');
    // This is what separates it from the names in the header, which do stop.
    REQUIRE(text.size() != std::string("a").size());
}

TEST_CASE("EXRLoader reads a fixed length string from an offset") {
    const std::vector<std::uint8_t> bytes{'x', 'a', 'b', 'c', 'y'};
    // Three bytes from the second one.
    REQUIRE(EXRLoader::fixedLengthString(bytes, 1, 3) == "abc");
    // A length that reaches past the end takes only what is there.
    REQUIRE(EXRLoader::fixedLengthString(bytes, 3, 10).size() == 2);
    // A length of zero takes nothing.
    REQUIRE(EXRLoader::fixedLengthString(bytes, 0, 0).empty());
    // An offset past the end takes nothing either.
    REQUIRE(EXRLoader::fixedLengthString(bytes, 99, 3).empty());
}

TEST_CASE("EXRLoader advances by four for both kinds") {
    // A float and an int are both four bytes.
    REQUIRE(EXRLoader::kFloatSize == 4);
    REQUIRE(EXRLoader::kIntSize == 4);
    REQUIRE(EXRLoader::advanceF32(0) == 4);
    REQUIRE(EXRLoader::advanceF32(4) == 8);
    REQUIRE(EXRLoader::advanceU32(8) == 12);
    // A vector of two floats therefore advances by eight.
    REQUIRE(EXRLoader::advanceF32(EXRLoader::advanceF32(0)) == 8);
}