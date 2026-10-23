#include "nodes/core/UVUtils.h"

#include "catch.hpp"

#include <cmath>

using Pair = UVUtils::Pair;

TEST_CASE("UVUtils treats anything that is not a function as a constant") {
    // A function is asked each time; anything else answers the same value however often it is
    // asked, which is what wrapping it in a function achieves.
    REQUIRE(UVUtils::kindOf(true) == UVUtils::ReplacementKind::Function);
    REQUIRE(UVUtils::kindOf(false) == UVUtils::ReplacementKind::Constant);
    REQUIRE(UVUtils::answer(UVUtils::ReplacementKind::Function, 0.25, 0.75) == 0.25);
    REQUIRE(UVUtils::answer(UVUtils::ReplacementKind::Constant, 0.25, 0.75) == 0.75);
    // The constant answer does not depend on what the function would have given.
    REQUIRE(UVUtils::answer(UVUtils::ReplacementKind::Constant, 0.1, 0.75) ==
            UVUtils::answer(UVUtils::ReplacementKind::Constant, 0.9, 0.75));
}

TEST_CASE("UVUtils' rotation keeps the centre still") {
    const Pair centre{0.5, 0.5};
    // The centre itself does not move, whatever the angle.
    for (int i = 0; i <= 8; ++i) {
        const Pair out = UVUtils::rotate(centre, i * 0.25, centre);
        REQUIRE(out.u == Approx(0.5).margin(1e-12));
        REQUIRE(out.v == Approx(0.5).margin(1e-12));
    }
    // A quarter turn sends a point to the right of the centre above it.
    const Pair right{0.6, 0.5};
    const Pair turned = UVUtils::rotate(right, 3.14159265358979323846 / 2.0, centre);
    REQUIRE(turned.u == Approx(0.5).margin(1e-12));
    REQUIRE(turned.v == Approx(0.6).margin(1e-12));
    // A full turn brings it back.
    const Pair full = UVUtils::rotate(right, 6.283185307179586, centre);
    REQUIRE(full.u == Approx(0.6).margin(1e-12));
    REQUIRE(full.v == Approx(0.5).margin(1e-12));
}

TEST_CASE("UVUtils' spherize pushes by a fifth power") {
    const Pair centre{0.5, 0.5};
    // At the centre nothing happens, since the offset is nought.
    const Pair atCentre = UVUtils::spherize(centre, 1.0, centre);
    REQUIRE(atCentre.u == Approx(0.5).margin(1e-12));
    // The offset for a squared distance of nought is nought, whatever the strength.
    REQUIRE(UVUtils::spherizeOffset(0.0, 5.0) == 0.0);
    // The offset is the squared distance squared times the strength, which for a distance of a
    // tenth is a squared distance of a hundredth and a fourth power of a ten thousandth.
    REQUIRE(UVUtils::spherizeOffset(0.01, 1.0) == Approx(0.0001).margin(1e-12));
    // A point a tenth to the right is therefore pushed by a tenth times that, and the result is
    // written out so that the power can be checked against a number.
    const Pair right{0.6, 0.5};
    const Pair out = UVUtils::spherize(right, 1.0, centre);
    REQUIRE(out.u == Approx(0.6 + 0.1 * 0.0001).margin(1e-12));
    REQUIRE(out.v == Approx(0.5).margin(1e-15));
}

TEST_CASE("UVUtils' spherize is not linear") {
    const Pair centre{0.5, 0.5};
    // Doubling the distance multiplies the displacement by thirty two, since the displacement
    // goes as the fifth power; a linear effect would multiply it by two.
    const Pair near_{0.6, 0.5};
    const Pair far_{0.7, 0.5};
    const double nearPush = UVUtils::spherize(near_, 1.0, centre).u - near_.u;
    const double farPush = UVUtils::spherize(far_, 1.0, centre).u - far_.u;
    REQUIRE(farPush / nearPush == Approx(32.0).epsilon(1e-9));
    REQUIRE(farPush / nearPush != Approx(2.0).epsilon(1e-9));
    // And the strength scales the displacement directly.
    REQUIRE(UVUtils::spherize(near_, 2.0, centre).u - near_.u ==
            Approx(2.0 * nearPush).margin(1e-18));
}
