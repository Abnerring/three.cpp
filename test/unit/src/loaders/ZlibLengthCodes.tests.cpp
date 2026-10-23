#include "loaders/Zlib.h"

#include "catch.hpp"

TEST_CASE("Zlib names a base length for each length code") {
    // The first eight run from three to ten without gaps.
    REQUIRE(Zlib::lengthBase(257) == 3);
    REQUIRE(Zlib::lengthBase(258) == 4);
    REQUIRE(Zlib::lengthBase(264) == 10);
    // Then they step by two, then four, then eight.
    REQUIRE(Zlib::lengthBase(265) == 11);
    REQUIRE(Zlib::lengthBase(266) == 13);
    REQUIRE(Zlib::lengthBase(269) == 19);
    REQUIRE(Zlib::lengthBase(273) == 35);
    REQUIRE(Zlib::lengthBase(277) == 67);
    REQUIRE(Zlib::lengthBase(281) == 131);
    // The last code is the special one, and means two hundred and fifty eight on its own.
    REQUIRE(Zlib::lengthBase(285) == 258);
    // The two codes above it name nothing.
    REQUIRE(Zlib::lengthBase(286) == 0);
    REQUIRE(Zlib::lengthBase(288) == 0);
    REQUIRE(Zlib::lengthBase(0) == 0);
}

TEST_CASE("Zlib counts the extra bits of a length code") {
    // The first eight carry none.
    REQUIRE(Zlib::lengthExtra(257) == 0);
    REQUIRE(Zlib::lengthExtra(264) == 0);
    // Then each four carries one more.
    REQUIRE(Zlib::lengthExtra(265) == 1);
    REQUIRE(Zlib::lengthExtra(268) == 1);
    REQUIRE(Zlib::lengthExtra(269) == 2);
    REQUIRE(Zlib::lengthExtra(273) == 3);
    REQUIRE(Zlib::lengthExtra(277) == 4);
    REQUIRE(Zlib::lengthExtra(281) == 5);
    // The special last code carries none despite sitting where the step would give six.
    REQUIRE(Zlib::lengthExtra(285) == 0);
    // Above the table there are none to count.
    REQUIRE(Zlib::lengthExtra(286) == 0);
}

TEST_CASE("Zlib names a base distance for each distance code") {
    // The first four are one to four.
    REQUIRE(Zlib::distanceBase(0) == 1);
    REQUIRE(Zlib::distanceBase(1) == 2);
    REQUIRE(Zlib::distanceBase(3) == 4);
    // Then pairs reaching four times as far.
    REQUIRE(Zlib::distanceBase(4) == 5);
    REQUIRE(Zlib::distanceBase(5) == 7);
    REQUIRE(Zlib::distanceBase(6) == 9);
    // The last pair is the furthest the format reaches.
    REQUIRE(Zlib::distanceBase(28) == 16385);
    REQUIRE(Zlib::distanceBase(29) == 24577);
    // Above the table there is no distance.
    REQUIRE(Zlib::distanceBase(30) == 0);
}

TEST_CASE("Zlib counts the extra bits of a distance code") {
    // The first four carry none and each pair after that carries one more, up to thirteen.
    REQUIRE(Zlib::distanceExtra(0) == 0);
    REQUIRE(Zlib::distanceExtra(3) == 0);
    REQUIRE(Zlib::distanceExtra(4) == 1);
    REQUIRE(Zlib::distanceExtra(6) == 2);
    REQUIRE(Zlib::distanceExtra(20) == 9);
    REQUIRE(Zlib::distanceExtra(28) == 13);
    REQUIRE(Zlib::distanceExtra(29) == 13);
    // Above the table there are none to count.
    REQUIRE(Zlib::distanceExtra(30) == 0);
}

TEST_CASE("Zlib agrees on the counts of the two tables") {
    // The table holds twenty nine length codes and thirty distance codes.
    REQUIRE(Zlib::kLengthCodeCount == 29);
    REQUIRE(Zlib::kDistanceCodeCount == 30);
    // Every length code names a base, and from two hundred and sixty five on each carries the
    // number of extra bits its position implies. The first eight carry none, so the formula
    // applies only above them; below, the subtraction would go negative.
    for (int code = 257; code <= 284; ++code) {
        REQUIRE(Zlib::lengthBase(code) > 0);
        if (code >= 265) {
            REQUIRE(Zlib::lengthExtra(code) == (code - 265) / 4 + 1);
        }
    }
    // Every distance code in range names a base and a bit count.
    for (int code = 0; code < 30; ++code) {
        REQUIRE(Zlib::distanceBase(code) > 0);
        REQUIRE(Zlib::distanceExtra(code) >= 0);
    }
}
