#include "nodes/core/ReferenceElementNode.h"

#include "catch.hpp"

#include <string>

TEST_CASE("ReferenceElementNode names itself") {
    REQUIRE(std::string(ReferenceElementNode::kTypeName) == "ReferenceElementNode");
    REQUIRE(ReferenceElementNode::isReferenceElementNode);
}

TEST_CASE("ReferenceElementNode reports the uniform type") {
    // The type comes from the reference node's uniform rather than from the builder, so the
    // node simply reports what it was given.
    const ReferenceElementNode element("vec4");
    REQUIRE(element.generateNodeType() == "vec4");
    // A different uniform gives a different type, and nothing about a builder enters.
    const ReferenceElementNode other("float");
    REQUIRE(other.generateNodeType() == "float");
    REQUIRE(other.generateNodeType() != element.generateNodeType());
}

TEST_CASE("ReferenceElementNode formats only when the types differ") {
    // The array type and the element type are the same when the reference holds plain values.
    REQUIRE_FALSE(ReferenceElementNode::needsFormatting("float", "float"));
    // An array of four parts gives elements of one, so the two differ and the snippet is
    // formatted from one to the other.
    REQUIRE(ReferenceElementNode::needsFormatting("vec4", "float"));
    // The other way round is also a difference, so the test is about equality and not order.
    REQUIRE(ReferenceElementNode::needsFormatting("float", "vec4"));
    // Two different names that both exist differ too.
    REQUIRE(ReferenceElementNode::needsFormatting("vec3", "vec4"));
}