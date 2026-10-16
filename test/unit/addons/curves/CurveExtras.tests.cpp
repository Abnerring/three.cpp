#include "catch.hpp"
#include "curves/CurveExtras.h"
#include "math/JSNumberMath.h"
#include <cmath>
#include <type_traits>

// Reference values in this file were produced by running the pinned three.js module
// examples/jsm/curves/CurveExtras.js under the pinned Node oracle. The differential probe
// (test/unit/utils/parity/compare-curve-extras.mjs) compares the same samples bit for bit.
namespace {
const double epsilon = 1e-12;
bool close(double actual, double expected) { return std::abs(actual - expected) <= epsilon; }
}

TEST_CASE("CurveExtras defaults match the pinned scales and type", "[curveextras]") {
    REQUIRE(HeartCurve().scale == 5.);
    REQUIRE(VivianiCurve().scale == 70.);
    REQUIRE(HeartCurve(2.5).scale == 2.5);
    REQUIRE(TrefoilKnot().scale == 10.);
    REQUIRE(TorusKnot().scale == 10.);
    REQUIRE(CinquefoilKnot().scale == 10.);
    REQUIRE(TrefoilPolynomialKnot().scale == 10.);
    REQUIRE(FigureEightPolynomialKnot().scale == 1.);
    REQUIRE(DecoratedTorusKnot4a().scale == 40.);
    REQUIRE(DecoratedTorusKnot4b().scale == 40.);
    REQUIRE(DecoratedTorusKnot5a().scale == 40.);
    REQUIRE(DecoratedTorusKnot5c().scale == 40.);
    // The upstream classes do not override `type`, so Curve's value stays in place.
    REQUIRE(GrannyKnot().type == "Curve");
    REQUIRE((std::is_base_of_v<Curve<Vector3>, GrannyKnot>));
    REQUIRE((std::is_base_of_v<Curve<Vector3>, KnotCurve>));
    REQUIRE((std::is_base_of_v<Curve<Vector3>, HelixCurve>));
    REQUIRE((std::is_base_of_v<Curve<Vector3>, FigureEightPolynomialKnot>));
}

TEST_CASE("CurveExtras samples match the pinned formulas", "[curveextras]") {
    Vector3 point;
    GrannyKnot granny;
    granny.getPoint(0, point);
    REQUIRE(close(point.x, -13.200000000000001));
    REQUIRE(close(point.y, 5.6000000000000005));
    REQUIRE(close(point.z, 14.));
    granny.getPoint(0.25, point);
    REQUIRE(close(point.x, -10.));
    REQUIRE(close(point.y, 9.599999999999998));
    REQUIRE(close(point.z, 7.999999999999998));

    VivianiCurve viviani;
    viviani.getPoint(0, point);
    REQUIRE(close(point.x, 70.));
    REQUIRE(close(point.y, 0.));
    REQUIRE(close(point.z, 0.));
    viviani.getPoint(0.25, point);
    REQUIRE(close(point.x, 0.));
    REQUIRE(close(point.z, 70.));

    KnotCurve knot;
    knot.getPoint(0, point);
    REQUIRE(close(point.x, 0.));
    REQUIRE(close(point.y, 60.));
    knot.getPoint(0.25, point);
    REQUIRE(close(point.x, 50.));
    REQUIRE(close(point.y, 6.123233995736768e-16));
    REQUIRE(close(point.z, 10.000000000000004));

    HelixCurve helix;
    helix.getPoint(0.5, point);
    REQUIRE(close(point.x, -30.));
    REQUIRE(close(point.y, 1.8369701987210297e-14));
    REQUIRE(close(point.z, 75.));

    TrefoilKnot trefoil;
    trefoil.getPoint(0, point);
    REQUIRE(close(point.x, 30.));
    REQUIRE(close(point.y, 0.));
    trefoil.getPoint(0.25, point);
    REQUIRE(close(point.x, -19.999999999999996));
    REQUIRE(close(point.z, -10.));

    TrefoilPolynomialKnot polynomial;
    polynomial.getPoint(0, point);
    REQUIRE(close(point.x, -20.));
    REQUIRE(close(point.z, -24.000000000000004));
    polynomial.getPoint(0.25, point);
    REQUIRE(close(point.x, 20.));
    REQUIRE(close(point.y, -30.));
    REQUIRE(close(point.z, 18.));
    polynomial.getPoint(0.5, point);
    REQUIRE(point.x == 0.);
    REQUIRE(point.y == 0.);
    REQUIRE(point.z == 0.);

    FigureEightPolynomialKnot eight;
    eight.getPoint(0, point);
    REQUIRE(close(point.x, -86.4));
    REQUIRE(close(point.y, 48.));
    REQUIRE(close(point.z, -134.40000000000003));

    DecoratedTorusKnot5c decorated;
    decorated.getPoint(0.5, point);
    REQUIRE(close(point.x, 28.));
    REQUIRE(close(point.z, 7.545557428530142e-14));
}

