#include "nodes/core/AssignNode.h"

#include "catch.hpp"

#include <string>

TEST_CASE("AssignNode's type follows what the caller asked for") {
    // A real output gets the target's type.
    REQUIRE(AssignNode::nodeType("vec3", "vec3") == "vec3");
    REQUIRE(AssignNode::nodeType("float", "vec3") == "vec3");
    // An output of nothing gets nothing, so the same node answers two different types depending
    // on what was asked of it.
    REQUIRE(AssignNode::nodeType("void", "vec3") == "void");
    REQUIRE(AssignNode::nodeType("void", "vec3") != AssignNode::nodeType("vec3", "vec3"));
    REQUIRE(AssignNode::isAssignNode);
    REQUIRE(std::string(AssignNode::kTypeName) == "AssignNode");
}

TEST_CASE("AssignNode writes its assignment once") {
    // The record is set at the end of a generate, so a second one finds it and answers with the
    // target rather than writing the assignment again.
    REQUIRE_FALSE(AssignNode::alreadyAssigned(false));
    REQUIRE(AssignNode::alreadyAssigned(true));
    REQUIRE(AssignNode::alreadyAssigned(true) != AssignNode::alreadyAssigned(false));
    // The value of an assignment is its target.
    REQUIRE(AssignNode::valueIsTheTarget);
}

TEST_CASE("AssignNode splits only for a non identity swizzle") {
    // With the backend able to assign through a swizzle, nothing needs splitting.
    REQUIRE_FALSE(AssignNode::needsSplit(true, true, 2, 2, "yx"));
    // A target that is not a split needs none either.
    REQUIRE_FALSE(AssignNode::needsSplit(false, false, 2, 2, "yx"));
    // One component is not enough to need a split, since there is nothing to order.
    REQUIRE_FALSE(AssignNode::needsSplit(false, true, 1, 2, "x"));
    // The first two of a two part value are the value itself, so naming them needs no split.
    REQUIRE_FALSE(AssignNode::needsSplit(false, true, 2, 2, "xy"));
    // Naming them the other way round does.
    REQUIRE(AssignNode::needsSplit(false, true, 2, 2, "yx"));
    REQUIRE(AssignNode::needsSplit(false, true, 2, 2, "yx") !=
            AssignNode::needsSplit(false, true, 2, 2, "xy"));
    // And a pair out of a larger value that is not its first two also needs one.
    REQUIRE(AssignNode::needsSplit(false, true, 2, 4, "xz"));
}

TEST_CASE("AssignNode adds a line only for a void output or source") {
    // Either of the two makes the assignment a statement.
    REQUIRE(AssignNode::addsLineToFlow("void", "vec3"));
    REQUIRE(AssignNode::addsLineToFlow("vec3", "void"));
    REQUIRE(AssignNode::addsLineToFlow("void", "void"));
    // A real output from a real source is a value, so no line is added and the text is returned
    // as the expression instead.
    REQUIRE_FALSE(AssignNode::addsLineToFlow("vec3", "vec3"));
    REQUIRE(AssignNode::addsLineToFlow("void", "vec3") !=
            AssignNode::addsLineToFlow("vec3", "vec3"));
}