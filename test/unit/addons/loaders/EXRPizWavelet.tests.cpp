#include "loaders/EXRLoader.h"

#include "catch.hpp"

#include <cstdint>

TEST_CASE("EXRLoader takes the smaller side for the wavelet") {
    // A square window gives that side.
    REQUIRE(EXRLoader::pizWaveletExtent(8, 8) == 8);
    // A wide window steps along its height and a tall one along its width.
    REQUIRE(EXRLoader::pizWaveletExtent(16, 4) == 4);
    REQUIRE(EXRLoader::pizWaveletExtent(4, 16) == 4);
    // A window of one is a single sample.
    REQUIRE(EXRLoader::pizWaveletExtent(1, 1) == 1);
}

TEST_CASE("EXRLoader halves the largest power of two that fits") {
    // Two doubles to four which passes two, so the largest power of two that fits is two and the
    // first step is half of it, one.
    REQUIRE(EXRLoader::pizWaveletFirstStep(2) == 1);
    // Eight gives four, and one gives nothing at all since one doubled already passes it.
    REQUIRE(EXRLoader::pizWaveletFirstStep(8) == 4);
    REQUIRE(EXRLoader::pizWaveletFirstStep(4) == 2);
    REQUIRE(EXRLoader::pizWaveletFirstStep(1) == 0);
    // An extent that is one short of a power of two still falls back to the one below it: nine
    // fits eight, so the step is four.
    REQUIRE(EXRLoader::pizWaveletFirstStep(9) == 4);
    REQUIRE(EXRLoader::pizWaveletFirstStep(15) == 4);
    REQUIRE(EXRLoader::pizWaveletFirstStep(16) == 8);
}

TEST_CASE("EXRLoader widens the arithmetic at sixteen thousand") {
    // Values below sixteen thousand three hundred and eighty four are handled narrowly.
    REQUIRE_FALSE(EXRLoader::pizWaveletWide(0));
    REQUIRE_FALSE(EXRLoader::pizWaveletWide(16383));
    // That value itself is where the wider arithmetic begins.
    REQUIRE(EXRLoader::pizWaveletWide(16384));
    REQUIRE(EXRLoader::pizWaveletWide(65535));
}

TEST_CASE("EXRLoader sizes a PIZ row") {
    // A row is the channel's width times the channel's own width in words.
    REQUIRE(EXRLoader::pizRowLength(4, 1) == 4);
    REQUIRE(EXRLoader::pizRowLength(4, 2) == 8);
    REQUIRE(EXRLoader::pizRowLength(1, 1) == 1);
    REQUIRE(EXRLoader::pizRowLength(0, 2) == 0);
}