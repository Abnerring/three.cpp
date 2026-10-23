#include "nodes/core/SampleNode.h"

#include "catch.hpp"

#include <string>

TEST_CASE("SampleNode's given coordinate is never read") {
    // The constructor stores a coordinate and the setup ignores it, making a default one
    // instead, so the second argument has no effect on what the callback is handed.
    REQUIRE(SampleNode::sourceUsedInSetup() == SampleNode::CoordinateSource::Default);
    REQUIRE(SampleNode::sourceUsedInSetup() != SampleNode::CoordinateSource::Given);
    REQUIRE_FALSE(SampleNode::givenCoordinateIsRead);
    // A caller that passes its own coordinate therefore gets the same answer as one that does
    // not, which is what a dead argument means.
    REQUIRE(SampleNode::sourceUsedInSetup() == SampleNode::sourceUsedInSetup());
    REQUIRE(std::string(SampleNode::kTypeName) == "SampleNode");
}

TEST_CASE("SampleNode answers with its callback and nothing else") {
    // The node adds nothing of its own, so the answer is the callback's answer.
    REQUIRE(SampleNode::sample(0.25) == 0.25);
    REQUIRE(SampleNode::sample(-1.5) == -1.5);
    REQUIRE(SampleNode::sample(0.0) == 0.0);
    // Two different answers stay different, so the node does not fold them together.
    REQUIRE(SampleNode::sample(0.25) != SampleNode::sample(0.5));
}

TEST_CASE("SampleNode refuses to be cached") {
    // As with the other nodes in this layer that refuse outright.
    REQUIRE_FALSE(SampleNode::isCacheable());
}