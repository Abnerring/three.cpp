#include "nodes/core/ToonOutlinePass.h"

#include "catch.hpp"

#include <string>

TEST_CASE("ToonOutlinePass names itself and its material") {
    REQUIRE(std::string(ToonOutlinePass::kTypeName) == "ToonOutlinePassNode");
    REQUIRE(std::string(ToonOutlinePass::kPassName) == "Outline Pass");
    REQUIRE(std::string(ToonOutlinePass::kMaterialName) == "Toon_Outline");
    REQUIRE(std::string(ToonOutlinePass::kSide) == "BackSide");
    // The defaults a call without arguments gets.
    REQUIRE(ToonOutlinePass::kDefaultThickness == 0.003);
    REQUIRE(ToonOutlinePass::kDefaultAlpha == 1.0);
    REQUIRE(ToonOutlinePass::kDefaultThickness != 0.03);
}

TEST_CASE("ToonOutlinePass takes the second position from the first") {
    // A point offset along the third axis gives a direction pointing back along it, since the
    // first is taken from the second.
    const auto back = ToonOutlinePass::direction(0.0, 0.0, 0.0, 0.0, 0.0, 1.0);
    REQUIRE(back.z == Approx(-1.0).epsilon(1e-12));
    REQUIRE(back.x == Approx(0.0).margin(1e-12));
    // The other way round would give the opposite, which would push the outline inwards.
    const auto forward = ToonOutlinePass::direction(0.0, 0.0, 1.0, 0.0, 0.0, 0.0);
    REQUIRE(forward.z == Approx(1.0).epsilon(1e-12));
    REQUIRE(back.z == Approx(-forward.z).epsilon(1e-12));
    // The result is of unit length, so the push does not scale with the distance between the
    // two points.
    const auto diagonal = ToonOutlinePass::direction(0.0, 0.0, 0.0, 3.0, 4.0, 0.0);
    REQUIRE(diagonal.x == Approx(-0.6).epsilon(1e-12));
    REQUIRE(diagonal.y == Approx(-0.8).epsilon(1e-12));
    const double length = diagonal.x * diagonal.x + diagonal.y * diagonal.y;
    REQUIRE(length == Approx(1.0).epsilon(1e-12));
}

TEST_CASE("ToonOutlinePass scales its push by the clip space w") {
    // The thickness alone, when the w is one.
    REQUIRE(ToonOutlinePass::push(0.003, 1.0) == Approx(0.003).epsilon(1e-15));
    // A larger w pushes further, which is what keeps the outline the same visual width at a
    // distance; without this the outline would shrink as the object moved away.
    REQUIRE(ToonOutlinePass::push(0.003, 10.0) == Approx(0.03).epsilon(1e-15));
    REQUIRE(ToonOutlinePass::push(0.003, 100.0) == Approx(0.3).epsilon(1e-15));
    // The two ends of that range differ by the same factor the w does.
    REQUIRE(ToonOutlinePass::push(0.003, 100.0) / ToonOutlinePass::push(0.003, 10.0) ==
            Approx(10.0).epsilon(1e-12));
}

TEST_CASE("ToonOutlinePass' thickness ratio does nothing") {
    // The factor is one, so it changes nothing; the source carries a note that a per vertex
    // ratio is not supported, so this is a stub rather than a setting.
    REQUIRE(ToonOutlinePass::thicknessRatio() == 1.0);
    REQUIRE(ToonOutlinePass::thicknessRatio() != 2.0);
}

TEST_CASE("ToonOutlinePass outlines only toon materials that are not wireframes") {
    // Both toon kinds receive an outline.
    REQUIRE(ToonOutlinePass::outlines(true, false, false));
    REQUIRE(ToonOutlinePass::outlines(false, true, false));
    REQUIRE(ToonOutlinePass::outlines(true, true, false));
    // Neither kind does not.
    REQUIRE_FALSE(ToonOutlinePass::outlines(false, false, false));
    // And a wireframe is skipped even when it is toon, which is the second condition.
    REQUIRE_FALSE(ToonOutlinePass::outlines(true, false, true));
    REQUIRE_FALSE(ToonOutlinePass::outlines(false, true, true));
}