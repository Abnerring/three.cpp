#include "nodes/core/ViewportDepthNode.h"

#include "catch.hpp"

#include <string>

using Result = ViewportDepthNode::Result;

TEST_CASE("ViewportDepthNode reports a float and has three scopes") {
    REQUIRE(std::string(ViewportDepthNode::kNodeType) == "float");
    const auto scopes = ViewportDepthNode::scopes();
    REQUIRE(scopes.size() == 3);
    REQUIRE(scopes[0] == "depthBase");
    REQUIRE(scopes[1] == "depth");
    REQUIRE(scopes[2] == "linearDepth");
    REQUIRE(ViewportDepthNode::isViewportDepthNode);
    REQUIRE(std::string(ViewportDepthNode::kTypeName) == "ViewportDepthNode");
}

TEST_CASE("ViewportDepthNode's base scope is the only one that answers with frag depth") {
    // Only the base scope has a value of its own to produce when generating.
    REQUIRE(ViewportDepthNode::answersWithFragDepth("depthBase"));
    REQUIRE_FALSE(ViewportDepthNode::answersWithFragDepth("depth"));
    REQUIRE_FALSE(ViewportDepthNode::answersWithFragDepth("linearDepth"));
    REQUIRE_FALSE(ViewportDepthNode::answersWithFragDepth(""));
    REQUIRE(ViewportDepthNode::answersWithFragDepth("depthBase") !=
            ViewportDepthNode::answersWithFragDepth("depth"));
}

TEST_CASE("ViewportDepthNode sets itself up in seven ways") {
    // The base scope assigns the value if there is one and does nothing if there is not, so a
    // node in that scope without a value answers nothing at all.
    REQUIRE(ViewportDepthNode::setupResult("depthBase", true, false) == Result::AssignedBase);
    REQUIRE(ViewportDepthNode::setupResult("depthBase", false, false) == Result::Nothing);
    REQUIRE(ViewportDepthNode::setupResult("depthBase", false, true) == Result::Nothing);
    // The depth scope converts the view position and never uses the value, whichever camera kind
    // it is given, so the value's presence does not change the answer.
    REQUIRE(ViewportDepthNode::setupResult("depth", false, true) == Result::PerspectiveDepth);
    REQUIRE(ViewportDepthNode::setupResult("depth", true, true) == Result::PerspectiveDepth);
    REQUIRE(ViewportDepthNode::setupResult("depth", true, false) == Result::OrthographicDepth);
    // The linear scope has three cases: from the value with a perspective camera, from the value
    // as it stands with an orthographic one, and from the view position when there is no value.
    REQUIRE(ViewportDepthNode::setupResult("linearDepth", true, true) == Result::LinearFromValue);
    REQUIRE(ViewportDepthNode::setupResult("linearDepth", true, false) == Result::ValueAsItStands);
    REQUIRE(ViewportDepthNode::setupResult("linearDepth", false, true) == Result::LinearFromPosition);
    REQUIRE(ViewportDepthNode::setupResult("linearDepth", false, false) == Result::LinearFromPosition);
}

TEST_CASE("ViewportDepthNode's linear value takes two conversions or none") {
    // A perspective camera has to undo the projection and convert again, so two.
    REQUIRE(ViewportDepthNode::conversionsForLinearValue(true) == 2);
    // An orthographic one is already linear, so the value passes through with none at all.
    REQUIRE(ViewportDepthNode::conversionsForLinearValue(false) == 0);
    REQUIRE(ViewportDepthNode::conversionsForLinearValue(true) !=
            ViewportDepthNode::conversionsForLinearValue(false));
    // The value is ignored in the depth scope and used in the other two.
    REQUIRE_FALSE(ViewportDepthNode::valueIsUsed("depth"));
    REQUIRE(ViewportDepthNode::valueIsUsed("depthBase"));
    REQUIRE(ViewportDepthNode::valueIsUsed("linearDepth"));
    REQUIRE_FALSE(ViewportDepthNode::isCacheable());
}