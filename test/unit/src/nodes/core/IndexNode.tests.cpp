#include "nodes/core/IndexNode.h"

#include "catch.hpp"

#include <string>

TEST_CASE("IndexNode reports an unsigned integer") {
    // An index counts, so it is unsigned rather than one of the value types.
    REQUIRE(std::string(IndexNode::kNodeType) == "uint");
    REQUIRE(std::string(IndexNode::kNodeType) != "float");
    REQUIRE(std::string(IndexNode::kNodeType) != "int");
    REQUIRE(IndexNode::isIndexNode);
    REQUIRE(std::string(IndexNode::kTypeName) == "IndexNode");
}

TEST_CASE("IndexNode has six scopes and a call for each") {
    const auto scopes = IndexNode::scopes();
    REQUIRE(scopes.size() == 6);
    REQUIRE(scopes[0] == "vertex");
    REQUIRE(scopes[5] == "draw");
    // Each goes through its own builder call, so no two of them answer alike.
    REQUIRE(IndexNode::builderCall("vertex") == "getVertexIndex");
    REQUIRE(IndexNode::builderCall("instance") == "getInstanceIndex");
    REQUIRE(IndexNode::builderCall("draw") == "getDrawIndex");
    REQUIRE(IndexNode::builderCall("invocationLocal") == "getInvocationLocalIndex");
    REQUIRE(IndexNode::builderCall("invocationSubgroup") == "getInvocationSubgroupIndex");
    REQUIRE(IndexNode::builderCall("subgroup") == "getSubgroupIndex");
    REQUIRE(IndexNode::builderCall("vertex") != IndexNode::builderCall("instance"));
}

TEST_CASE("IndexNode throws for a scope it does not know") {
    // Unlike every other place in this layer taken so far, an unknown scope is not answered with
    // a default and a warning: the source throws.
    REQUIRE_FALSE(IndexNode::isKnown("tangent"));
    REQUIRE_FALSE(IndexNode::isKnown(""));
    REQUIRE(IndexNode::builderCall("tangent").empty());
    // The sentence carries the scope, so a reader can tell which one it was.
    REQUIRE(IndexNode::unknownScopeMessage("tangent") == "THREE.IndexNode: Unknown scope: tangent");
    REQUIRE(IndexNode::unknownScopeMessage("a") != IndexNode::unknownScopeMessage("b"));
    // Every scope it does know is known, which the loop shows rather than a remark.
    for (const auto& scope : IndexNode::scopes()) REQUIRE(IndexNode::isKnown(scope));
}

TEST_CASE("IndexNode reads directly in two stages, not one") {
    // The vertex stage reads the index itself, and so does the compute stage.
    REQUIRE(IndexNode::readsDirectly("vertex"));
    REQUIRE(IndexNode::readsDirectly("compute"));
    // Every other stage takes it through a varying, which the name of the first stage would not
    // lead a reader to expect of the second.
    REQUIRE_FALSE(IndexNode::readsDirectly("fragment"));
    REQUIRE_FALSE(IndexNode::readsDirectly(""));
    REQUIRE(IndexNode::readsDirectly("compute") != IndexNode::readsDirectly("fragment"));
}