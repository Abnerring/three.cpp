#include "nodes/core/AttributeNode.h"

#include "catch.hpp"

#include <string>

using Form = AttributeNode::Form;

TEST_CASE("AttributeNode's type comes from the attribute or falls back to a float") {
    // A type given when the node was made wins.
    REQUIRE(AttributeNode::nodeType(true, "vec3", true, "vec2") == "vec3");
    // Otherwise the attribute's own type is used.
    REQUIRE(AttributeNode::nodeType(false, "vec3", true, "vec2") == "vec2");
    // An attribute the geometry does not have becomes a float, and a warning is printed, so the
    // shader still compiles and gives nought there.
    REQUIRE(AttributeNode::nodeType(false, "vec3", false, "vec2") == "float");
    REQUIRE(AttributeNode::nodeType(false, "vec3", false, "vec2") != "vec2");
    // That default is the same one the member node and the parameter node use.
    REQUIRE(std::string(AttributeNode::kMissingAttributeType) == "float");
    REQUIRE(std::string(AttributeNode::kTypeName) == "AttributeNode");
}

TEST_CASE("AttributeNode's warning names the attribute") {
    REQUIRE(AttributeNode::missingAttributeWarning("position") ==
            "AttributeNode: Vertex attribute \"position\" not found on geometry.");
    // The name really is part of it.
    REQUIRE(AttributeNode::missingAttributeWarning("position") !=
            AttributeNode::missingAttributeWarning("normal"));
}

TEST_CASE("AttributeNode is wrapped in a varying outside the vertex stage") {
    // The vertex stage writes the attribute itself, since that is where attributes are read.
    REQUIRE(AttributeNode::formFor("vertex") == Form::Attribute);
    // Every other stage gets a varying, so the same node produces code of two different shapes
    // depending on where it is used.
    REQUIRE(AttributeNode::formFor("fragment") == Form::Varying);
    REQUIRE(AttributeNode::formFor("compute") == Form::Varying);
    REQUIRE(AttributeNode::formFor("") == Form::Varying);
    // The two forms differ, which is the whole consequence of the stage.
    REQUIRE(AttributeNode::formFor("vertex") != AttributeNode::formFor("fragment"));
}

TEST_CASE("AttributeNode hashes its name rather than an identifier") {
    // Two nodes naming the same attribute hash alike, which is the opposite of what the
    // parameter node does with its identifier.
    REQUIRE(AttributeNode::hashIsTheName());
    REQUIRE(AttributeNode::globalByDefault);
    REQUIRE_FALSE(AttributeNode::isCacheable());
}