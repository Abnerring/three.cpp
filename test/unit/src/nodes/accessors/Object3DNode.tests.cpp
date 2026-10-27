#include "nodes/accessors/Object3DNode.h"
#include "catch.hpp"

TEST_CASE("Object3DNode compatibility helpers cover the six actual scopes") {
    const std::vector<std::string> expected{"position","viewPosition","direction","scale","worldMatrix","radius"};
    REQUIRE(Object3DNode::scopes()==expected);
    for(const auto* scope:{"position","viewPosition","direction","scale"})REQUIRE(Object3DNode::nodeType(scope)=="vec3");
    REQUIRE(Object3DNode::nodeType("worldMatrix")=="mat4");
    REQUIRE(Object3DNode::nodeType("radius")=="float");
    REQUIRE(Object3DNode::nodeType("notAscope").empty());
    REQUIRE(Object3DNode::nodeType("").empty());
    REQUIRE(Object3DNode::nodeType("WorldMatrix").empty());
    REQUIRE_FALSE(Object3DNode::isCacheable());
}
