#include "nodes/utils/JoinNode.h"

#include "catch.hpp"

#include <string>
#include <vector>

using Overrun = JoinNode::Overrun;

TEST_CASE("JoinNode's width is the sum of its parts") {
    // Two single values make a pair.
    REQUIRE(JoinNode::joinedWidth({1, 1}) == 2);
    // One and three make four.
    REQUIRE(JoinNode::joinedWidth({1, 3}) == 4);
    // Three and one also make four, so the order does not matter.
    REQUIRE(JoinNode::joinedWidth({3, 1}) == 4);
    // Nothing at all makes nought, which is a width no type has.
    REQUIRE(JoinNode::joinedWidth({}) == 0);
    REQUIRE(std::string(JoinNode::kTypeName) == "JoinNode");
}

TEST_CASE("JoinNode can build a width no type has") {
    // Two parts of three and two make five, and the node builds a five part type rather than
    // refusing, because nothing checks that the sum is a width that exists. This is the part a
    // reader would expect to be rejected.
    REQUIRE(JoinNode::joinedWidth({3, 2}) == 5);
    REQUIRE(JoinNode::joinedWidth({3, 2}) != 4);
    // Four parts of one make four, which does exist, so a valid answer can come from the same
    // rule.
    REQUIRE(JoinNode::joinedWidth({1, 1, 1, 1}) == 4);
    // Six parts of one make six, which does not.
    REQUIRE(JoinNode::joinedWidth({1, 1, 1, 1, 1, 1}) == 6);
}

TEST_CASE("JoinNode has two overrun checks with different comparisons") {
    // A running count that has already reached the maximum is the first case, and it uses
    // greater or equal: being exactly full counts.
    REQUIRE(JoinNode::checkOverrun(4, 4, 1) == Overrun::AlreadyFull);
    REQUIRE(JoinNode::checkOverrun(5, 4, 1) == Overrun::AlreadyFull);
    // A part that would pass the maximum is the second, and it uses greater alone: a part that
    // exactly fills the space is fine.
    REQUIRE(JoinNode::checkOverrun(3, 4, 2) == Overrun::Truncated);
    REQUIRE(JoinNode::checkOverrun(3, 4, 1) == Overrun::None);
    REQUIRE(JoinNode::checkOverrun(0, 4, 4) == Overrun::None);
    // The two comparisons therefore differ at the exact boundary, which is the point.
    REQUIRE(JoinNode::checkOverrun(4, 4, 1) == Overrun::AlreadyFull);
    REQUIRE(JoinNode::checkOverrun(0, 4, 4) == Overrun::None);
}

TEST_CASE("JoinNode cuts a part that does not fit and carries on") {
    // What is left of the maximum is what the part is cut down to.
    REQUIRE(JoinNode::truncatedWidth(3, 4) == 1);
    REQUIRE(JoinNode::truncatedWidth(0, 2) == 2);
    REQUIRE(JoinNode::truncatedWidth(2, 4) == 2);
    // Both overruns print a message and neither stops the work, so a caller that ignores the
    // message gets a patched result rather than a failure.
    REQUIRE(JoinNode::printsMessage(Overrun::AlreadyFull));
    REQUIRE(JoinNode::printsMessage(Overrun::Truncated));
    REQUIRE_FALSE(JoinNode::printsMessage(Overrun::None));
}

TEST_CASE("JoinNode's two messages differ and name the type") {
    // Each names the type it was building, and the two say different things.
    REQUIRE(JoinNode::alreadyFullMessage("vec4") ==
            "TSL: Length of parameters exceeds maximum length of function 'vec4()' type.");
    REQUIRE(JoinNode::truncatedMessage("vec4") ==
            "TSL: Length of 'vec4()' data exceeds maximum length of output type.");
    REQUIRE(JoinNode::alreadyFullMessage("vec4") != JoinNode::truncatedMessage("vec4"));
    // A different type gives a different sentence, so the name really is part of it.
    REQUIRE(JoinNode::alreadyFullMessage("vec4") != JoinNode::alreadyFullMessage("vec3"));
}