#include "nodes/core/ColorSpaceNode.h"

#include "catch.hpp"

#include <string>

using Step = ColorSpaceNode::Step;

TEST_CASE("ColorSpaceNode skips a conversion in four cases") {
    // Management exactly off is one of them.
    REQUIRE_FALSE(ColorSpaceNode::converts(false, "sRGB", "Linear"));
    // Equal names are another.
    REQUIRE_FALSE(ColorSpaceNode::converts(true, "sRGB", "sRGB"));
    // An empty source or target is a third and a fourth, which the truth tests reach.
    REQUIRE_FALSE(ColorSpaceNode::converts(true, "", "Linear"));
    REQUIRE_FALSE(ColorSpaceNode::converts(true, "sRGB", ""));
    // Two different names with both present do convert.
    REQUIRE(ColorSpaceNode::converts(true, "sRGB", "Linear"));
    REQUIRE(ColorSpaceNode::converts(true, "Linear", "sRGB"));
}

TEST_CASE("ColorSpaceNode's empties are not errors") {
    // A name that is present but empty skips the conversion rather than failing, which is the
    // consequence of testing for truth; an error would be the other reasonable choice.
    REQUIRE_FALSE(ColorSpaceNode::converts(true, "", "sRGB"));
    REQUIRE_FALSE(ColorSpaceNode::converts(true, "sRGB", ""));
    // Two empties are equal as well, so they skip for two reasons at once and the answer is the
    // same either way.
    REQUIRE_FALSE(ColorSpaceNode::converts(true, "", ""));
}

TEST_CASE("ColorSpaceNode runs its three steps in one order") {
    // The source's transfer comes first, then the matrix, then the target's transfer.
    const auto all = ColorSpaceNode::steps(true, true, true);
    REQUIRE(all.size() == 3);
    REQUIRE(all[0] == Step::UndoSourceTransfer);
    REQUIRE(all[1] == Step::PrimariesMatrix);
    REQUIRE(all[2] == Step::ApplyTargetTransfer);
    // With no primaries difference the matrix is left out but the order of the other two holds.
    const auto noMatrix = ColorSpaceNode::steps(true, false, true);
    REQUIRE(noMatrix.size() == 2);
    REQUIRE(noMatrix[0] == Step::UndoSourceTransfer);
    REQUIRE(noMatrix[1] == Step::ApplyTargetTransfer);
    // A source that is not in the transfer's space skips the first step.
    const auto linearIn = ColorSpaceNode::steps(false, true, true);
    REQUIRE(linearIn.size() == 2);
    REQUIRE(linearIn[0] == Step::PrimariesMatrix);
    // Nothing needed gives nothing.
    REQUIRE(ColorSpaceNode::steps(false, false, false).empty());
}

TEST_CASE("ColorSpaceNode takes the output space with a fallback") {
    // The context's own is used when it is there.
    REQUIRE(ColorSpaceNode::outputColorSpace("srgb", "display-p3") == "srgb");
    // An empty one falls back to the renderer's, which is what the short circuiting or does.
    REQUIRE(ColorSpaceNode::outputColorSpace("", "display-p3") == "display-p3");
    REQUIRE(ColorSpaceNode::outputColorSpace("", "") == "");
}

TEST_CASE("ColorSpaceNode resolves its two names and passes the rest") {
    // The working name becomes the working space.
    REQUIRE(ColorSpaceNode::resolve(ColorSpaceNode::kWorkingName, "linear", "srgb") == "linear");
    // The output name becomes the resolved output.
    REQUIRE(ColorSpaceNode::resolve(ColorSpaceNode::kOutputName, "linear", "srgb") == "srgb");
    // Anything else is itself, including a name that differs only in case.
    REQUIRE(ColorSpaceNode::resolve("sRGB", "linear", "srgb") == "sRGB");
    REQUIRE(ColorSpaceNode::resolve("workingcolorspace", "linear", "srgb") == "workingcolorspace");
    REQUIRE(std::string(ColorSpaceNode::kNodeType) == "vec4");
}