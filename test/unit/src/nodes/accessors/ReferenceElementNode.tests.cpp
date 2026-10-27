#include "nodes/accessors/ReferenceElementNode.h"
#include "catch.hpp"
#include <string>

TEST_CASE("ReferenceElementNode names itself") {
    REQUIRE(std::string(ReferenceElementNode::kTypeName) == "ReferenceElementNode");
    REQUIRE(ReferenceElementNode::isReferenceElementNode);
}

TEST_CASE("ReferenceElementNode reports the uniform type") {
    auto reference=JSValue::object();reference.set("uniformType","vec4");
    const ReferenceElementNode element(reference,nullptr);
    REQUIRE(element.generateNodeType().string() == "vec4");
    auto otherReference=JSValue::object();otherReference.set("uniformType","float");
    const ReferenceElementNode other(otherReference,nullptr);
    REQUIRE(other.generateNodeType().string() == "float");
    REQUIRE_FALSE(other.generateNodeType().sameValue(element.generateNodeType()));
}

TEST_CASE("ReferenceElementNode formats only when the types differ") {
    REQUIRE_FALSE(ReferenceElementNode::needsFormatting("float", "float"));
    REQUIRE(ReferenceElementNode::needsFormatting("vec4", "float"));
    REQUIRE(ReferenceElementNode::needsFormatting("float", "vec4"));
    REQUIRE(ReferenceElementNode::needsFormatting("vec3", "vec4"));
}
