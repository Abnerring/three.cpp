#include "nodes/core/VarNode.h"

#include "catch.hpp"

#include <string>

TEST_CASE("VarNode's default flags are two on and two off") {
    // A variable is global and requires its parents.
    REQUIRE(VarNode::kGlobal);
    REQUIRE(VarNode::kParents);
    // It is neither read only nor an intent.
    REQUIRE_FALSE(VarNode::kReadOnly);
    REQUIRE_FALSE(VarNode::kIntent);
    // The names do not all say which is which, so the four are recorded rather than guessed.
    REQUIRE(VarNode::kGlobal != VarNode::kReadOnly);
    REQUIRE(VarNode::kParents != VarNode::kIntent);
    REQUIRE(VarNode::isVarNode);
    REQUIRE(std::string(VarNode::kTypeName) == "VarNode");
}

TEST_CASE("VarNode's name is absent by default") {
    // A variable made without a name gets a generated one rather than being refused.
    REQUIRE_FALSE(VarNode::hasNameByDefault());
}

TEST_CASE("VarNode's intent is asked of the builder, not of the node") {
    // The flag on the node is the request.
    REQUIRE(VarNode::intentRequested(true));
    REQUIRE_FALSE(VarNode::intentRequested(false));
    // But the question of whether the node is an intent is answered from the builder's record, so
    // the two can disagree until the builder has looked at it.
    REQUIRE(VarNode::intentComesFromTheBuilder());
    // Setting the flag answers the node itself so the call can be chained.
    REQUIRE(VarNode::setterReturnsTheNode());
    REQUIRE_FALSE(VarNode::isCacheable());
}