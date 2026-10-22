#include "nodes/core/Node.h"

#include "catch.hpp"

TEST_CASE("Node assigns increasing ids and the upstream default state") {
    Node first;
    Node second;
    REQUIRE(Node::isNode);
    REQUIRE(second.id() == first.id() + 1);
    REQUIRE(first.version() == 0);
    REQUIRE(first.name().empty());
    REQUIRE_FALSE(first.global());
    REQUIRE_FALSE(first.parents());
    REQUIRE_FALSE(first.nodeType().has_value());

    Node typed(std::string("vec3"));
    REQUIRE(typed.nodeType().has_value());
    REQUIRE(typed.nodeType().value() == "vec3");
}

TEST_CASE("Node accessors match the upstream defaults") {
    Node node;
    REQUIRE(node.getUpdateType() == NodeUpdateType::NONE);
    REQUIRE(node.getUpdateBeforeType() == NodeUpdateType::NONE);
    REQUIRE(node.getUpdateAfterType() == NodeUpdateType::NONE);
    REQUIRE(node.isCacheable());
    // customCacheKey is this.id, getHash is String( this.id ), getScope is this.
    REQUIRE(node.customCacheKey() == static_cast<double>(node.id()));
    REQUIRE(node.getHash() == std::to_string(node.id()));
    REQUIRE(node.getScope() == &node);
}

TEST_CASE("Node derived classes can override the accessors") {
    struct Custom : Node {
        explicit Custom(double key) : key(key) {}
        bool isCacheable() const override { return false; }
        double customCacheKey() const override { return key; }
        double key;
    };
    Custom custom(42.0);
    REQUIRE_FALSE(custom.isCacheable());
    REQUIRE(custom.customCacheKey() == 42.0);
    // Unoverridden defaults still apply.
    REQUIRE(custom.getHash() == std::to_string(custom.id()));
}
TEST_CASE("Node reports its global flag, array count and member type") {
    Node node;
    REQUIRE_FALSE(node.isGlobal());
    // The base class has no array length, which upstream expresses as null.
    REQUIRE_FALSE(node.getArrayCount().has_value());
    // And it has no members, so member access reports void.
    REQUIRE(node.getMemberType("any") == "void");

    struct GlobalNode : Node {
        GlobalNode() { _global = true; }
        std::optional<double> getArrayCount() const override { return 4.0; }
    };
    GlobalNode global;
    REQUIRE(global.isGlobal());
    REQUIRE(global.getArrayCount().has_value());
    REQUIRE(global.getArrayCount().value() == 4.0);
}