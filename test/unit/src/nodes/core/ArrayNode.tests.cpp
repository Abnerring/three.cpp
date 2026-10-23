#include "nodes/core/ArrayNode.h"

#include "catch.hpp"

#include <string>
#include <vector>

using Shape = ArrayNode::Shape;

TEST_CASE("ArrayNode's factory means two things by argument count") {
    // One argument is a list of values, whose length is the count and which has no type of its
    // own.
    const Shape one = ArrayNode::fromOneArgument({10, 11, 12});
    REQUIRE(one.count == 3);
    REQUIRE(one.hasValues);
    REQUIRE_FALSE(one.hasType);
    // Two arguments are a type and a count, with no values at all.
    const Shape two = ArrayNode::fromTwoArguments("vec3", 4);
    REQUIRE(two.count == 4);
    REQUIRE(two.hasType);
    REQUIRE(two.type == "vec3");
    REQUIRE_FALSE(two.hasValues);
    // The two shapes differ in both settings, so the arity decides everything about them.
    REQUIRE(one.hasType != two.hasType);
    REQUIRE(one.hasValues != two.hasValues);
    REQUIRE(ArrayNode::isArrayNode);
}

TEST_CASE("ArrayNode's type comes from the first value when none was given") {
    // A given type wins.
    REQUIRE(ArrayNode::nodeType(true, "vec3", false, "float") == "vec3");
    // Without one the first value's type is used, so a list of numbers reports a number's type
    // even though the node itself said nothing.
    REQUIRE(ArrayNode::nodeType(false, "vec3", true, "float") == "float");
    REQUIRE(ArrayNode::nodeType(false, "vec3", true, "float") != "vec3");
    // The two shapes answer whether they were given a type, which is what decides whether a
    // value has to be read at all.
    REQUIRE(ArrayNode::typeIsGiven(ArrayNode::fromTwoArguments("vec3", 4)));
    REQUIRE_FALSE(ArrayNode::typeIsGiven(ArrayNode::fromOneArgument({1, 2})));
    REQUIRE(std::string(ArrayNode::kTypeName) == "ArrayNode");
}

TEST_CASE("ArrayNode's filled list repeats one node") {
    // Every place holds the same node, so the list is one object repeated rather than distinct
    // entries; a caller expecting separate ones would find them all alike.
    const auto filled = ArrayNode::filled(4, 7);
    REQUIRE(filled.size() == 4);
    for (const auto id : filled) REQUIRE(id == 7);
    // Two different nodes fill differently, so the value does come from the argument.
    const auto other = ArrayNode::filled(4, 8);
    REQUIRE(other[0] != filled[0]);
    // A count of nought gives an empty list.
    REQUIRE(ArrayNode::filled(0, 7).empty());
    REQUIRE(ArrayNode::filled(0, 7).size() == 0);
}