#include "nodes/core/Object3DNode.h"

#include "catch.hpp"

#include <string>

TEST_CASE("Object3DNode has five scopes") {
    const auto& scopes = Object3DNode::scopes();
    REQUIRE(scopes.size() == 5);
    REQUIRE(scopes[0] == "position");
    REQUIRE(scopes[1] == "viewPosition");
    REQUIRE(scopes[2] == "direction");
    REQUIRE(scopes[3] == "scale");
    REQUIRE(scopes[4] == "worldMatrix");
    REQUIRE(std::string(Object3DNode::kTypeName) == "Object3DNode");
}

TEST_CASE("Object3DNode reports a vector for four scopes and a matrix for one") {
    // Four of the five are three part vectors.
    REQUIRE(Object3DNode::nodeType("position") == "vec3");
    REQUIRE(Object3DNode::nodeType("viewPosition") == "vec3");
    REQUIRE(Object3DNode::nodeType("direction") == "vec3");
    REQUIRE(Object3DNode::nodeType("scale") == "vec3");
    // Only the world matrix is a four by four.
    REQUIRE(Object3DNode::nodeType("worldMatrix") == "mat4");
    // Counting them shows it is one of five, which is the point.
    int matrices = 0;
    for (const auto& scope : Object3DNode::scopes()) {
        if (Object3DNode::nodeType(scope) == "mat4") matrices += 1;
    }
    REQUIRE(matrices == 1);
}

TEST_CASE("Object3DNode answers with a vector for a scope it does not know") {
    // The source falls through to the same place rather than refusing, so an unknown name gets
    // a vector. That is a property of the branch rather than a rule about names.
    REQUIRE(Object3DNode::nodeType("notAscope") == "vec3");
    REQUIRE(Object3DNode::nodeType("") == "vec3");
    // Case matters, so a capitalised matrix name does not get the matrix.
    REQUIRE(Object3DNode::nodeType("WorldMatrix") == "vec3");
}

TEST_CASE("Object3DNode refuses to cache and updates per object") {
    REQUIRE_FALSE(Object3DNode::isCacheable());
    REQUIRE(std::string(Object3DNode::kUpdateType) == "OBJECT");
    // The update kind is not one of the two more common ones, which is what makes the node
    // different from most.
    REQUIRE(std::string(Object3DNode::kUpdateType) != "FRAME");
    REQUIRE(std::string(Object3DNode::kUpdateType) != "RENDER");
}