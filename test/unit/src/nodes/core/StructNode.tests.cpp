#include "nodes/core/StructNode.h"

#include "catch.hpp"

#include <string>
#include <vector>

using Values = StructNode::Values;

TEST_CASE("StructNode's constructor type is never reported") {
    // The constructor passes a vector up and the node overrides the method that reports its
    // type, so the argument is dead; the structure's type is what comes out.
    REQUIRE(std::string(StructNode::kConstructorType) == "vec3");
    REQUIRE_FALSE(StructNode::constructorTypeIsUsed());
    REQUIRE(StructNode::isStructNode);
    REQUIRE(std::string(StructNode::kTypeName) == "StructNode");
}

TEST_CASE("StructNode takes its values in one of three ways") {
    // Separate arguments are matched to the member names by position.
    REQUIRE(StructNode::valuesKind(2, true) == Values::ZippedByName);
    // A single object is taken as it stands.
    REQUIRE(StructNode::valuesKind(1, false) == Values::TakenAsGiven);
    // Nothing at all gives nothing, which is not the same as an empty object.
    REQUIRE(StructNode::valuesKind(0, false) == Values::None);
    REQUIRE(StructNode::valuesKind(0, true) == Values::None);
    // The two non empty kinds differ, which is the reason for the check.
    REQUIRE(StructNode::valuesKind(2, true) != StructNode::valuesKind(1, false));
}

TEST_CASE("StructNode matches a positional argument by its position") {
    // The names come from the layout's order, so the first argument fills the first member
    // whatever it was meant to be.
    const std::vector<std::string> order{"min", "max"};
    REQUIRE(StructNode::memberForPosition(order, 0) == "min");
    REQUIRE(StructNode::memberForPosition(order, 1) == "max");
    // A position past the end gives nothing rather than repeating the last name.
    REQUIRE(StructNode::memberForPosition(order, 2).empty());
    REQUIRE(StructNode::memberForPosition(order, -1).empty());
    // A different order gives different answers for the same positions.
    const std::vector<std::string> reversed{"max", "min"};
    REQUIRE(StructNode::memberForPosition(reversed, 0) == "max");
    REQUIRE(StructNode::memberForPosition(reversed, 0) != StructNode::memberForPosition(order, 0));
}

TEST_CASE("StructNode moves its structure type to the end of the children") {
    // The type is placed last whatever order it started in, because the code has to be
    // generated with the type after the parts that use it.
    REQUIRE(StructNode::movesStructTypeLast());
    REQUIRE_FALSE(StructNode::isCacheable());
}