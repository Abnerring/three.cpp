#include "loaders/VOXLoader.h"

#include "catch.hpp"

#include <vector>

namespace {
VOXLoader::Node makeNode(int id, std::vector<int> children = {}, std::vector<int> models = {}) {
    VOXLoader::Node node;
    node.transform.nodeId = id;
    node.children = std::move(children);
    node.models = std::move(models);
    return node;
}
}  // namespace

TEST_CASE("VOXLoader finds the single root of a tree") {
    // Zero holds one and two, and two holds three.
    std::vector<VOXLoader::Node> nodes{makeNode(0, {1, 2}), makeNode(1, {}, {7}), makeNode(2, {3}),
                                      makeNode(3, {}, {8})};
    const auto scene = VOXLoader::assembleScene(std::move(nodes));
    REQUIRE(scene.nodes.size() == 4);
    // Only zero is unclaimed.
    REQUIRE(scene.roots.size() == 1);
    REQUIRE(scene.roots[0] == 0);
    // The grandchildren survive.
    REQUIRE(scene.nodes[2].children.size() == 1);
    REQUIRE(scene.nodes[2].children[0] == 3);
    // A shape keeps its model reference.
    REQUIRE(scene.nodes[1].models[0] == 7);
}

TEST_CASE("VOXLoader reports every unclaimed node as a root") {
    // Two separate nodes with no parent.
    std::vector<VOXLoader::Node> nodes{makeNode(0), makeNode(1)};
    const auto scene = VOXLoader::assembleScene(std::move(nodes));
    REQUIRE(scene.roots.size() == 2);
    REQUIRE(scene.roots[0] == 0);
    REQUIRE(scene.roots[1] == 1);
}

TEST_CASE("VOXLoader drops a child reference to a missing node") {
    std::vector<VOXLoader::Node> nodes{makeNode(0, {9})};
    const auto scene = VOXLoader::assembleScene(std::move(nodes));
    REQUIRE(scene.nodes[0].children.empty());
    // Zero is still the root.
    REQUIRE(scene.roots.size() == 1);
}

TEST_CASE("VOXLoader drops a second claim on the same child") {
    // Both groups claim node two, so the second claim is dropped and one root remains.
    std::vector<VOXLoader::Node> nodes{makeNode(0, {2}), makeNode(1, {2}), makeNode(2)};
    const auto scene = VOXLoader::assembleScene(std::move(nodes));
    REQUIRE(scene.nodes[0].children.size() == 1);
    REQUIRE(scene.nodes[1].children.empty());
    // Zero and one are roots because one lost its only child.
    REQUIRE(scene.roots.size() == 2);
}