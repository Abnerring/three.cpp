#include "loaders/LDrawLoader.h"

#include "catch.hpp"

#include <string>

TEST_CASE("LDrawLoader applies a bfc word") {
    // Certification sets the flag and the winding together.
    LDrawLoader::BfcState state;
    REQUIRE(LDrawLoader::applyBfcToken("CERTIFY", state));
    REQUIRE(state.certified);
    REQUIRE(state.ccw);
    // Decertifying clears the flag.
    REQUIRE(LDrawLoader::applyBfcToken("NOCERTIFY", state));
    REQUIRE_FALSE(state.certified);
}

TEST_CASE("LDrawLoader resets the winding on either certification word") {
    // Turn the winding round, then certify: the winding comes back to counter clockwise.
    LDrawLoader::BfcState state;
    REQUIRE(LDrawLoader::applyBfcToken("CW", state));
    REQUIRE_FALSE(state.ccw);
    REQUIRE(LDrawLoader::applyBfcToken("CERTIFY", state));
    REQUIRE(state.ccw);
    // The same happens with the decertifying word, which is the surprising part.
    REQUIRE(LDrawLoader::applyBfcToken("CW", state));
    REQUIRE_FALSE(state.ccw);
    REQUIRE(LDrawLoader::applyBfcToken("NOCERTIFY", state));
    REQUIRE(state.ccw);
}

TEST_CASE("LDrawLoader changes only the winding for its own words") {
    // A winding word leaves the certification alone.
    LDrawLoader::BfcState state;
    REQUIRE(LDrawLoader::applyBfcToken("CERTIFY", state));
    REQUIRE(LDrawLoader::applyBfcToken("CW", state));
    REQUIRE_FALSE(state.ccw);
    // The certification flag is untouched.
    REQUIRE(state.certified);
    // Counter clockwise turns it back.
    REQUIRE(LDrawLoader::applyBfcToken("CCW", state));
    REQUIRE(state.ccw);
}

TEST_CASE("LDrawLoader sets the inversion for the next object") {
    LDrawLoader::BfcState state;
    REQUIRE_FALSE(state.inverted);
    REQUIRE(LDrawLoader::applyBfcToken("INVERTNEXT", state));
    REQUIRE(state.inverted);
    // No word clears it; the caller does that after using it.
    REQUIRE(LDrawLoader::applyBfcToken("CCW", state));
    REQUIRE(state.inverted);
}

TEST_CASE("LDrawLoader sets the culling flag") {
    LDrawLoader::BfcState state;
    REQUIRE(state.cull);
    REQUIRE(LDrawLoader::applyBfcToken("NOCLIP", state));
    REQUIRE_FALSE(state.cull);
    REQUIRE(LDrawLoader::applyBfcToken("CLIP", state));
    REQUIRE(state.cull);
}

TEST_CASE("LDrawLoader reports a word it does not know") {
    LDrawLoader::BfcState state;
    // An unknown word leaves the state as it was.
    REQUIRE_FALSE(LDrawLoader::applyBfcToken("SIDEWAYS", state));
    REQUIRE_FALSE(state.certified);
    REQUIRE(state.ccw);
    REQUIRE(state.cull);
    // An empty word is unknown too.
    REQUIRE_FALSE(LDrawLoader::applyBfcToken("", state));
}
