#include "nodes/core/ConstNode.h"

#include "catch.hpp"

TEST_CASE("ConstNode recognises the numeric type names") {
    // The three the pattern names.
    REQUIRE(ConstNode::isNumericType("float"));
    REQUIRE(ConstNode::isNumericType("int"));
    REQUIRE(ConstNode::isNumericType("uint"));
    // The pattern matches anywhere in the string rather than the whole of it, so a name that
    // merely holds one of them counts as well, which is wider than comparing names.
    REQUIRE(ConstNode::isNumericType("float32"));
    REQUIRE(ConstNode::isNumericType("myint"));
    // A name holding none of them does not count.
    REQUIRE_FALSE(ConstNode::isNumericType("vec3"));
    REQUIRE_FALSE(ConstNode::isNumericType("bool"));
    REQUIRE_FALSE(ConstNode::isNumericType(""));
}

TEST_CASE("ConstNode writes directly only when both sides are numeric") {
    REQUIRE(ConstNode::writesDirectly("float", "int"));
    REQUIRE(ConstNode::writesDirectly("uint", "float"));
    // A numeric constant going to a vector is converted rather than written straight in.
    REQUIRE_FALSE(ConstNode::writesDirectly("float", "vec3"));
    REQUIRE_FALSE(ConstNode::writesDirectly("vec3", "float"));
    REQUIRE_FALSE(ConstNode::writesDirectly("bool", "bool"));
}

TEST_CASE("ConstNode cuts an integer towards zero") {
    // Cutting, not rounding: one and a half becomes one and minus one and a half minus one.
    REQUIRE(ConstNode::numericValue("int", 1.5) == 1.0);
    REQUIRE(ConstNode::numericValue("int", -1.5) == -1.0);
    REQUIRE(ConstNode::numericValue("int", 2.9) == 2.0);
    REQUIRE(ConstNode::numericValue("int", -2.9) == -2.0);
    // A whole number is unchanged.
    REQUIRE(ConstNode::numericValue("int", 7.0) == 7.0);
}

TEST_CASE("ConstNode sends a negative unsigned integer to zero") {
    // The unsigned case differs from the signed one for every negative value: it becomes zero
    // before the cut, rather than being cut towards zero.
    REQUIRE(ConstNode::numericValue("uint", -1.5) == 0.0);
    REQUIRE(ConstNode::numericValue("int", -1.5) == -1.0);
    REQUIRE(ConstNode::numericValue("uint", -0.5) == 0.0);
    REQUIRE(ConstNode::numericValue("int", -0.5) == 0.0);
    // A positive one is cut the same way as a signed one.
    REQUIRE(ConstNode::numericValue("uint", 1.5) == 1.0);
    REQUIRE(ConstNode::numericValue("uint", 3.7) == 3.0);
}

TEST_CASE("ConstNode leaves other types alone") {
    // Only the two named types are handled; anything else keeps its value.
    REQUIRE(ConstNode::numericValue("float", 1.5) == 1.5);
    REQUIRE(ConstNode::numericValue("vec3", -1.5) == -1.5);
}

TEST_CASE("ConstNode is weak only when a document says exactly so") {
    // Nothing said leaves it fixed, and anything other than exactly true leaves it fixed.
    REQUIRE_FALSE(ConstNode::isWeak(false, false));
    REQUIRE_FALSE(ConstNode::isWeak(false, true));
    REQUIRE_FALSE(ConstNode::isWeak(true, false));
    // Only the one combination makes it weak.
    REQUIRE(ConstNode::isWeak(true, true));
}