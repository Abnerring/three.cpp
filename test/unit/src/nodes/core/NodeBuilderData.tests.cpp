#include "nodes/core/NodeBuilderData.h"

#include "catch.hpp"

#include <string>

using CacheChoice = NodeBuilderData::CacheChoice;

TEST_CASE("NodeBuilder keeps a node's data in one of two caches") {
    // A node that calls itself global has its data in the shared cache.
    REQUIRE(NodeBuilderData::cacheFor(false, true) == CacheChoice::Global);
    // One that does not has it in the per-build cache.
    REQUIRE(NodeBuilderData::cacheFor(false, false) == CacheChoice::PerBuild);
    REQUIRE(NodeBuilderData::cacheFor(false, true) != NodeBuilderData::cacheFor(false, false));
    // A caller may name a cache, which overrides the node's own answer either way.
    REQUIRE(NodeBuilderData::cacheFor(true, true) == CacheChoice::Named);
    REQUIRE(NodeBuilderData::cacheFor(true, false) == CacheChoice::Named);
}

TEST_CASE("NodeBuilder nests a node's data two or three levels deep") {
    // The node and the stage, always.
    REQUIRE(NodeBuilderData::levelCount(false) == 2);
    // Inside a sub-build the sub-build is a third level, so the same node keeps different data in
    // different sub-builds.
    REQUIRE(NodeBuilderData::levelCount(true) == 3);
    REQUIRE(NodeBuilderData::levelCount(true) != NodeBuilderData::levelCount(false));
    // And that third level is only reached when the builder is inside one.
    REQUIRE(NodeBuilderData::subBuildLevelIsReached(true));
    REQUIRE_FALSE(NodeBuilderData::subBuildLevelIsReached(false));
}

TEST_CASE("NodeBuilder asks for a node's properties under the word for any") {
    // The default is the word for any rather than whatever stage the builder is in, so a property
    // belongs to the node as a whole and asking from inside a stage reaches the same record.
    REQUIRE(std::string(NodeBuilderData::kDefaultPropertiesStage) == "any");
    REQUIRE(std::string(NodeBuilderData::kDefaultPropertiesStage) != "fragment");
    REQUIRE(std::string(NodeBuilderData::kDefaultPropertiesStage) != "vertex");
}

TEST_CASE("NodeBuilder makes a node's properties on the first ask") {
    // The record is made then and given one entry, so a caller always receives something with the
    // field it expects rather than an empty record to test.
    REQUIRE(NodeBuilderData::propertiesAreMadeOnFirstAsk());
    REQUIRE(NodeBuilderData::startsWithAnEmptyOutputNode());
}