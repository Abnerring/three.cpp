#include "nodes/core/ViewportUtils.h"

#include "catch.hpp"

#include <string>

TEST_CASE("ViewportUtils corrects only a sample in front of the surface") {
    // A negative difference means the sampled depth is nearer, which is the case the function
    // exists for; there it answers with the screen coordinate.
    REQUIRE(ViewportUtils::choose(-1.0) == ViewportUtils::Choice::Screen);
    REQUIRE(ViewportUtils::choose(-0.001) == ViewportUtils::Choice::Screen);
    // A positive difference means the sample is behind, so the given coordinate is kept.
    REQUIRE(ViewportUtils::choose(1.0) == ViewportUtils::Choice::Given);
    REQUIRE(ViewportUtils::choose(0.001) == ViewportUtils::Choice::Given);
}

TEST_CASE("ViewportUtils keeps its own coordinate when the depths are equal") {
    // Exactly nought is not less than nought, so a surface compared with itself keeps its
    // coordinate rather than being corrected; the boundary is where a strict comparison matters.
    REQUIRE(ViewportUtils::choose(0.0) == ViewportUtils::Choice::Given);
    REQUIRE(ViewportUtils::choose(0.0) != ViewportUtils::Choice::Screen);
    // The two sides of nought give the two different answers, which is what makes the strictness
    // of the comparison visible.
    REQUIRE(ViewportUtils::choose(-0.001) != ViewportUtils::choose(0.001));
}

TEST_CASE("ViewportUtils takes its depths from the linear kind") {
    // The difference is taken against the linear depth rather than the raw one, which a test
    // can only record rather than exercise.
    REQUIRE(std::string(ViewportUtils::kDepthKind) == "linear");
    REQUIRE(std::string(ViewportUtils::kDepthKind) != "raw");
}

TEST_CASE("ViewportUtils' two choices are not the same") {
    // The two answers exist and differ, which is what the choice is for.
    REQUIRE(ViewportUtils::Choice::Screen != ViewportUtils::Choice::Given);
}