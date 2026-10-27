#include "nodes/utils/FlipNode.h"

#include "catch.hpp"

#include <string>

TEST_CASE("FlipNode's components are named in a fixed order") {
    const auto& names = FlipNode::componentNames();
    REQUIRE(names.size() == 4);
    REQUIRE(names[0] == "x");
    REQUIRE(names[1] == "y");
    REQUIRE(names[2] == "z");
    REQUIRE(names[3] == "w");
    REQUIRE(std::string(FlipNode::kTypeName) == "FlipNode");
}

TEST_CASE("FlipNode flips the components a list names in order") {
    // The first two names flip the first two components and leave the rest.
    const auto firstTwo = FlipNode::flipped(4, "xy");
    REQUIRE(firstTwo[0]);
    REQUIRE(firstTwo[1]);
    REQUIRE_FALSE(firstTwo[2]);
    REQUIRE_FALSE(firstTwo[3]);
    // One name flips one component.
    const auto second = FlipNode::flipped(4, "y");
    REQUIRE_FALSE(second[0]);
    REQUIRE(second[1]);
    REQUIRE_FALSE(second[2]);
    REQUIRE_FALSE(second[3]);
    // A name that is not a component at all flips nothing, without complaining.
    const auto none = FlipNode::flipped(4, "q");
    for (const bool f : none) REQUIRE_FALSE(f);
    // And an empty list flips nothing.
    const auto empty = FlipNode::flipped(4, "");
    for (const bool f : empty) REQUIRE_FALSE(f);
}

TEST_CASE("FlipNode drops a name that does not match where the walk has reached") {
    // The walk looks for one name at a time and only advances when it finds a match. Asking for
    // the second then the first therefore matches the second at the step that looks for the
    // first, flips it, and then finds nothing for the first: the list behaves as if it had only
    // named the second. That is quieter than flipping nothing, which was my first reading.
    const auto reversed = FlipNode::flipped(4, "yx");
    REQUIRE_FALSE(reversed[0]);
    REQUIRE(reversed[1]);
    REQUIRE_FALSE(reversed[2]);
    REQUIRE_FALSE(reversed[3]);
    // The two lists therefore give the same answer, which is what a dropped name means.
    const auto onlySecond = FlipNode::flipped(4, "y");
    REQUIRE(reversed == onlySecond);
    // The same names in the right order flip both, so the loss is in the order.
    const auto ordered = FlipNode::flipped(4, "xy");
    REQUIRE(ordered[0]);
    REQUIRE(ordered[1]);
    // The two differ in the first component and agree in the second.
    REQUIRE(ordered[0] != reversed[0]);
    REQUIRE(ordered[1] == reversed[1]);
}

TEST_CASE("FlipNode can be asked whether a list is in the walk's order") {
    // In order, so nothing is dropped.
    REQUIRE(FlipNode::isInWalkOrder("x"));
    REQUIRE(FlipNode::isInWalkOrder("xy"));
    REQUIRE(FlipNode::isInWalkOrder("xyz"));
    REQUIRE(FlipNode::isInWalkOrder("w"));
    // Out of order, so a name is dropped silently.
    REQUIRE_FALSE(FlipNode::isInWalkOrder("yx"));
    REQUIRE_FALSE(FlipNode::isInWalkOrder("zy"));
    // A repeat is out of order too, since the walk never goes back.
    REQUIRE_FALSE(FlipNode::isInWalkOrder("xx"));
}

TEST_CASE("FlipNode's flip is one less the value") {
    // The two ends swap and the middle stays.
    REQUIRE(FlipNode::flip(0.0) == 1.0);
    REQUIRE(FlipNode::flip(1.0) == 0.0);
    REQUIRE(FlipNode::flip(0.25) == 0.75);
    // Applying it twice gives the value back.
    REQUIRE(FlipNode::flip(FlipNode::flip(0.3)) == Approx(0.3).epsilon(1e-12));
}