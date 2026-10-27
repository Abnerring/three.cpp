#include "nodes/display/RenderOutputNode.h"

#include "catch.hpp"

#include <string>

using Step = RenderOutput::Step;

TEST_CASE("RenderOutput's alpha steps cancel and leave the clamp") {
    // With premultiplied alpha on, the colour is undone and put back with the tone mapping and
    // colour space work in between. The pair therefore undo one another, so the only lasting
    // effect of the alpha handling is the clamp at the very start.
    const auto all = RenderOutput::steps(true, true, true);
    REQUIRE(all.size() == 5);
    REQUIRE(all[0] == Step::ClampAlpha);
    REQUIRE(all[1] == Step::Unpremultiply);
    REQUIRE(all[2] == Step::ToneMapping);
    REQUIRE(all[3] == Step::WorkingToOutput);
    REQUIRE(all[4] == Step::Premultiply);
    // The pair is at the two ends of the middle work, which is what makes them cancel.
    REQUIRE(all[1] == Step::Unpremultiply);
    REQUIRE(all[4] == Step::Premultiply);
}

TEST_CASE("RenderOutput drops the alpha steps together") {
    // The two are controlled by one flag, so they are always both there or both away; a test
    // that found one without the other would mean the flag had been read twice.
    const auto none = RenderOutput::steps(false, false, false);
    REQUIRE(none.size() == 1);
    REQUIRE(none[0] == Step::ClampAlpha);
    const auto middle = RenderOutput::steps(false, true, true);
    REQUIRE(middle.size() == 3);
    REQUIRE(middle[0] == Step::ClampAlpha);
    REQUIRE(middle[1] == Step::ToneMapping);
    REQUIRE(middle[2] == Step::WorkingToOutput);
}

TEST_CASE("RenderOutput's tone mapping falls back twice") {
    // The own value is taken when it is there.
    REQUIRE(RenderOutput::toneMapping(true, 4, 7, 0) == 4);
    // Otherwise the context's.
    REQUIRE(RenderOutput::toneMapping(false, 4, 7, 0) == 7);
    // And the chosen value falls back to the none value when it is nought, so a context tone
    // mapping of nought gives the none value, which is itself nought and so indistinguishable
    // here; the point is that the second test ran at all.
    REQUIRE(RenderOutput::toneMapping(false, 4, 0, 0) == 0);
    REQUIRE(RenderOutput::toneMapping(true, 0, 7, 0) == 0);
    // With a none value that is not nought the second test becomes visible.
    REQUIRE(RenderOutput::toneMapping(false, 4, 0, 99) == 99);
    REQUIRE(RenderOutput::toneMapping(true, 0, 7, 99) == 99);
}

TEST_CASE("RenderOutput's colour space falls back the same way") {
    REQUIRE(RenderOutput::outputColorSpace(true, "srgb", "p3", "none") == "srgb");
    REQUIRE(RenderOutput::outputColorSpace(false, "srgb", "p3", "none") == "p3");
    // An empty chosen value falls back, which is the truth test.
    REQUIRE(RenderOutput::outputColorSpace(false, "srgb", "", "none") == "none");
    REQUIRE(RenderOutput::outputColorSpace(true, "", "p3", "none") == "none");
}

TEST_CASE("RenderOutput skips the colour space conversion in two cases") {
    // The none space skips it, and so does the working space, which would be a conversion to
    // itself.
    REQUIRE_FALSE(RenderOutput::converts(true, false));
    REQUIRE_FALSE(RenderOutput::converts(false, true));
    REQUIRE_FALSE(RenderOutput::converts(true, true));
    // Anything else converts.
    REQUIRE(RenderOutput::converts(false, false));
    // And the alpha clamp holds an out of range value.
    REQUIRE(RenderOutput::clampAlpha(-0.5) == 0.0);
    REQUIRE(RenderOutput::clampAlpha(1.5) == 1.0);
    REQUIRE(RenderOutput::clampAlpha(0.5) == 0.5);
    REQUIRE(std::string(RenderOutput::kTypeName) == "RenderOutputNode");
}