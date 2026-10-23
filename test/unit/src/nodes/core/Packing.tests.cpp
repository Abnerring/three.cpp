#include "nodes/core/Packing.h"

#include "catch.hpp"

#include <cmath>
#include <string>

TEST_CASE("Packing takes a direction into a colour and back") {
    // The two ends of the direction range become the two ends of the colour range.
    REQUIRE(Packing::packPart(-1.0) == 0.0);
    REQUIRE(Packing::packPart(0.0) == 0.5);
    REQUIRE(Packing::packPart(1.0) == 1.0);
    // And the way back undoes it for values inside the range.
    REQUIRE(Packing::unpackPart(0.0) == -1.0);
    REQUIRE(Packing::unpackPart(0.5) == 0.0);
    REQUIRE(Packing::unpackPart(1.0) == 1.0);
    REQUIRE(Packing::unpackPart(Packing::packPart(0.3)) == Approx(0.3).epsilon(1e-12));
}

TEST_CASE("Packing's clamp is a clamp and not an absolute value") {
    // A negative is brought to nought, not to its size; that distinction is what keeps a pair
    // outside the unit circle from inventing a normal.
    REQUIRE(Packing::saturate(-0.5) == 0.0);
    REQUIRE(Packing::saturate(-0.5) != 0.5);
    // Values above one are brought to one.
    REQUIRE(Packing::saturate(1.5) == 1.0);
    // Values inside are left alone.
    REQUIRE(Packing::saturate(0.25) == 0.25);
    REQUIRE(Packing::saturate(0.0) == 0.0);
    REQUIRE(Packing::saturate(1.0) == 1.0);
}

TEST_CASE("Packing reconstructs the third part of a normal") {
    // A flat pair gives a third part of one, which is the normal pointing straight out.
    REQUIRE(Packing::reconstructedZ(0.0, 0.0) == 1.0);
    // A pair on the unit circle gives nought.
    REQUIRE(Packing::reconstructedZ(1.0, 0.0) == 0.0);
    REQUIRE(Packing::reconstructedZ(0.0, 1.0) == 0.0);
    // A pair inside the circle gives the remainder rooted, which is less than one.
    REQUIRE(Packing::reconstructedZ(0.5, 0.0) == Approx(std::sqrt(0.75)).epsilon(1e-12));
    REQUIRE(Packing::reconstructedZ(0.6, 0.8) == Approx(0.0).margin(1e-12));
}

TEST_CASE("Packing gives nought rather than a not a number outside the circle") {
    // A pair beyond the unit circle leaves a negative remainder. Without the clamp the root
    // would be a not a number; with it the answer is nought.
    REQUIRE(Packing::reconstructedZ(2.0, 0.0) == 0.0);
    REQUIRE(std::isfinite(Packing::reconstructedZ(2.0, 0.0)));
    REQUIRE_FALSE(std::isnan(Packing::reconstructedZ(2.0, 0.0)));
    // The same for a pair just outside the circle, which is the case a normal map with slightly
    // wrong values actually reaches.
    REQUIRE(Packing::reconstructedZ(0.708, 0.708) == 0.0);
    REQUIRE(std::isfinite(Packing::reconstructedZ(1.01, 0.0)));
}

TEST_CASE("Packing's renamed helpers name both of their names") {
    REQUIRE(Packing::packRenameWarning() ==
            "TSL: \"directionToColor()\" has been renamed to \"packNormalToRGB()\".");
    REQUIRE(Packing::unpackRenameWarning() ==
            "TSL: \"colorToDirection()\" has been renamed to \"unpackRGBToNormal()\".");
    // Each sentence carries both the old name and the new one, which is what makes it useful.
    REQUIRE(Packing::packRenameWarning().find("directionToColor") != std::string::npos);
    REQUIRE(Packing::packRenameWarning().find("packNormalToRGB") != std::string::npos);
    REQUIRE(Packing::unpackRenameWarning().find("colorToDirection") != std::string::npos);
    REQUIRE(Packing::unpackRenameWarning().find("unpackRGBToNormal") != std::string::npos);
}