#include "nodes/core/UniformNode.h"

#include "catch.hpp"

#include <string>

TEST_CASE("UniformNode starts with no name and the per object group") {
    // The name is the empty string rather than nothing.
    REQUIRE(std::string(UniformNode::kDefaultName) == "");
    // The group is the per object one, so a uniform made without saying follows its object rather
    // than being shared between them.
    REQUIRE(std::string(UniformNode::kDefaultGroup) == "object");
    REQUIRE(std::string(UniformNode::kDefaultGroup) != "render");
    REQUIRE(std::string(UniformNode::kDefaultGroup) != "frame");
    REQUIRE(UniformNode::isUniformNode);
    REQUIRE(std::string(UniformNode::kTypeName) == "UniformNode");
}

TEST_CASE("UniformNode's hash is the ordinary node hash by default") {
    // A derived class may answer otherwise, but this one passes the ordinary hash through.
    REQUIRE(UniformNode::uniformHash("abc") == "abc");
    REQUIRE(UniformNode::uniformHash("") == "");
    // And the hash is what decides whether two uniforms are the same one.
    REQUIRE(UniformNode::hashesMergeNodes());
}

TEST_CASE("UniformNode's shared node is the first registered for its hash") {
    // Nothing registered yet: this node registers itself and becomes the shared one.
    const auto first = UniformNode::sharedNode(false, 0, 7);
    REQUIRE(first.wasRegistered);
    REQUIRE(first.nodeId == 7);
    // Something already registered: this node refers to that one instead of registering.
    const auto second = UniformNode::sharedNode(true, 7, 9);
    REQUIRE_FALSE(second.wasRegistered);
    REQUIRE(second.nodeId == 7);
    // So two nodes that hash alike end up as one, and the one they end up as is the earlier.
    REQUIRE(second.nodeId != 9);
    REQUIRE(second.nodeId == first.nodeId);
    REQUIRE(first.wasRegistered != second.wasRegistered);
}

TEST_CASE("UniformNode's deprecated setter names both the old and the new") {
    const std::string warning = UniformNode::deprecationWarning();
    REQUIRE(warning.find("label()") != std::string::npos);
    REQUIRE(warning.find("setName()") != std::string::npos);
    REQUIRE(warning.find("deprecated") != std::string::npos);
    REQUIRE(warning == "TSL: \"label()\" has been deprecated. Use \"setName()\" instead.");
}