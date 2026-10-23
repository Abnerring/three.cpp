#include "nodes/core/ContextNode.h"

#include "catch.hpp"

#include <map>
#include <string>

TEST_CASE("ContextNode passes scope and type straight through") {
    // The wrapper has no scope or type of its own, so all three questions are answered by what it
    // wraps. That is what makes it invisible.
    REQUIRE(ContextNode::scope("main") == "main");
    REQUIRE(ContextNode::nodeType("vec3") == "vec3");
    REQUIRE(ContextNode::memberType("float") == "float");
    // Two different answers pass through differently, so nothing is fixed.
    REQUIRE(ContextNode::nodeType("vec3") != ContextNode::nodeType("float"));
    REQUIRE(ContextNode::isContextNode);
    REQUIRE(std::string(ContextNode::kTypeName) == "ContextNode");
}

TEST_CASE("ContextNode gathers several values with the later winning") {
    // A key that appears twice takes the value from the later context node, since the objects are
    // merged in the order they were walked.
    const auto merged = ContextNode::gather({{{"a", 1}}, {{"a", 2}}});
    REQUIRE(merged.at("a") == 2);
    REQUIRE(merged.at("a") != 1);
    // Keys that do not collide are all present, so merging is not a replacement.
    const auto both = ContextNode::gather({{{"a", 1}}, {{"b", 2}}});
    REQUIRE(both.size() == 2);
    REQUIRE(both.at("a") == 1);
    REQUIRE(both.at("b") == 2);
    // Nothing at all gathers to nothing.
    REQUIRE(ContextNode::gather({}).empty());
}

TEST_CASE("ContextNode applies its context only on the first use") {
    // The count is taken after incrementing, so one means this was the first use.
    REQUIRE(ContextNode::appliesContext(1));
    REQUIRE_FALSE(ContextNode::appliesContext(2));
    REQUIRE_FALSE(ContextNode::appliesContext(0));
    REQUIRE(ContextNode::appliesContext(1) != ContextNode::appliesContext(2));
    // And the previous context is not put back on a throw, the restore being a plain statement
    // after the call.
    REQUIRE_FALSE(ContextNode::restoresOnThrow());
    REQUIRE_FALSE(ContextNode::isCacheable());
}