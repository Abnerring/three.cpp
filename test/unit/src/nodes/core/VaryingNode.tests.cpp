#include "nodes/core/VaryingNode.h"

#include "catch.hpp"

#include <string>

using Emission = VaryingNode::Emission;

TEST_CASE("VaryingNode builds its wrapped node in the vertex stage") {
    // The stage is fixed rather than following the use, so a varying read from the fragment stage
    // still has its node built for the vertex one.
    REQUIRE(std::string(VaryingNode::kNodeStage) == "VERTEX");
    REQUIRE(std::string(VaryingNode::kNodeStage) != "FRAGMENT");
    REQUIRE(VaryingNode::isVaryingNode);
    REQUIRE(std::string(VaryingNode::kTypeName) == "VaryingNode");
}

TEST_CASE("VaryingNode sets its interpolation flag to whether the stage is the fragment one") {
    // The value assigned is the answer to that question, not a constant.
    REQUIRE(VaryingNode::needsInterpolationAssigned("fragment"));
    REQUIRE_FALSE(VaryingNode::needsInterpolationAssigned("vertex"));
    // So used from the vertex stage the flag is set to false, which changes nothing, while used
    // from the fragment stage it becomes true.
    REQUIRE(VaryingNode::needsInterpolationAssigned("fragment") !=
            VaryingNode::needsInterpolationAssigned("vertex"));
    // And the assignment only runs when the flag is falsy, so a true one is not overwritten.
    REQUIRE_FALSE(VaryingNode::overwritesExistingFlag());
    REQUIRE(VaryingNode::globalByDefault);
}

TEST_CASE("VaryingNode emits once per block in the vertex stage and once in total elsewhere") {
    // The vertex stage guards on the current flow block, so the assignment is emitted once for
    // each block; other stages guard on nothing, so it is emitted once however many blocks there
    // are.
    REQUIRE(VaryingNode::emissionFor("vertex") == Emission::PerBlock);
    REQUIRE(VaryingNode::emissionFor("fragment") == Emission::OnceInTotal);
    REQUIRE(VaryingNode::emissionFor("compute") == Emission::OnceInTotal);
    REQUIRE(VaryingNode::emissionFor("") == Emission::OnceInTotal);
    REQUIRE(VaryingNode::emissionFor("vertex") != VaryingNode::emissionFor("fragment"));
}

TEST_CASE("VaryingNode's setup and analyze do the same thing") {
    // Both build the varying and queue the node for the vertex stage, so one is a copy of the
    // other rather than a different step.
    REQUIRE(VaryingNode::setupAndAnalyzeAreTheSame());
    REQUIRE_FALSE(VaryingNode::isCacheable());
}

TEST_CASE("VaryingNode hashes its name or falls back") {
    // An explicit name is the hash, so two nodes with the same name hash alike.
    REQUIRE(VaryingNode::hash(true, "vPosition", "ordinary") == "vPosition");
    // Without one the ordinary hash is used, so two unnamed nodes do not hash alike unless their
    // ordinary hashes agree.
    REQUIRE(VaryingNode::hash(false, "vPosition", "ordinary") == "ordinary");
    REQUIRE(VaryingNode::hash(false, "vPosition", "ordinary") != "vPosition");
    // The two paths differ, which is the point of having a fallback.
    REQUIRE(VaryingNode::hash(true, "v", "o") != VaryingNode::hash(false, "v", "o"));
}