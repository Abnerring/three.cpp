#include "nodes/core/NodeBuilderAssembly.h"

#include "catch.hpp"

#include <string>

using Filled = NodeBuilderAssembly::Filled;

TEST_CASE("NodeBuilder fills a list place from a value or a constant") {
    // A value that is there is built.
    REQUIRE(NodeBuilderAssembly::arrayPlace(true, true) == Filled::Built);
    // A missing one is a constant of the list's type, so a partial list is completed with defaults
    // rather than left with gaps.
    REQUIRE(NodeBuilderAssembly::arrayPlace(true, false) == Filled::Constant);
    // And a missing list makes every place a constant.
    REQUIRE(NodeBuilderAssembly::arrayPlace(false, true) == Filled::Constant);
    REQUIRE(NodeBuilderAssembly::arrayPlace(false, false) == Filled::Constant);
}

TEST_CASE("NodeBuilder asks three questions of a structure place") {
    // Only all three passing means the value is built.
    REQUIRE(NodeBuilderAssembly::structPlace(true, true, true) == Filled::Built);
    // Without values at all, every place is a constant.
    REQUIRE(NodeBuilderAssembly::structPlace(false, true, true) == Filled::Constant);
    // Without a value for this member, likewise.
    REQUIRE(NodeBuilderAssembly::structPlace(true, false, true) == Filled::Constant);
    // And a member given something that is not a node becomes the default rather than failing.
    REQUIRE(NodeBuilderAssembly::structPlace(true, true, false) == Filled::Constant);
    REQUIRE(NodeBuilderAssembly::structPlace(true, true, false) !=
            NodeBuilderAssembly::structPlace(true, true, true));
}

TEST_CASE("NodeBuilder's two steps do not ask the same question") {
    // The list asks only whether a value is there, the structure also whether it is a node, so the
    // same value would be built into a list and replaced by a constant in a structure.
    REQUIRE_FALSE(NodeBuilderAssembly::bothAskTheSame());
    REQUIRE(NodeBuilderAssembly::arrayPlace(true, true) == Filled::Built);
    REQUIRE(NodeBuilderAssembly::structPlace(true, true, false) == Filled::Constant);
}

TEST_CASE("NodeBuilder separates places after every one but the last") {
    // One place has no separator, three have two.
    REQUIRE(NodeBuilderAssembly::joined(0).empty());
    REQUIRE(NodeBuilderAssembly::joined(1) == "x");
    REQUIRE(NodeBuilderAssembly::joined(2) == "x, x");
    REQUIRE(NodeBuilderAssembly::joined(3) == "x, x, x");
    // A member's own type shadows the structure's inside the loop, and both are used.
    REQUIRE(NodeBuilderAssembly::memberTypeShadowsStructureType());
}