#include "nodes/core/NodeShared.h"

#include "catch.hpp"

using Outcome = NodeShared::Outcome;

TEST_CASE("Node shares only with a different node under its hash") {
    // A hash that leads to another node is a share.
    REQUIRE(NodeShared::outcome(true, false, false) == Outcome::SharedByHash);
    // A hash that leads back to the node asking is not, so the asking node falls through to the
    // override check rather than answering with itself through the first branch.
    REQUIRE(NodeShared::outcome(true, true, false) == Outcome::Itself);
    REQUIRE(NodeShared::outcome(true, true, true) == Outcome::Overridden);
    REQUIRE(NodeShared::outcome(true, false, false) != NodeShared::outcome(true, true, false));
}

TEST_CASE("Node considers its override only when the hash found nothing") {
    // With nothing under the hash the override applies.
    REQUIRE(NodeShared::outcome(false, false, true) == Outcome::Overridden);
    // With nothing at all it is the node itself.
    REQUIRE(NodeShared::outcome(false, false, false) == Outcome::Itself);
    // And a hash match skips the override entirely, so which of the two applies depends on a
    // lookup rather than on anything the caller said.
    REQUIRE_FALSE(NodeShared::overrideIsConsideredAfterHashMatch());
    REQUIRE(NodeShared::outcome(true, false, true) == Outcome::SharedByHash);
}

TEST_CASE("Node marks an override before running it") {
    // The first time the mark is clear, so the callback runs and the mark is set.
    const auto first = NodeShared::overrideState(false);
    REQUIRE_FALSE(first.isOverwritten);
    REQUIRE(first.runsCallback);
    // A re-entry finds the mark set and is answered from the stored result instead, so the
    // callback cannot call itself for ever.
    const auto again = NodeShared::overrideState(true);
    REQUIRE(again.isOverwritten);
    REQUIRE_FALSE(again.runsCallback);
    REQUIRE(first.runsCallback != again.runsCallback);
    // The callback's answer has the node's own override removed, so a node used inside it is not
    // replaced by the override that is building it.
    REQUIRE(NodeShared::cancelsOwnOverrideDuringCallback());
}

TEST_CASE("Node answers with itself when nothing was shared") {
    // No share and no override means the node asking.
    REQUIRE(NodeShared::fallsBackToItself(false));
    REQUIRE_FALSE(NodeShared::fallsBackToItself(true));
    // And a cache key is made from the children and from the node's own key together, so a node
    // may make itself distinct without changing what it holds.
    REQUIRE(NodeShared::keyUsesBothChildrenAndCustomKey());
}