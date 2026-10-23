#include "nodes/core/DepthConversion.h"

#include "catch.hpp"

#include <cmath>

TEST_CASE("DepthConversion's orthographic curve runs from nought to one") {
    // At the near plane the sum is nought, so the answer is nought; at the far plane the
    // numerator equals the divisor, so the answer is one.
    REQUIRE(DepthConversion::viewZToOrthographicDepth(-1.0, 1.0, 100.0) == Approx(0.0).margin(1e-12));
    REQUIRE(DepthConversion::viewZToOrthographicDepth(-100.0, 1.0, 100.0) == Approx(1.0).margin(1e-12));
    // Halfway between the planes gives a half, since the curve is a straight line.
    REQUIRE(DepthConversion::viewZToOrthographicDepth(-50.5, 1.0, 100.0) == Approx(0.5).margin(1e-12));
}

TEST_CASE("DepthConversion's reversed curve swaps the two ends") {
    // The reversed form is one at the near plane and nought at the far, which is exactly the
    // other curve the other way round.
    REQUIRE(DepthConversion::viewZToReversedOrthographicDepth(-1.0, 1.0, 100.0) ==
            Approx(1.0).margin(1e-12));
    REQUIRE(DepthConversion::viewZToReversedOrthographicDepth(-100.0, 1.0, 100.0) ==
            Approx(0.0).margin(1e-12));
    // The two therefore sum to one everywhere, which is the property the sign flip gives.
    for (int i = 0; i <= 10; ++i) {
        const double viewZ = -1.0 - 9.9 * i;
        REQUIRE(DepthConversion::viewZToOrthographicDepth(viewZ, 1.0, 100.0) +
                    DepthConversion::viewZToReversedOrthographicDepth(viewZ, 1.0, 100.0) ==
                Approx(1.0).margin(1e-12));
    }
}

TEST_CASE("DepthConversion's inverse undoes the curve in both modes") {
    // A depth of nought is the near plane and one the far.
    REQUIRE(DepthConversion::orthographicDepthToViewZ(0.0, 1.0, 100.0, false) == Approx(-1.0).margin(1e-12));
    REQUIRE(DepthConversion::orthographicDepthToViewZ(1.0, 1.0, 100.0, false) == Approx(-100.0).margin(1e-12));
    // The reversed mode swaps them, as the forward curves do.
    REQUIRE(DepthConversion::orthographicDepthToViewZ(0.0, 1.0, 100.0, true) == Approx(-100.0).margin(1e-12));
    REQUIRE(DepthConversion::orthographicDepthToViewZ(1.0, 1.0, 100.0, true) == Approx(-1.0).margin(1e-12));
    // Rounding a view depth through the curve and back gives it again.
    const double viewZ = -37.5;
    const double depth = DepthConversion::viewZToOrthographicDepth(viewZ, 1.0, 100.0);
    REQUIRE(DepthConversion::orthographicDepthToViewZ(depth, 1.0, 100.0, false) ==
            Approx(viewZ).margin(1e-9));
}

TEST_CASE("DepthConversion's perspective curve also runs from nought to one") {
    // At the near plane the added near value cancels the view depth, so the answer is nought.
    REQUIRE(DepthConversion::viewZToPerspectiveDepth(-1.0, 1.0, 100.0) == Approx(0.0).margin(1e-12));
    // At the far plane the numerator and denominator cancel, so the answer is one.
    REQUIRE(DepthConversion::viewZToPerspectiveDepth(-100.0, 1.0, 100.0) == Approx(1.0).margin(1e-12));
    // Unlike the orthographic curve this one is not a straight line: halfway between the two
    // planes gives a value near one rather than a half, since the curve crowds the near
    // range into the first small part of the depth. Worked out: the numerator is the near
    // less the halfway depth times the far, which is minus four thousand nine hundred and
    // fifty, and the denominator is the far less the near times the same depth, which is
    // minus four thousand nine hundred and ninety nine and a half; their quotient is
    // nought point nine nine nought one.
    const double middle = DepthConversion::viewZToPerspectiveDepth(-50.5, 1.0, 100.0);
    REQUIRE(middle > 0.9);
    REQUIRE(middle < 1.0);
    REQUIRE(middle != Approx(0.5).margin(1e-6));
}
