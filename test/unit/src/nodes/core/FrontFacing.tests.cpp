#include "nodes/core/FrontFacing.h"

#include "catch.hpp"

#include <string>

using Side = FrontFacing::Side;
using Answer = FrontFacing::Answer;

TEST_CASE("FrontFacing answers true outside the fragment stage") {
    // Facing is a question only the fragment stage can answer, so any other stage says true
    // without asking anything.
    REQUIRE(FrontFacing::answer(false, Side::Front) == Answer::True);
    REQUIRE(FrontFacing::answer(false, Side::Back) == Answer::True);
    REQUIRE(FrontFacing::answer(false, Side::Double) == Answer::True);
    // The stage is checked before the side, so even a back sided material says true there.
    REQUIRE(FrontFacing::answer(false, Side::Back) == Answer::True);
}

TEST_CASE("FrontFacing answers false for a back sided material") {
    // In the fragment stage a back sided material is never front facing.
    REQUIRE(FrontFacing::answer(true, Side::Back) == Answer::False);
    // The other two ask the builder.
    REQUIRE(FrontFacing::answer(true, Side::Front) == Answer::AskBuilder);
    REQUIRE(FrontFacing::answer(true, Side::Double) == Answer::AskBuilder);
    // The three answers are not all the same, which is the point of the order of the checks.
    REQUIRE(FrontFacing::answer(true, Side::Back) != FrontFacing::answer(true, Side::Front));
}

TEST_CASE("FrontFacing turns its answer into a number") {
    // Doubling and taking one away gives one for true and minus one for false.
    REQUIRE(FrontFacing::faceDirection(true) == 1.0);
    REQUIRE(FrontFacing::faceDirection(false) == -1.0);
    // The two are not symmetric about zero, which is what the doubling does.
    REQUIRE(FrontFacing::faceDirection(true) + FrontFacing::faceDirection(false) == 0.0);
    REQUIRE(FrontFacing::faceDirection(true) == -FrontFacing::faceDirection(false));
}

TEST_CASE("FrontFacing multiplies a vector by one of three factors") {
    // Sided to the front leaves the vector alone whatever the fragment says.
    REQUIRE(FrontFacing::backSideFactor(Side::Front, true) == 1.0);
    REQUIRE(FrontFacing::backSideFactor(Side::Front, false) == 1.0);
    // Sided to the back always turns it over.
    REQUIRE(FrontFacing::backSideFactor(Side::Back, true) == -1.0);
    REQUIRE(FrontFacing::backSideFactor(Side::Back, false) == -1.0);
    // Sided both ways follows the facing, so the two answers differ and are the two numbers.
    REQUIRE(FrontFacing::backSideFactor(Side::Double, true) == 1.0);
    REQUIRE(FrontFacing::backSideFactor(Side::Double, false) == -1.0);
    REQUIRE(FrontFacing::backSideFactor(Side::Double, true) !=
            FrontFacing::backSideFactor(Side::Double, false));
}

TEST_CASE("FrontFacing's renamed function says what it became") {
    REQUIRE(FrontFacing::deprecatedRenameWarning() ==
            "TSL: \"directionToFaceDirection()\" has been renamed to \"negateOnBackSide()\".");
    REQUIRE(std::string(FrontFacing::kTypeName) == "FrontFacingNode");
}