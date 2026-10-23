#include "nodes/core/ModelNode.h"

#include "catch.hpp"

#include <string>

TEST_CASE("ModelNode has five scopes") {
    const auto& scopes = ModelNode::scopes();
    REQUIRE(scopes.size() == 5);
    // In the order the source lists them.
    REQUIRE(scopes[0] == "position");
    REQUIRE(scopes[1] == "viewPosition");
    REQUIRE(scopes[2] == "direction");
    REQUIRE(scopes[3] == "scale");
    REQUIRE(scopes[4] == "worldMatrix");
    REQUIRE(std::string(ModelNode::kTypeName) == "ModelNode");
}

TEST_CASE("ModelNode accepts only those scopes") {
    // Every name on the list is accepted.
    for (const auto& scope : ModelNode::scopes()) REQUIRE(ModelNode::isScope(scope));
    // Anything else is not, and the comparison is exact.
    REQUIRE_FALSE(ModelNode::isScope("Position"));
    REQUIRE_FALSE(ModelNode::isScope("worldmatrix"));
    REQUIRE_FALSE(ModelNode::isScope(""));
    REQUIRE_FALSE(ModelNode::isScope("position "));
    // The names are camel case where the source has them so, and two of the five are.
    REQUIRE(ModelNode::isScope("viewPosition"));
    REQUIRE(ModelNode::isScope("worldMatrix"));
    REQUIRE_FALSE(ModelNode::isScope("viewposition"));
}

TEST_CASE("ModelNode takes its object from the frame") {
    // The node does not hold an object of its own, which is what separates it from the node it
    // specialises; a test cannot show the frame being read, but it can record the decision.
    REQUIRE_FALSE(ModelNode::holdsOwnObject);
}