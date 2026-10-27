#include "nodes/utils/SplitNode.h"

#include "catch.hpp"

#include <string>

TEST_CASE("SplitNode's width is the largest position it names, not the number of names") {
    // In order and contiguous the two happen to agree.
    REQUIRE(SplitNode::vectorLength("x") == 1);
    REQUIRE(SplitNode::vectorLength("xy") == 2);
    REQUIRE(SplitNode::vectorLength("xyz") == 3);
    REQUIRE(SplitNode::vectorLength("xyzw") == 4);
    // But naming the fourth alone needs four places, not one.
    REQUIRE(SplitNode::vectorLength("w") == 4);
    REQUIRE(SplitNode::vectorLength("w") != 1);
    // And naming the first and third needs three, not two.
    REQUIRE(SplitNode::vectorLength("xz") == 3);
    REQUIRE(SplitNode::vectorLength("xz") != 2);
    // Out of order is the same as in order for this question.
    REQUIRE(SplitNode::vectorLength("zx") == 3);
    REQUIRE(SplitNode::vectorLength("zy") == 3);
    // No names at all needs nothing.
    REQUIRE(SplitNode::vectorLength("") == 0);
    // A name that is not a component contributes nothing.
    REQUIRE(SplitNode::vectorLength("q") == 1);
    REQUIRE(std::string(SplitNode::kTypeName) == "SplitNode");
}

TEST_CASE("SplitNode writes no swizzle when reading the whole value in order") {
    // As many names as the value is wide, in order, is the value itself.
    REQUIRE(SplitNode::swizzleIsIdentity(2, "xy"));
    REQUIRE(SplitNode::swizzleIsIdentity(3, "xyz"));
    REQUIRE(SplitNode::swizzleIsIdentity(4, "xyzw"));
    // One name from a one part value is also the whole of it.
    REQUIRE(SplitNode::swizzleIsIdentity(1, "x"));
    // Out of order with the same count is not: the names are all there but rearranged.
    REQUIRE_FALSE(SplitNode::swizzleIsIdentity(2, "yx"));
    REQUIRE_FALSE(SplitNode::swizzleIsIdentity(3, "xzy"));
    // Fewer names than the width is a real read.
    REQUIRE_FALSE(SplitNode::swizzleIsIdentity(2, "x"));
    REQUIRE_FALSE(SplitNode::swizzleIsIdentity(4, "xyz"));
    // And more names cannot be the value itself.
    REQUIRE_FALSE(SplitNode::swizzleIsIdentity(2, "xyz"));
}

TEST_CASE("SplitNode ignores the names for a one part value") {
    // A float has nothing to read out of it, so the names are not consulted; asking a float for
    // its second component gives the float.
    REQUIRE_FALSE(SplitNode::componentsAreRead(1));
    // Anything wider does consult them.
    REQUIRE(SplitNode::componentsAreRead(2));
    REQUIRE(SplitNode::componentsAreRead(3));
    REQUIRE(SplitNode::componentsAreRead(4));
    // The threshold is exactly at one, which is where the source's test sits.
    REQUIRE(SplitNode::componentsAreRead(2) != SplitNode::componentsAreRead(1));
}

TEST_CASE("SplitNode refuses to be cached") {
    REQUIRE_FALSE(SplitNode::isCacheable());
}