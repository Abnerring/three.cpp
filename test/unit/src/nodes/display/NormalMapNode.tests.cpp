#include "nodes/display/NormalMapNode.h"

#include "catch.hpp"

#include <string>

using Packing = NormalMap::Packing;
using Channels = NormalMap::Channels;

TEST_CASE("NormalMap reads a green alpha map from the second and fourth parts") {
    // A red green map keeps its pair in the first two parts.
    REQUIRE(NormalMap::channelsFor(Packing::RedGreen) == Channels::Xy);
    // A green alpha map keeps them in the second and fourth, which is not the second and third
    // and not the first two; reading it as the red green case would give a wrong direction for
    // every pixel.
    REQUIRE(NormalMap::channelsFor(Packing::GreenAlpha) == Channels::Yw);
    REQUIRE(NormalMap::channelsFor(Packing::GreenAlpha) != Channels::Xy);
    // Unpacked maps use the whole colour.
    REQUIRE(NormalMap::channelsFor(Packing::None) == Channels::Whole);
}

TEST_CASE("NormalMap's three packings are all handled") {
    // Every member of the enumeration is one the node knows, so the unexpected branch is
    // unreachable through this interface; the test says so rather than leaving it implied.
    REQUIRE_FALSE(NormalMap::isUnexpected(Packing::None));
    REQUIRE_FALSE(NormalMap::isUnexpected(Packing::RedGreen));
    REQUIRE_FALSE(NormalMap::isUnexpected(Packing::GreenAlpha));
}

TEST_CASE("NormalMap names an unexpected mode in its message") {
    // The message carries the mode's own name, so a reader can tell which one it was.
    REQUIRE(NormalMap::unexpectedModeMessage("SomethingElse") ==
            "THREE.NodeMaterial: Unexpected unpack normal mode: SomethingElse");
    // The prefix is fixed and the name comes after it.
    REQUIRE(NormalMap::unexpectedModeMessage("X").find("THREE.NodeMaterial:") == 0);
    REQUIRE(NormalMap::unexpectedModeMessage("X") != NormalMap::unexpectedModeMessage("Y"));
}

TEST_CASE("NormalMap takes a sampled value into a signed range") {
    // The range nought to one becomes minus one to one.
    REQUIRE(NormalMap::signedValue(0.0) == -1.0);
    REQUIRE(NormalMap::signedValue(1.0) == 1.0);
    // Halfway, which is the flat normal, becomes nought.
    REQUIRE(NormalMap::signedValue(0.5) == 0.0);
    // The two ends are symmetric about that middle.
    REQUIRE(NormalMap::signedValue(1.0) == -NormalMap::signedValue(0.0));
}