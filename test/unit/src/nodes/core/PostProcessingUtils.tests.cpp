#include "nodes/core/PostProcessingUtils.h"

#include "catch.hpp"

#include <string>

using Pair = PostProcessingUtils::Pair;
using Clip = PostProcessingUtils::Clip;

TEST_CASE("PostProcessingUtils' background test points the other way when depth is reversed") {
    // In the ordinary case the background is a depth of one or more, and the boundary counts.
    REQUIRE(PostProcessingUtils::isBackgroundDepth(false, 1.0));
    REQUIRE(PostProcessingUtils::isBackgroundDepth(false, 1.5));
    REQUIRE_FALSE(PostProcessingUtils::isBackgroundDepth(false, 0.5));
    REQUIRE_FALSE(PostProcessingUtils::isBackgroundDepth(false, 0.0));
    // Reversed, it is a depth of nought or less, so the same two values give the opposite
    // answers. A test that assumed one direction for both would be wrong for every value.
    REQUIRE(PostProcessingUtils::isBackgroundDepth(true, 0.0));
    REQUIRE(PostProcessingUtils::isBackgroundDepth(true, -0.5));
    REQUIRE_FALSE(PostProcessingUtils::isBackgroundDepth(true, 0.5));
    REQUIRE_FALSE(PostProcessingUtils::isBackgroundDepth(true, 1.0));
    // The middle of the range is not the background either way.
    REQUIRE_FALSE(PostProcessingUtils::isBackgroundDepth(false, 0.5));
    REQUIRE_FALSE(PostProcessingUtils::isBackgroundDepth(true, 0.5));
}

TEST_CASE("PostProcessingUtils scales the depth in only one of its two cases") {
    // With a reversed depth buffer or the newer coordinate system, the depth is already in clip
    // space and is passed through as it is, while the two screen coordinates are taken into
    // clip space.
    const Clip newer = PostProcessingUtils::clipFromScreen(true, 0.5, 0.5, 0.25);
    REQUIRE(newer.z == 0.25);
    REQUIRE(newer.z != 0.25 * 2.0 - 1.0);
    // In the ordinary case all three are taken into clip space together, so the depth is scaled
    // as well and the same input gives a different third part.
    const Clip ordinary = PostProcessingUtils::clipFromScreen(false, 0.5, 0.5, 0.25);
    REQUIRE(ordinary.z == 0.25 * 2.0 - 1.0);
    REQUIRE(ordinary.z != newer.z);
    // The first two parts agree, so the difference is only in the third.
    REQUIRE(ordinary.x == newer.x);
    REQUIRE(ordinary.y == newer.y);
    // The vertical coordinate is turned over in both, which a test checks by comparing against
    // an unturned value.
    REQUIRE(newer.y == (1.0 - 0.5) * 2.0 - 1.0);
}

TEST_CASE("PostProcessingUtils takes the middle to the middle") {
    // The centre of the screen stays the centre, so the range map and the turn agree there.
    const Clip centre = PostProcessingUtils::clipFromScreen(false, 0.5, 0.5, 0.5);
    REQUIRE(centre.x == 0.0);
    REQUIRE(centre.y == 0.0);
    const Clip reversedCentre = PostProcessingUtils::clipFromScreen(true, 0.5, 0.5, 0.5);
    REQUIRE(reversedCentre.x == 0.0);
    REQUIRE(reversedCentre.y == 0.0);
    // And a screen position from a centre clip position gives the centre back.
    const Pair back = PostProcessingUtils::screenFromClip(0.0, 0.0, 1.0);
    REQUIRE(back.x == 0.5);
    REQUIRE(back.y == 0.5);
}

TEST_CASE("PostProcessingUtils turns the vertical coordinate after the range map") {
    // A clip position at the top gives a screen y of nought, not one: the turn comes after the
    // range map, so the two orders are not interchangeable.
    const Pair top = PostProcessingUtils::screenFromClip(0.0, 1.0, 1.0);
    REQUIRE(top.y == 0.0);
    REQUIRE(top.y != 1.0);
    // And one at the bottom gives one.
    const Pair bottom = PostProcessingUtils::screenFromClip(0.0, -1.0, 1.0);
    REQUIRE(bottom.y == 1.0);
    // The horizontal is not turned, so it keeps the range map's answer.
    REQUIRE(top.x == 0.5);
    const Pair right = PostProcessingUtils::screenFromClip(1.0, 0.0, 1.0);
    REQUIRE(right.x == 1.0);
    // A fourth part other than one divides first, so the same first two parts give a different
    // answer when the fourth differs.
    const Pair halved = PostProcessingUtils::screenFromClip(1.0, 0.0, 2.0);
    REQUIRE(halved.x == 0.75);
    REQUIRE(halved.x != right.x);
    REQUIRE(std::string(PostProcessingUtils::kDepthKind) == "linear");
}