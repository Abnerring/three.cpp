#include "nodes/accessors/ModelNode.h"
#include "catch.hpp"

TEST_CASE("ModelNode compatibility helpers cover the six inherited scopes") {
    const std::vector<std::string> expected{"position","viewPosition","direction","scale","worldMatrix","radius"};
    REQUIRE(ModelNode::scopes()==expected);
    for(const auto& scope:ModelNode::scopes())REQUIRE(ModelNode::isScope(scope));
    for(const auto* scope:{"Position","worldmatrix","","position ","viewposition"})REQUIRE_FALSE(ModelNode::isScope(scope));
    REQUIRE_FALSE(ModelNode::holdsOwnObject);
}
