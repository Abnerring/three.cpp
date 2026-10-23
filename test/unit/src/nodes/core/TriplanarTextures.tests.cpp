#include "nodes/core/TriplanarTextures.h"

#include "catch.hpp"

#include <cmath>

using Weights = TriplanarTextures::Weights;
using Pair = TriplanarTextures::Pair;

TEST_CASE("TriplanarTextures' weights always add to one") {
    // Whichever way the surface faces, the three weights add to one, which is what the second
    // division is for; normalising alone would leave the sum somewhere else.
    const double cases[][3] = {{0, 0, 1}, {0, 3, 4}, {1, 1, 1}, {0.2, 0.3, 0.7}, {1, 0, 0}};
    for (const auto& n : cases) {
        const Weights w = TriplanarTextures::weights(n[0], n[1], n[2]);
        REQUIRE(w.x + w.y + w.z == Approx(1.0).epsilon(1e-12));
    }
    // A normal along one axis puts all the weight there.
    const Weights alongZ = TriplanarTextures::weights(0.0, 0.0, 1.0);
    REQUIRE(alongZ.x == Approx(0.0).margin(1e-12));
    REQUIRE(alongZ.z == Approx(1.0).epsilon(1e-12));
    // A normal at forty five degrees to two axes splits evenly.
    const Weights diagonal = TriplanarTextures::weights(1.0, 1.0, 0.0);
    REQUIRE(diagonal.x == Approx(0.5).epsilon(1e-12));
    REQUIRE(diagonal.y == Approx(0.5).epsilon(1e-12));
}

TEST_CASE("TriplanarTextures' weights use the absolute parts") {
    // A negative part weighs the same as a positive one, so a normal and its negation blend the
    // same way, and that is what drawing both sides of a surface needs.
    const Weights forward = TriplanarTextures::weights(0.2, -0.3, 0.7);
    const Weights flipped = TriplanarTextures::weights(-0.2, 0.3, -0.7);
    REQUIRE(forward.x == Approx(flipped.x).epsilon(1e-12));
    REQUIRE(forward.y == Approx(flipped.y).epsilon(1e-12));
    REQUIRE(forward.z == Approx(flipped.z).epsilon(1e-12));
    // The sum the weights are divided by is the same for both.
    REQUIRE(TriplanarTextures::weightSum(0.2, -0.3, 0.7) ==
            Approx(TriplanarTextures::weightSum(-0.2, 0.3, -0.7)).epsilon(1e-12));
    REQUIRE(std::string(TriplanarTextures::kTypeName) == "TriplanarTextures");
}

TEST_CASE("TriplanarTextures' coordinate pairs run in a cycle") {
    // The first axis takes the second and third parts.
    const Pair x = TriplanarTextures::sampleCoordinates('X', 1.0, 2.0, 3.0, 1.0);
    REQUIRE(x.u == 2.0);
    REQUIRE(x.v == 3.0);
    // The second takes the third then the first, which is not the first then the third: a
    // mapping built from the axis's complement would give one and three here and would mirror
    // that texture.
    const Pair y = TriplanarTextures::sampleCoordinates('Y', 1.0, 2.0, 3.0, 1.0);
    REQUIRE(y.u == 3.0);
    REQUIRE(y.v == 1.0);
    REQUIRE(y.u != 1.0);
    REQUIRE(y.v != 3.0);
    // The third takes the first and second.
    const Pair z = TriplanarTextures::sampleCoordinates('Z', 1.0, 2.0, 3.0, 1.0);
    REQUIRE(z.u == 1.0);
    REQUIRE(z.v == 2.0);
}

TEST_CASE("TriplanarTextures' scale multiplies both coordinates") {
    // The scale applies to each part of the pair, so doubling it doubles both.
    const Pair scaled = TriplanarTextures::sampleCoordinates('X', 1.0, 2.0, 3.0, 2.0);
    REQUIRE(scaled.u == 4.0);
    REQUIRE(scaled.v == 6.0);
    // A scale of one leaves them alone, and a scale of a half halves them.
    const Pair plain = TriplanarTextures::sampleCoordinates('Z', 4.0, 6.0, 0.0, 1.0);
    REQUIRE(plain.u == 4.0);
    const Pair half = TriplanarTextures::sampleCoordinates('Z', 4.0, 6.0, 0.0, 0.5);
    REQUIRE(half.u == 2.0);
    REQUIRE(half.v == 3.0);
    // The other two textures fall back to the first when not given.
    REQUIRE(TriplanarTextures::missingTexturesFallBackToFirst);
}