#include "nodes/core/NodeBuilderContext.h"

#include "catch.hpp"

#include <map>
#include <string>

TEST_CASE("NodeBuilder's added context wins where the two overlap") {
    // Overlapping entries take the added value.
    const auto one = NodeBuilderContext::merged({{"a", 1}}, {{"a", 2}});
    REQUIRE(one.at("a") == 2);
    REQUIRE(one.at("a") != 1);
    // Entries that do not overlap are kept from both, so the merge adds rather than replaces.
    const auto both = NodeBuilderContext::merged({{"a", 1}}, {{"b", 2}});
    REQUIRE(both.size() == 2);
    REQUIRE(both.at("a") == 1);
    REQUIRE(both.at("b") == 2);
    // And the call answers with the context that was there before, which is what a caller puts
    // back afterwards.
    REQUIRE(NodeBuilderContext::returnsThePreviousContext());
}

TEST_CASE("NodeBuilder's shared context drops exactly two entries") {
    // The material and the coordinates getter, both belonging to one material rather than to the
    // shader construction.
    const auto removed = NodeBuilderContext::entriesRemovedForSharing();
    REQUIRE(removed.size() == 2);
    REQUIRE(removed[0] == "material");
    REQUIRE(removed[1] == "getUV");
    // Those two and no others.
    REQUIRE(NodeBuilderContext::isRemovedForSharing("material"));
    REQUIRE(NodeBuilderContext::isRemovedForSharing("getUV"));
    REQUIRE_FALSE(NodeBuilderContext::isRemovedForSharing("inspector"));
    REQUIRE_FALSE(NodeBuilderContext::isRemovedForSharing(""));
    REQUIRE_FALSE(NodeBuilderContext::isRemovedForSharing("getUv"));
}

TEST_CASE("NodeBuilder adds a stack and keeps two records of its nesting") {
    // A child is made from the current one, the previous is saved, and the new one becomes
    // current; both the builder's chain and the module's notion of the current stack move.
    const auto step = NodeBuilderContext::addStack();
    REQUIRE(step.makesAChild);
    REQUIRE(step.savesThePrevious);
    REQUIRE(step.becomesCurrent);
    REQUIRE(NodeBuilderContext::nestingRecordsKept() == 2);
}

TEST_CASE("NodeBuilder marks every collected node before stepping back") {
    // Each node collected in the stack is marked with it, so the record of which stack a node
    // belonged to survives the removal rather than going with the stack.
    REQUIRE(NodeBuilderContext::backStampsEveryNode());
}