TEST_CASE("CurveExtras scales the sampled point", "[curveextras]") {
    Vector3 single, scaled;
    HeartCurve(1).getPoint(0.25, single);
    HeartCurve(2.5).getPoint(0.25, scaled);
    REQUIRE(close(single.x, 16.));
    REQUIRE(close(single.y, 4.000000000000001));
    REQUIRE(close(scaled.x, 40.));
    REQUIRE(close(scaled.y, 10.000000000000002));
    Vector3 base, ribbon;
    TrefoilKnot(1).getPoint(0.25, base);
    TrefoilKnot(10).getPoint(0.25, ribbon);
    REQUIRE(close(ribbon.x, base.x * 10));
    REQUIRE(close(ribbon.z, base.z * 10));
}

TEST_CASE("CurveExtras tangents and arc length match the pinned curve", "[curveextras]") {
    TrefoilKnot trefoil;
    Vector3 tangent;
    trefoil.getTangent(0.25, tangent);
    REQUIRE(close(tangent.x, -0.5999995704746764));
    REQUIRE(close(tangent.y, -0.8000003221438127));
    REQUIRE(close(tangent.z, 0.));
    GrannyKnot granny;
    granny.arcLengthDivisions = 8;
    REQUIRE(close(granny.getLength(), 297.2210460621086));
    HeartCurve heart;
    heart.arcLengthDivisions = 8;
    REQUIRE(close(heart.getLength(), 479.3737047499778));
    FigureEightPolynomialKnot eight;
    eight.arcLengthDivisions = 8;
    REQUIRE(close(eight.getLength(), 542.0593609561188));
}

TEST_CASE("JSNumberMath pow mirrors the pinned Node Math.pow", "[curveextras]") {
    // The pinned Node release enables V8's --use-std-math-pow default, whose wrapper is
    // reproduced in src/math/JSNumberMath.cpp.
    REQUIRE(JSNumberMath::pow(0., 0.) == 1.);
    REQUIRE(JSNumberMath::pow(-2., 2.) == 4.);
    REQUIRE(JSNumberMath::pow(1.000093339211477, 2.) == 1.000093339211477 * 1.000093339211477);
    const double nan = std::numeric_limits<double>::quiet_NaN();
    const double inf = std::numeric_limits<double>::infinity();
    REQUIRE(std::isnan(JSNumberMath::pow(2., nan)));
    REQUIRE(std::isnan(JSNumberMath::pow(1., inf)));
    REQUIRE(std::isnan(JSNumberMath::pow(-1., -inf)));
    REQUIRE(JSNumberMath::pow(2., inf) == inf);
    REQUIRE(JSNumberMath::pow(0.5, inf) == 0.);
    REQUIRE(JSNumberMath::pow(-0., 0.5) == 0.);
    REQUIRE(JSNumberMath::pow(-inf, 0.5) == inf);
    REQUIRE(JSNumberMath::pow(1.7976931348623157e308, 0.5) == std::sqrt(1.7976931348623157e308));
    REQUIRE(JSNumberMath::pow(0.9999999999999999, 0.5) == std::sqrt(0.9999999999999999));
}
