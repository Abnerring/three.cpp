#include "nodes/display/BumpMapNode.h"

#include "catch.hpp"

#include <string>

TEST_CASE("BumpMap skips a wireframe material") {
    // Only exactly true skips it, since the test is strict; a material that says nothing is not
    // treated as wireframe.
    REQUIRE(BumpMap::skipsBump(true));
    REQUIRE_FALSE(BumpMap::skipsBump(false));
    REQUIRE(std::string(BumpMap::kTypeName) == "BumpMapNode");
}

TEST_CASE("BumpMap uses a scale of nought as nought") {
    // The test is against null, so a scale that is present is used even when it is nought. A
    // truthy test would jump to one here and silently unflatten a deliberately flat material.
    REQUIRE(BumpMap::bumpScale(true, 0.0) == 0.0);
    REQUIRE(BumpMap::bumpScale(true, 0.0) != 1.0);
    // A present non zero scale is used as given.
    REQUIRE(BumpMap::bumpScale(true, 0.5) == 0.5);
    REQUIRE(BumpMap::bumpScale(true, -2.0) == -2.0);
    // Only an absent scale falls back to one.
    REQUIRE(BumpMap::bumpScale(false, 0.0) == 1.0);
    REQUIRE(BumpMap::bumpScale(false, 5.0) == 1.0);
}

TEST_CASE("BumpMap's gradient uses the sign and not the size") {
    // A positive determinant gives one and a negative one gives minus one, whichever size they
    // have; the size is what tells the two surfaces apart, and losing it is the point.
    REQUIRE(BumpMap::gradientSign(3.0) == 1.0);
    REQUIRE(BumpMap::gradientSign(0.001) == 1.0);
    REQUIRE(BumpMap::gradientSign(-3.0) == -1.0);
    REQUIRE(BumpMap::gradientSign(-0.001) == -1.0);
    // The sign does not change with the size, which is what makes it a sign.
    REQUIRE(BumpMap::gradientSign(3.0) == BumpMap::gradientSign(0.001));
    // A determinant of exactly nought has no sign, and gives nought rather than a choice.
    REQUIRE(BumpMap::gradientSign(0.0) == 0.0);
}

TEST_CASE("BumpMap's determinant is passed through") {
    // The determinant is whatever the dot product gave, before the facing is multiplied in.
    REQUIRE(BumpMap::determinant(2.5) == 2.5);
    REQUIRE(BumpMap::determinant(-0.5) == -0.5);
    REQUIRE(BumpMap::determinant(0.0) == 0.0);
}