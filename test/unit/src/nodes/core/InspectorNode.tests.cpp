#include "nodes/core/InspectorNode.h"

#include "catch.hpp"

#include <string>

TEST_CASE("InspectorNode borrows the wrapped node's name when it has none") {
    // Its own name is used when it has one.
    REQUIRE(InspectorNode::name("mine", "theirs") == "mine");
    // An empty one falls through to the wrapped node's, which is what the short circuiting or
    // does; a test that took the name as fixed would answer with nothing here.
    REQUIRE(InspectorNode::name("", "theirs") == "theirs");
    REQUIRE(InspectorNode::name("", "theirs") != "");
    // Two empties give nothing, since there is nothing to borrow.
    REQUIRE(InspectorNode::name("", "") == "");
    REQUIRE(InspectorNode::isInspectorNode);
    REQUIRE(std::string(InspectorNode::kTypeName) == "InspectorNode");
}

TEST_CASE("InspectorNode runs its callback only for an exactly true flag") {
    // Both conditions have to hold.
    REQUIRE(InspectorNode::runsCallback(true, true));
    // The flag is compared against true exactly, so a value that is merely present does not
    // count; here the flag is false and the callback is there.
    REQUIRE_FALSE(InspectorNode::runsCallback(false, true));
    // And a callback that is absent stops it whatever the flag says.
    REQUIRE_FALSE(InspectorNode::runsCallback(true, false));
    REQUIRE_FALSE(InspectorNode::runsCallback(false, false));
    // The two conditions are tested differently, so the four answers are not symmetric.
    REQUIRE(InspectorNode::runsCallback(true, true) != InspectorNode::runsCallback(false, true));
}

TEST_CASE("InspectorNode warns about the backend only in one combination") {
    // Both negations have to hold: not WebGPU and not the base class.
    REQUIRE(InspectorNode::warnsAboutBackend(false, false));
    // A default inspector on a non-WebGPU backend is silent.
    REQUIRE_FALSE(InspectorNode::warnsAboutBackend(false, true));
    // And the WebGPU backend is silent whatever the inspector is.
    REQUIRE_FALSE(InspectorNode::warnsAboutBackend(true, false));
    REQUIRE_FALSE(InspectorNode::warnsAboutBackend(true, true));
    // Counting shows exactly one of the four combinations warns.
    int warnings = 0;
    for (const bool gpu : {true, false}) {
        for (const bool base : {true, false}) {
            if (InspectorNode::warnsAboutBackend(gpu, base)) warnings += 1;
        }
    }
    REQUIRE(warnings == 1);
}

TEST_CASE("InspectorNode's warning names the method and the backend") {
    const std::string warning = InspectorNode::backendWarning();
    REQUIRE(warning.find("toInspector") != std::string::npos);
    REQUIRE(warning.find("WebGPU") != std::string::npos);
    REQUIRE(warning == "TSL: \".toInspector()\" is only available with WebGPU.");
    // The refresh is once per frame.
    REQUIRE(std::string(InspectorNode::kUpdateType) == "FRAME");
}

TEST_CASE("InspectorNode's wrapper answers with the wrapped node's chain") {
    // The inspector is inserted before the wrapped node and the answer is that node's
    // before-chain, not the inspector, so the original node is what continues in the expression.
    REQUIRE(InspectorNode::answersTheWrappedNodesChain());
}