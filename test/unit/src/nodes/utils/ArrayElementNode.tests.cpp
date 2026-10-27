#include "nodes/utils/ArrayElementNode.h"

#include "catch.hpp"

#include <string>

TEST_CASE("ArrayElementNode keeps an integer index as it is") {
    // An integer that is not a vector is written with its own type.
    REQUIRE(ArrayElementNode::indexType(false, true, "int") == "int");
    REQUIRE(ArrayElementNode::indexType(false, true, "uint") == "uint");
    REQUIRE(ArrayElementNode::indexKeepsType(false, true));
    REQUIRE(std::string(ArrayElementNode::kTypeName) == "ArrayElementNode");
}

TEST_CASE("ArrayElementNode quietly turns a float index into an unsigned integer") {
    // A float is not an integer, so it becomes an unsigned integer without any message; the
    // truncation happens in the shader rather than here.
    REQUIRE(ArrayElementNode::indexType(false, false, "float") == "uint");
    REQUIRE(ArrayElementNode::indexType(false, false, "float") != "float");
    REQUIRE_FALSE(ArrayElementNode::indexKeepsType(false, false));
    // A vector is turned into the same thing, even though its parts are integers.
    REQUIRE(ArrayElementNode::indexType(true, true, "ivec2") == "uint");
    REQUIRE_FALSE(ArrayElementNode::indexKeepsType(true, true));
    // The two cases that lose the type differ in their reason but agree in their answer.
    REQUIRE(ArrayElementNode::indexType(false, false, "float") ==
            ArrayElementNode::indexType(true, true, "ivec2"));
}

TEST_CASE("ArrayElementNode's own type is the element's") {
    // The node answers with the type of what it reads, not of the array it reads from.
    REQUIRE(ArrayElementNode::nodeTypeFrom("float") == "float");
    REQUIRE(ArrayElementNode::nodeTypeFrom("vec3") == "vec3");
    REQUIRE(ArrayElementNode::nodeTypeFrom("vec3") != "vec3[]");
}

TEST_CASE("ArrayElementNode looks a member up on the array") {
    // A member read from an element is forwarded to the array-like node rather than to the
    // element, so a caller expecting the element's members would be looking in the wrong place.
    REQUIRE(ArrayElementNode::memberLookupTarget() == ArrayElementNode::MemberLookup::Array);
    REQUIRE(ArrayElementNode::memberLookupTarget() != ArrayElementNode::MemberLookup::Element);
    REQUIRE_FALSE(ArrayElementNode::isCacheable());
}