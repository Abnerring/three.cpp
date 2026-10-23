#include "nodes/core/OverrideContextNode.h"

#include "catch.hpp"

#include <string>

using Stored = OverrideContextNode::Stored;

TEST_CASE("OverrideContextNode stores an override in one of three ways") {
    // A function is kept as it is.
    REQUIRE(OverrideContextNode::storedKind(true, true) == Stored::Function);
    // A node, which is a value that is not a function, is wrapped in one that always answers it.
    REQUIRE(OverrideContextNode::storedKind(true, false) == Stored::Wrapper);
    // Nothing at all stays nothing.
    REQUIRE(OverrideContextNode::storedKind(false, false) == Stored::Nothing);
    REQUIRE(OverrideContextNode::storedKind(false, true) == Stored::Nothing);
    // The wrapper case is not the same as the nothing case, which a test that only checked two
    // of the three would miss.
    REQUIRE(OverrideContextNode::storedKind(true, false) != OverrideContextNode::storedKind(false, false));
    REQUIRE(OverrideContextNode::isOverrideContextNode);
}

TEST_CASE("OverrideContextNode's two factories ask different questions") {
    // The single one only asks whether the callback is a node, so a function is taken as it is
    // without any question about whether there is anything.
    REQUIRE_FALSE(OverrideContextNode::singleAsksWhetherThereIsAnything());
    // The multiple one asks three questions in a row, the first being whether there is anything.
    REQUIRE(OverrideContextNode::multipleAsksWhetherThereIsAnything());
    REQUIRE(OverrideContextNode::singleAsksWhetherThereIsAnything() !=
            OverrideContextNode::multipleAsksWhetherThereIsAnything());
    REQUIRE(std::string(OverrideContextNode::kTypeName) == "OverrideContextNode");
}

TEST_CASE("OverrideContextNode's chained form reverses two of its arguments") {
    // The free function takes the target first and the flow node last.
    const auto free = OverrideContextNode::freeFunctionArgumentOrder();
    REQUIRE(free.size() == 3);
    REQUIRE(free[0] == "targetNode");
    REQUIRE(free[2] == "flowNode");
    // The chained form takes the flow node first, and passes it through in last place.
    const auto chained = OverrideContextNode::chainedArgumentOrder();
    REQUIRE(chained.size() == 3);
    REQUIRE(chained[0] == "flowNode");
    REQUIRE(chained[2] == "callback");
    // So the first of one is the last of the other's pass through, which is the reversal.
    REQUIRE(free[0] != chained[0]);
    REQUIRE(free[2] != chained[2]);
}

TEST_CASE("OverrideContextNode's merge keeps the later of two entries") {
    // A key that appears twice takes its later value, because the map is built from every entry
    // in turn; the earlier one is overwritten.
    REQUIRE(OverrideContextNode::mergedValue({1, 2}) == 2);
    REQUIRE(OverrideContextNode::mergedValue({1, 2}) != 1);
    // A single entry is itself, and no entries gives nothing.
    REQUIRE(OverrideContextNode::mergedValue({7}) == 7);
    REQUIRE(OverrideContextNode::mergedValue({}) == 0);
    // Every ancestor is gathered rather than only the nearest.
    REQUIRE(OverrideContextNode::gathersEveryAncestor());
}