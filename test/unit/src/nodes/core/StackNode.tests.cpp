#include "nodes/core/StackNode.h"

#include "catch.hpp"

#include <string>

TEST_CASE("StackNode's three questions answer the empty type when it has no output") {
    // With an output all three pass its answer through.
    REQUIRE(StackNode::elementType(true, "vec3") == "vec3");
    REQUIRE(StackNode::nodeType(true, "vec3") == "vec3");
    REQUIRE(StackNode::memberType(true, "float") == "float");
    // Without one all three are the empty type rather than a refusal, so a stack that was never
    // given an output is void in every sense.
    REQUIRE(StackNode::elementType(false, "vec3") == "void");
    REQUIRE(StackNode::nodeType(false, "vec3") == "void");
    REQUIRE(StackNode::memberType(false, "float") == "void");
    REQUIRE(StackNode::nodeType(false, "vec3") != "vec3");
    REQUIRE(StackNode::isStackNode);
    REQUIRE(std::string(StackNode::kTypeName) == "StackNode");
}

TEST_CASE("StackNode discards its result in exactly one pairing") {
    // At the third stage with an empty type the result is thrown away.
    REQUIRE_FALSE(StackNode::keepsResult("generate", "void"));
    // At the third stage with any other type it is kept.
    REQUIRE(StackNode::keepsResult("generate", "vec3"));
    // And at any other stage it is kept whatever the type.
    REQUIRE(StackNode::keepsResult("setup", "void"));
    REQUIRE(StackNode::keepsResult("analyze", "void"));
    REQUIRE(StackNode::keepsResult("", "void"));
    // So the one pairing is not reachable by relaxing either half on its own.
    REQUIRE(StackNode::keepsResult("setup", "void") != StackNode::keepsResult("generate", "void"));
    REQUIRE(StackNode::keepsResult("generate", "vec3") != StackNode::keepsResult("generate", "void"));
}

TEST_CASE("StackNode builds itself when it has no output") {
    // Without an output there is nothing to build through, so the node falls back to itself.
    REQUIRE(StackNode::fallsBackToItself(false));
    REQUIRE_FALSE(StackNode::fallsBackToItself(true));
    // And the current stack is not put back on a throw, the restore being a plain statement at
    // the end.
    REQUIRE_FALSE(StackNode::restoresOnThrow());
    REQUIRE_FALSE(StackNode::isCacheable());
}