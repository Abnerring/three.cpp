#include "nodes/core/NodeAttribute.h"
#include "nodes/core/NodeParser.h"

#include "catch.hpp"

#include <string>

TEST_CASE("NodeParser's unimplemented method warns and returns nothing") {
    // It says the function is abstract and gives back nothing; it does not throw, so a parser
    // that was never written leaves its caller with an empty answer rather than an error.
    REQUIRE(NodeParser::abstractWarning() == "Abstract function.");
    REQUIRE(NodeParser::returnsNothing());
    REQUIRE_FALSE(NodeParser::throwsOnMissingImplementation());
    // The two answers are opposite, which is the point of having both.
    REQUIRE(NodeParser::returnsNothing() != NodeParser::throwsOnMissingImplementation());
}

TEST_CASE("NodeAttribute holds a name and a type") {
    const NodeAttribute attribute("position", "vec3");
    REQUIRE(attribute.name() == "position");
    REQUIRE(attribute.type() == "vec3");
    REQUIRE(NodeAttribute::isNodeAttribute);
    // A different attribute holds different values, so they are not fixed.
    const NodeAttribute other("uv", "vec2");
    REQUIRE(other.name() != attribute.name());
    REQUIRE(other.type() != attribute.type());
}

TEST_CASE("NodeAttribute's node is optional") {
    // Without one the attribute is still valid, which is why the reference is optional.
    const NodeAttribute bare("normal", "vec3");
    REQUIRE_FALSE(bare.hasNode());
    REQUIRE(bare.nodeId() == 0);
    // With one it remembers which.
    const NodeAttribute linked("tangent", "vec4", true, 42);
    REQUIRE(linked.hasNode());
    REQUIRE(linked.nodeId() == 42);
    // The two differ only in that reference, so the name and type are still there.
    REQUIRE(linked.name() == "tangent");
    REQUIRE(linked.type() == "vec4");
    REQUIRE(linked.hasNode() != bare.hasNode());
}