#include "nodes/display/ScreenNode.h"

#include "catch.hpp"

#include <string>

TEST_CASE("ScreenNode has four measurements") {
    const auto& scopes = ScreenNode::scopes();
    REQUIRE(scopes.size() == 4);
    REQUIRE(scopes[0] == "coordinate");
    REQUIRE(scopes[1] == "viewport");
    REQUIRE(scopes[2] == "size");
    REQUIRE(scopes[3] == "uv");
    REQUIRE(std::string(ScreenNode::kTypeName) == "ScreenNode");
    REQUIRE(ScreenNode::isViewportNode);
}

TEST_CASE("ScreenNode reports four parts only for the viewport") {
    // The viewport is a rectangle and needs four parts; the rest are pairs.
    REQUIRE(ScreenNode::nodeType("viewport") == "vec4");
    REQUIRE(ScreenNode::nodeType("coordinate") == "vec2");
    REQUIRE(ScreenNode::nodeType("size") == "vec2");
    REQUIRE(ScreenNode::nodeType("uv") == "vec2");
    // Counting shows only one of the four is a matrix-like four.
    int fours = 0;
    for (const auto& scope : ScreenNode::scopes()) {
        if (ScreenNode::nodeType(scope) == "vec4") fours += 1;
    }
    REQUIRE(fours == 1);
    // A name it does not know still answers with a pair, since the source falls through.
    REQUIRE(ScreenNode::nodeType("nope") == "vec2");
}

TEST_CASE("ScreenNode refreshes only the measurements taken from the renderer") {
    // The size and the viewport are read from the renderer every render.
    REQUIRE(ScreenNode::updateType("size") == "RENDER");
    REQUIRE(ScreenNode::updateType("viewport") == "RENDER");
    // The other two are worked out in the shader and need nothing.
    REQUIRE(ScreenNode::updateType("coordinate") == "NONE");
    REQUIRE(ScreenNode::updateType("uv") == "NONE");
    // The none kind is stated rather than left unset, so the two kinds are the only answers.
    REQUIRE(ScreenNode::updateType("uv") != ScreenNode::updateType("size"));
}

TEST_CASE("ScreenNode applies the pixel ratio only without a render target") {
    // A target's viewport is already in physical pixels, so the ratio is not applied again;
    // doing so would double count on a display that is not one to one.
    REQUIRE(ScreenNode::appliesPixelRatio(false));
    REQUIRE_FALSE(ScreenNode::appliesPixelRatio(true));
    REQUIRE(ScreenNode::appliesPixelRatio(false) != ScreenNode::appliesPixelRatio(true));
    // A target's viewport is read from the target itself.
    REQUIRE(ScreenNode::viewportFromTarget(true));
    REQUIRE_FALSE(ScreenNode::viewportFromTarget(false));
}

TEST_CASE("ScreenNode flips the vertical coordinate about the middle") {
    // The flip is the size less the coordinate, so the two ends swap and the middle stays.
    REQUIRE(ScreenNode::flippedY(100.0, 0.0) == 100.0);
    REQUIRE(ScreenNode::flippedY(100.0, 100.0) == 0.0);
    REQUIRE(ScreenNode::flippedY(100.0, 50.0) == 50.0);
    // Applying it twice gives the value back, which is what a flip about the middle means.
    REQUIRE(ScreenNode::flippedY(100.0, ScreenNode::flippedY(100.0, 30.0)) == 30.0);
    REQUIRE_FALSE(ScreenNode::isCacheable());
}
