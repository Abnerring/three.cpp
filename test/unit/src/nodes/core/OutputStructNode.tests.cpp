#include "nodes/core/OutputStructNode.h"

#include "catch.hpp"

#include <string>

TEST_CASE("OutputStructNode reports one fixed type") {
    // Whatever the members are, the answer is the same word; the shape is carried by the
    // structure the builder makes, not by this type.
    REQUIRE(std::string(OutputStructNode::kNodeType) == "OutputType");
    REQUIRE(std::string(OutputStructNode::kNodeType) != "vec4");
    REQUIRE(std::string(OutputStructNode::kNodeType) != "float");
    REQUIRE(OutputStructNode::isOutputStructNode);
    REQUIRE(std::string(OutputStructNode::kTypeName) == "OutputStructNode");
}

TEST_CASE("OutputStructNode names its members by position") {
    // The names are positional, so the first is m nought whatever its value's type is.
    REQUIRE(OutputStructNode::memberName(0) == "m0");
    REQUIRE(OutputStructNode::memberName(1) == "m1");
    REQUIRE(OutputStructNode::memberName(10) == "m10");
    // The name therefore says nothing about the value, which a test records by the name being
    // the same for two different indices' kinds of value.
    REQUIRE(OutputStructNode::memberName(0) != "colour");
    REQUIRE(OutputStructNode::memberName(2) == "m2");
}

TEST_CASE("OutputStructNode's prefix is nothing at all for an empty name") {
    // A name that is there takes a dot after it.
    REQUIRE(OutputStructNode::structPrefix("out") == "out.");
    // An empty one gives nothing rather than a bare dot, so no line begins with one.
    REQUIRE(OutputStructNode::structPrefix("") == "");
    REQUIRE(OutputStructNode::structPrefix("").empty());
    REQUIRE(OutputStructNode::structPrefix("") != ".");
    // The two differ, which is the point of the check.
    REQUIRE(OutputStructNode::structPrefix("out") != OutputStructNode::structPrefix(""));
}

TEST_CASE("OutputStructNode writes a line per member") {
    // With a name the line is qualified.
    REQUIRE(OutputStructNode::memberLine("out", 0, "colour") == "out.m0 = colour");
    REQUIRE(OutputStructNode::memberLine("out", 1, "size") == "out.m1 = size");
    // Without one it is the bare name, with no leading dot.
    REQUIRE(OutputStructNode::memberLine("", 0, "colour") == "m0 = colour");
    REQUIRE(OutputStructNode::memberLine("", 0, "colour") != ".m0 = colour");
    // The two forms differ only in the prefix.
    REQUIRE(OutputStructNode::memberLine("", 2, "x") == "m2 = x");
}

TEST_CASE("OutputStructNode answers with its property name") {
    // The generated answer is the name rather than a constructor of the members.
    REQUIRE(OutputStructNode::generateAnswer("out") == "out");
    REQUIRE(OutputStructNode::generateAnswer("") == "");
    REQUIRE(OutputStructNode::generateAnswer("out") != "OutputType(out.m0)");
    // And the layout is worked out once and kept.
    REQUIRE(OutputStructNode::layoutIsCached());
    REQUIRE_FALSE(OutputStructNode::isCacheable());
}