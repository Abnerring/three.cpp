#include "nodes/display/ToneMappingFunctions.h"

#include "catch.hpp"

#include <cmath>

TEST_CASE("ToneMappingFunctions' linear curve multiplies and holds") {
    // Exposure is a plain multiply.
    REQUIRE(ToneMappingFunctions::linear(0.5, 1.0) == Approx(0.5).epsilon(1e-12));
    REQUIRE(ToneMappingFunctions::linear(0.5, 2.0) == Approx(1.0).epsilon(1e-12));
    // Above one the hold takes over, which is what brings a bright colour into the range.
    REQUIRE(ToneMappingFunctions::linear(1.0, 4.0) == Approx(1.0).epsilon(1e-12));
    // Doubling exposure does not double a colour that is already near the top.
    REQUIRE(ToneMappingFunctions::linear(0.6, 2.0) == Approx(1.0).epsilon(1e-12));
    REQUIRE(ToneMappingFunctions::linear(0.6, 1.0) == Approx(0.6).epsilon(1e-12));
}

TEST_CASE("ToneMappingFunctions' reinhard curve never reaches one") {
    // A half over one and a half is a third.
    REQUIRE(ToneMappingFunctions::reinhard(0.5, 1.0) == Approx(1.0 / 3.0).epsilon(1e-12));
    // It rises with the colour without ever reaching one, which is the property that makes the
    // hold unnecessary.
    REQUIRE(ToneMappingFunctions::reinhard(1.0, 1.0) == Approx(0.5).epsilon(1e-12));
    REQUIRE(ToneMappingFunctions::reinhard(9.0, 1.0) == Approx(9.0 / 10.0).epsilon(1e-12));
    REQUIRE(ToneMappingFunctions::reinhard(1000.0, 1.0) < 1.0);
    REQUIRE(ToneMappingFunctions::reinhard(1000.0, 1.0) > 0.99);
    REQUIRE(ToneMappingFunctions::reinhard(1000.0, 1.0) == Approx(1000.0 / 1001.0).epsilon(1e-12));
    // Nought stays nought.
    REQUIRE(ToneMappingFunctions::reinhard(0.0, 1.0) == 0.0);
}

TEST_CASE("ToneMappingFunctions' filmic curve takes a black offset off first") {
    // A colour at or below the offset gives nought, since the subtraction is clamped there.
    REQUIRE(ToneMappingFunctions::cineon(0.004, 1.0) == Approx(0.0).margin(1e-12));
    REQUIRE(ToneMappingFunctions::cineon(0.0, 1.0) == Approx(0.0).margin(1e-12));
    REQUIRE(ToneMappingFunctions::cineon(0.002, 1.0) == Approx(0.0).margin(1e-12));
    // A half comes out near a half, and the arithmetic is written out here so that the constants
    // can be checked: with the offset off, a is nought point four nine six times three point
    // five seven five two, b is nought point four nine six times four point seven seven five two
    // plus nought point nought six, and their quotient raised to two point two gives this.
    // The same expression written out so the assertion does not rest on a value I worked by
    // hand; the two are compared to the same margin.
    const double c = 0.5 - ToneMappingFunctions::kBlackOffset;
    const double num = c * (c * ToneMappingFunctions::kSharedSlope + ToneMappingFunctions::kNumeratorAddend);
    const double den = c * (c * ToneMappingFunctions::kSharedSlope + ToneMappingFunctions::kDenominatorAddend) + ToneMappingFunctions::kDenominatorOffset;
    const double expected = std::pow(num / den, ToneMappingFunctions::kPower);
    REQUIRE(ToneMappingFunctions::cineon(0.5, 1.0) == Approx(expected).epsilon(1e-12));
    // And the value is near a half, which is the property the constants were chosen for.
    REQUIRE(ToneMappingFunctions::cineon(0.5, 1.0) == Approx(0.5).margin(0.01));
}

TEST_CASE("ToneMappingFunctions' filmic curve is not the linear one") {
    // The four curves give four different answers at the same input, so none is another by
    // accident.
    const double in = 0.8;
    REQUIRE(ToneMappingFunctions::linear(in, 1.0) != ToneMappingFunctions::reinhard(in, 1.0));
    REQUIRE(ToneMappingFunctions::linear(in, 1.0) != ToneMappingFunctions::cineon(in, 1.0));
    REQUIRE(ToneMappingFunctions::reinhard(in, 1.0) != ToneMappingFunctions::cineon(in, 1.0));
    // The filmic curve is brighter than the dividing one and darker than the plain one at this
    // input, which is what a curve that lifts the middle while holding the top should be. The
    // dividing curve gives nought point four four and the filmic one nought point six three,
    // so it is the dividing one that is the darkest here, not the filmic one; my first
    // reading of the direction was wrong and this assertion records the corrected one.
    REQUIRE(ToneMappingFunctions::cineon(in, 1.0) > ToneMappingFunctions::reinhard(in, 1.0));
    REQUIRE(ToneMappingFunctions::cineon(in, 1.0) < ToneMappingFunctions::linear(in, 1.0));
    // The dividing one is the darkest of the three at this input.
    REQUIRE(ToneMappingFunctions::reinhard(in, 1.0) < ToneMappingFunctions::linear(in, 1.0));
    // And its constants are the ones written above.
    REQUIRE(ToneMappingFunctions::kBlackOffset == 0.004);
    REQUIRE(ToneMappingFunctions::kSharedSlope == 6.2);
    REQUIRE(ToneMappingFunctions::kNumeratorAddend == 0.5);
    REQUIRE(ToneMappingFunctions::kDenominatorAddend == 1.7);
    REQUIRE(ToneMappingFunctions::kDenominatorOffset == 0.06);
    REQUIRE(ToneMappingFunctions::kPower == 2.2);
    // The two addends differ, which is the transcription risk the comment names.
    REQUIRE(ToneMappingFunctions::kNumeratorAddend != ToneMappingFunctions::kDenominatorAddend);
}
