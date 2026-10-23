#include "loaders/EXRLoader.h"

#include "catch.hpp"

#include <array>

TEST_CASE("EXRLoader leaves an empty block empty") {
    // A block of zeros has nothing to transform.
    double block[64] = {};
    EXRLoader::dctInverse(block);
    for (int i = 0; i < 64; ++i) REQUIRE(block[i] == 0.0);
}

TEST_CASE("EXRLoader spreads a constant through the block") {
    // Only the first coefficient is set, so every output is the same number: the constant
    // squared, since the transform runs over the rows and then the columns. The constant is a
    // cosine of a pi written to five places, so the square is a hair under an eighth and the
    // tolerance is one of those rather than one of the last bits.
    // squared, since the transform runs over the rows and then the columns.
    double block[64] = {};
    block[0] = 8.0;
    EXRLoader::dctInverse(block);
    for (int i = 0; i < 64; ++i) REQUIRE(block[i] == Approx(1.0).margin(1e-6));

    // Doubling the coefficient doubles every output.
    double doubled[64] = {};
    doubled[0] = 16.0;
    EXRLoader::dctInverse(doubled);
    for (int i = 0; i < 64; ++i) REQUIRE(doubled[i] == Approx(2.0).margin(1e-6));
}

TEST_CASE("EXRLoader transforms in proportion") {
    // The transform is linear, so doubling every coefficient doubles every output. This holds
    // whatever the coefficients are, which is what makes it a better check than a guess about
    // which cells a particular coefficient reaches.
    double small[64] = {};
    small[0] = 4.0;
    small[5] = 2.0;
    small[27] = -1.0;
    double large[64] = {};
    large[0] = 8.0;
    large[5] = 4.0;
    large[27] = -2.0;

    EXRLoader::dctInverse(small);
    EXRLoader::dctInverse(large);
    for (int i = 0; i < 64; ++i) {
        REQUIRE(large[i] == Approx(2.0 * small[i]).margin(1e-6));
    }
}
TEST_CASE("EXRLoader puts the energy where the coefficient says") {
    // A constant plus one frequency gives a block that is not constant, so the two cases above
    // are telling different things apart.
    double flat[64] = {};
    flat[0] = 8.0;
    EXRLoader::dctInverse(flat);
    double mixed[64] = {};
    mixed[0] = 8.0;
    mixed[1] = 8.0;
    EXRLoader::dctInverse(mixed);
    bool differs = false;
    for (int i = 0; i < 64; ++i) {
        if (flat[i] != mixed[i]) differs = true;
    }
    REQUIRE(differs);
}