#include "loaders/LDrawLoader.h"

#include "catch.hpp"

TEST_CASE("LDrawLoader consumes the reference flags") {
    LDrawLoader::BfcState state;
    bool startingStep = false;
    // Nothing set, nothing taken.
    const auto quiet = LDrawLoader::consumeReferenceFlags(state, startingStep);
    REQUIRE_FALSE(quiet.inverted);
    REQUIRE_FALSE(quiet.startingBuildingStep);
}

TEST_CASE("LDrawLoader clears the flags as it reads them") {
    LDrawLoader::BfcState state;
    bool startingStep = false;
    // The inversion is set by its directive.
    REQUIRE(LDrawLoader::applyBfcToken("INVERTNEXT", state));
    REQUIRE(state.inverted);
    // Reading it takes it and clears it.
    const auto first = LDrawLoader::consumeReferenceFlags(state, startingStep);
    REQUIRE(first.inverted);
    REQUIRE_FALSE(state.inverted);
}

TEST_CASE("LDrawLoader gives the second reference quiet flags") {
    LDrawLoader::BfcState state;
    bool startingStep = true;
    REQUIRE(LDrawLoader::applyBfcToken("INVERTNEXT", state));
    // The first reference takes both.
    const auto first = LDrawLoader::consumeReferenceFlags(state, startingStep);
    REQUIRE(first.inverted);
    REQUIRE(first.startingBuildingStep);
    // The one straight after gets neither.
    const auto second = LDrawLoader::consumeReferenceFlags(state, startingStep);
    REQUIRE_FALSE(second.inverted);
    REQUIRE_FALSE(second.startingBuildingStep);
}

TEST_CASE("LDrawLoader keeps the two flags apart") {
    LDrawLoader::BfcState state;
    bool startingStep = false;
    // A step marker with no inversion.
    startingStep = true;
    const auto stepOnly = LDrawLoader::consumeReferenceFlags(state, startingStep);
    REQUIRE_FALSE(stepOnly.inverted);
    REQUIRE(stepOnly.startingBuildingStep);
    // An inversion with no step marker.
    REQUIRE(LDrawLoader::applyBfcToken("INVERTNEXT", state));
    const auto invertOnly = LDrawLoader::consumeReferenceFlags(state, startingStep);
    REQUIRE(invertOnly.inverted);
    REQUIRE_FALSE(invertOnly.startingBuildingStep);
}

TEST_CASE("LDrawLoader leaves the rest of the bfc state alone") {
    LDrawLoader::BfcState state;
    bool startingStep = false;
    // Certify and cull off, then take the flags.
    REQUIRE(LDrawLoader::applyBfcToken("CERTIFY", state));
    REQUIRE(LDrawLoader::applyBfcToken("NOCLIP", state));
    LDrawLoader::consumeReferenceFlags(state, startingStep);
    // Culling off and certification are untouched by the consumption.
    REQUIRE(state.certified);
    REQUIRE_FALSE(state.cull);
    REQUIRE(state.ccw);
}