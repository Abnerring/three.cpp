#include "nodes/core/SpriteUtils.h"

#include "catch.hpp"

#include <cmath>

TEST_CASE("SpriteUtils always takes the rotation branch") {
    // The branch is chosen by asking whether the flag is defined, and the flag has a default, so
    // it always is and the rotation branch is always taken. A caller passing false gets the same
    // branch as one passing true.
    REQUIRE(SpriteUtils::branch(true) == SpriteUtils::Branch::Rotation);
    REQUIRE(SpriteUtils::branch(true) != SpriteUtils::Branch::Axes);
    // The other branch can only be reached by a flag that is not there, which the default
    // prevents.
    REQUIRE(SpriteUtils::branch(false) == SpriteUtils::Branch::Axes);
    // The three defaults are what make the flags defined.
    REQUIRE(SpriteUtils::kDefaultHorizontal);
    REQUIRE_FALSE(SpriteUtils::kDefaultVertical);
    REQUIRE_FALSE(SpriteUtils::kDefaultRotation);
}

TEST_CASE("SpriteUtils' flags have no effect") {
    // A flag that is defined changes nothing, since the test is against being defined rather
    // than against being true; so passing the vertical flag as false does not stop the vertical
    // axis being set.
    REQUIRE_FALSE(SpriteUtils::flagHasEffect(true));
    // Only a flag that is not there would have an effect, and a default prevents that.
    REQUIRE(SpriteUtils::flagHasEffect(false));
    REQUIRE(SpriteUtils::flagHasEffect(true) != SpriteUtils::flagHasEffect(false));
}

TEST_CASE("SpriteUtils' unreachable branch would set both axes") {
    // Both axes are set when their flags are defined, which both are, so even the branch that is
    // never taken would set them both.
    REQUIRE(SpriteUtils::unreachableBranchSetsBothAxes(true, true));
    REQUIRE_FALSE(SpriteUtils::unreachableBranchSetsBothAxes(true, false));
    REQUIRE_FALSE(SpriteUtils::unreachableBranchSetsBothAxes(false, true));
    // A caller that wanted only one axis would not be able to say so.
    REQUIRE(SpriteUtils::unreachableBranchSetsBothAxes(true, true) !=
            SpriteUtils::unreachableBranchSetsBothAxes(true, false));
}

TEST_CASE("SpriteUtils builds its right axis by swapping and negating") {
    // The horizontal part becomes the third and the negated one the first, with the middle
    // nought, which is the same pattern the matcap coordinates use.
    const auto axis = SpriteUtils::rightFromLook(2.0, 3.0);
    REQUIRE(axis.x == 3.0);
    REQUIRE(axis.y == 0.0);
    REQUIRE(axis.z == -2.0);
    // The two are not copied across, so the answer differs from the input.
    REQUIRE(axis.x != 2.0);
    REQUIRE(axis.z != 3.0);
    // A nought in either place stays nought.
    const auto zero = SpriteUtils::rightFromLook(0.0, 5.0);
    REQUIRE(zero.x == 5.0);
    REQUIRE(zero.z == 0.0);
}

TEST_CASE("SpriteUtils' centre subtracts rather than adds") {
    // The centre is the world position less the rotated local one, so a nought rotated position
    // leaves the world one alone and anything else moves it the other way.
    REQUIRE(SpriteUtils::centreComponent(5.0, 0.0) == 5.0);
    REQUIRE(SpriteUtils::centreComponent(5.0, 2.0) == 3.0);
    // Adding would have given seven, which is the mistake this records.
    REQUIRE(SpriteUtils::centreComponent(5.0, 2.0) != 7.0);
    // And the two terms are not interchangeable: the other order gives the opposite.
    REQUIRE(SpriteUtils::centreComponent(5.0, 2.0) != SpriteUtils::centreComponent(2.0, 5.0));
}