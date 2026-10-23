#include "nodes/core/MRTNode.h"

#include "catch.hpp"

#include <string>
#include <vector>

TEST_CASE("MRTNode finds a texture by name or reports a miss as minus one") {
    const std::vector<std::string> names{"output", "normal", "depth"};
    // A name that is there gives its index.
    REQUIRE(MRTNode::textureIndex(names, "output") == 0);
    REQUIRE(MRTNode::textureIndex(names, "normal") == 1);
    REQUIRE(MRTNode::textureIndex(names, "depth") == 2);
    // A name that is not gives minus one, which is the array idiom for a miss; a caller comparing
    // against nought would take the first texture for a miss.
    REQUIRE(MRTNode::textureIndex(names, "colour") == -1);
    REQUIRE(MRTNode::textureIndex(names, "") == -1);
    REQUIRE(MRTNode::textureIndex({}, "output") == -1);
    // The miss is not the first index, which is the confusion worth pinning.
    REQUIRE(MRTNode::textureIndex(names, "colour") != 0);
    REQUIRE(MRTNode::isMRTNode);
}

TEST_CASE("MRTNode's blend fallback is not its default") {
    // The output name has a default of its own, which is material blending.
    REQUIRE(MRTNode::blendMode(true, false, "") == "MaterialBlending");
    // Any other name falls back to no blending, which is a different answer.
    REQUIRE(MRTNode::blendMode(false, false, "") == "NoBlending");
    REQUIRE(MRTNode::blendMode(false, false, "") != MRTNode::blendMode(true, false, ""));
    // A name that was given a mode uses it whatever it is called.
    REQUIRE(MRTNode::blendMode(false, true, "AdditiveBlending") == "AdditiveBlending");
    REQUIRE(MRTNode::blendMode(true, true, "AdditiveBlending") == "AdditiveBlending");
    REQUIRE(std::string(MRTNode::kNoBlending) == "NoBlending");
    REQUIRE(std::string(MRTNode::kMaterialBlending) == "MaterialBlending");
}

TEST_CASE("MRTNode drops an output the render target has no texture for") {
    // An output declared in the node but never given a texture in the target is skipped without
    // a word, which the source's comment explains.
    REQUIRE(MRTNode::skipsUnusedOutput());
    // Members are placed at their texture's index rather than in declaration order, so a target
    // using the second texture but not the first leaves a gap rather than closing it up.
    REQUIRE(MRTNode::membersArePlacedByIndex());
    REQUIRE(MRTNode::leavesGaps(true));
    REQUIRE_FALSE(MRTNode::leavesGaps(false));
    REQUIRE(MRTNode::leavesGaps(true) != MRTNode::leavesGaps(false));
    REQUIRE(std::string(MRTNode::kTypeName) == "MRTNode");
}

TEST_CASE("MRTNode's merge lets its argument win") {
    // The two dictionaries are spread with the argument second, so a name both sides carry takes
    // the argument's value.
    REQUIRE(MRTNode::argumentWinsOnConflict());
    REQUIRE_FALSE(MRTNode::isCacheable());
}