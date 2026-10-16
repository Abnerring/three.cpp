#include "curves/CurveExtras.h"
#include "math/JSNumberMath.h"

// Every formula below is the pinned source expression with JSNumberMath::sin/cos/pow
// standing in for Math.sin/Math.cos/Math.pow and the same multiplication order; `t` is
// reassigned exactly where the upstream method reassigns it.
namespace {
// Math.PI as a binary64 literal, so the module does not depend on PI being defined.
constexpr double PI = 3.141592653589793;
// The module-local helper of the pinned source: scaleTo( x, y, t ).
double scaleTo(double x, double y, double t) {
    const double r = y - x;
    return t * r + x;
}
}

Vector3& GrannyKnot::getPoint(double t, Vector3& optionalTarget) {
    Vector3& point = optionalTarget;
    t = 2 * PI * t;
    const double x = -0.22 * JSNumberMath::cos(t) - 1.28 * JSNumberMath::sin(t) -
                     0.44 * JSNumberMath::cos(3 * t) - 0.78 * JSNumberMath::sin(3 * t);
    const double y = -0.1 * JSNumberMath::cos(2 * t) - 0.27 * JSNumberMath::sin(2 * t) +
                     0.38 * JSNumberMath::cos(4 * t) + 0.46 * JSNumberMath::sin(4 * t);
    const double z = 0.7 * JSNumberMath::cos(3 * t) - 0.4 * JSNumberMath::sin(3 * t);
    return point.set(x, y, z).multiplyScalar(20);
}

HeartCurve::HeartCurve(double scale) : scale(scale) {}

Vector3& HeartCurve::getPoint(double t, Vector3& optionalTarget) {
    Vector3& point = optionalTarget;
    t *= 2 * PI;
    const double x = 16 * JSNumberMath::pow(JSNumberMath::sin(t), 3);
    const double y = 13 * JSNumberMath::cos(t) - 5 * JSNumberMath::cos(2 * t) - 2 * JSNumberMath::cos(3 * t) -
                     JSNumberMath::cos(4 * t);
    const double z = 0;
    return point.set(x, y, z).multiplyScalar(scale);
}

VivianiCurve::VivianiCurve(double scale) : scale(scale) {}

Vector3& VivianiCurve::getPoint(double t, Vector3& optionalTarget) {
    Vector3& point = optionalTarget;
    t = t * 4 * PI; // normalized to 0..1
    const double a = scale / 2;
    const double x = a * (1 + JSNumberMath::cos(t));
    const double y = a * JSNumberMath::sin(t);
    const double z = 2 * a * JSNumberMath::sin(t / 2);
    return point.set(x, y, z);
}

Vector3& KnotCurve::getPoint(double t, Vector3& optionalTarget) {
    Vector3& point = optionalTarget;
    t *= 2 * PI;
    const double R = 10;
    const double s = 50;
    const double x = s * JSNumberMath::sin(t);
    const double y = JSNumberMath::cos(t) * (R + s * JSNumberMath::cos(t));
    const double z = JSNumberMath::sin(t) * (R + s * JSNumberMath::cos(t));
    return point.set(x, y, z);
}

Vector3& HelixCurve::getPoint(double t, Vector3& optionalTarget) {
    Vector3& point = optionalTarget;
    const double a = 30;  // radius
    const double b = 150; // height
    const double t2 = 2 * PI * t * b / 30;
    const double x = JSNumberMath::cos(t2) * a;
    const double y = JSNumberMath::sin(t2) * a;
    const double z = b * t;
    return point.set(x, y, z);
}

TrefoilKnot::TrefoilKnot(double scale) : scale(scale) {}

Vector3& TrefoilKnot::getPoint(double t, Vector3& optionalTarget) {
    Vector3& point = optionalTarget;
    t *= PI * 2;
    const double x = (2 + JSNumberMath::cos(3 * t)) * JSNumberMath::cos(2 * t);
    const double y = (2 + JSNumberMath::cos(3 * t)) * JSNumberMath::sin(2 * t);
    const double z = JSNumberMath::sin(3 * t);
    return point.set(x, y, z).multiplyScalar(scale);
}

TorusKnot::TorusKnot(double scale) : scale(scale) {}

Vector3& TorusKnot::getPoint(double t, Vector3& optionalTarget) {
    Vector3& point = optionalTarget;
    const double p = 3;
    const double q = 4;
    t *= PI * 2;
    const double x = (2 + JSNumberMath::cos(q * t)) * JSNumberMath::cos(p * t);
    const double y = (2 + JSNumberMath::cos(q * t)) * JSNumberMath::sin(p * t);
    const double z = JSNumberMath::sin(q * t);
    return point.set(x, y, z).multiplyScalar(scale);
}

CinquefoilKnot::CinquefoilKnot(double scale) : scale(scale) {}

Vector3& CinquefoilKnot::getPoint(double t, Vector3& optionalTarget) {
    Vector3& point = optionalTarget;
    const double p = 2;
    const double q = 5;
    t *= PI * 2;
    const double x = (2 + JSNumberMath::cos(q * t)) * JSNumberMath::cos(p * t);
    const double y = (2 + JSNumberMath::cos(q * t)) * JSNumberMath::sin(p * t);
    const double z = JSNumberMath::sin(q * t);
    return point.set(x, y, z).multiplyScalar(scale);
}

TrefoilPolynomialKnot::TrefoilPolynomialKnot(double scale) : scale(scale) {}

Vector3& TrefoilPolynomialKnot::getPoint(double t, Vector3& optionalTarget) {
    Vector3& point = optionalTarget;
    t = t * 4 - 2;
    const double x = JSNumberMath::pow(t, 3) - 3 * t;
    const double y = JSNumberMath::pow(t, 4) - 4 * t * t;
    const double z = 1.0 / 5 * JSNumberMath::pow(t, 5) - 2 * t;
    return point.set(x, y, z).multiplyScalar(scale);
}

FigureEightPolynomialKnot::FigureEightPolynomialKnot(double scale) : scale(scale) {}

Vector3& FigureEightPolynomialKnot::getPoint(double t, Vector3& optionalTarget) {
    Vector3& point = optionalTarget;
    t = scaleTo(-4, 4, t);
    const double x = 2.0 / 5 * t * (t * t - 7) * (t * t - 10);
    const double y = JSNumberMath::pow(t, 4) - 13 * t * t;
    const double z = 1.0 / 10 * t * (t * t - 4) * (t * t - 9) * (t * t - 12);
    return point.set(x, y, z).multiplyScalar(scale);
}

DecoratedTorusKnot4a::DecoratedTorusKnot4a(double scale) : scale(scale) {}

Vector3& DecoratedTorusKnot4a::getPoint(double t, Vector3& optionalTarget) {
    Vector3& point = optionalTarget;
    t *= PI * 2;
    const double x = JSNumberMath::cos(2 * t) *
                     (1 + 0.6 * (JSNumberMath::cos(5 * t) + 0.75 * JSNumberMath::cos(10 * t)));
    const double y = JSNumberMath::sin(2 * t) *
                     (1 + 0.6 * (JSNumberMath::cos(5 * t) + 0.75 * JSNumberMath::cos(10 * t)));
    const double z = 0.35 * JSNumberMath::sin(5 * t);
    return point.set(x, y, z).multiplyScalar(scale);
}

DecoratedTorusKnot4b::DecoratedTorusKnot4b(double scale) : scale(scale) {}

Vector3& DecoratedTorusKnot4b::getPoint(double t, Vector3& optionalTarget) {
    Vector3& point = optionalTarget;
    const double fi = t * PI * 2;
    const double x = JSNumberMath::cos(2 * fi) *
                     (1 + 0.45 * JSNumberMath::cos(3 * fi) + 0.4 * JSNumberMath::cos(9 * fi));
    const double y = JSNumberMath::sin(2 * fi) *
                     (1 + 0.45 * JSNumberMath::cos(3 * fi) + 0.4 * JSNumberMath::cos(9 * fi));
    const double z = 0.2 * JSNumberMath::sin(9 * fi);
    return point.set(x, y, z).multiplyScalar(scale);
}

DecoratedTorusKnot5a::DecoratedTorusKnot5a(double scale) : scale(scale) {}

Vector3& DecoratedTorusKnot5a::getPoint(double t, Vector3& optionalTarget) {
    Vector3& point = optionalTarget;
    const double fi = t * PI * 2;
    const double x = JSNumberMath::cos(3 * fi) *
                     (1 + 0.3 * JSNumberMath::cos(5 * fi) + 0.5 * JSNumberMath::cos(10 * fi));
    const double y = JSNumberMath::sin(3 * fi) *
                     (1 + 0.3 * JSNumberMath::cos(5 * fi) + 0.5 * JSNumberMath::cos(10 * fi));
    const double z = 0.2 * JSNumberMath::sin(20 * fi);
    return point.set(x, y, z).multiplyScalar(scale);
}

DecoratedTorusKnot5c::DecoratedTorusKnot5c(double scale) : scale(scale) {}

Vector3& DecoratedTorusKnot5c::getPoint(double t, Vector3& optionalTarget) {
    Vector3& point = optionalTarget;
    const double fi = t * PI * 2;
    const double x = JSNumberMath::cos(4 * fi) *
                     (1 + 0.5 * (JSNumberMath::cos(5 * fi) + 0.4 * JSNumberMath::cos(20 * fi)));
    const double y = JSNumberMath::sin(4 * fi) *
                     (1 + 0.5 * (JSNumberMath::cos(5 * fi) + 0.4 * JSNumberMath::cos(20 * fi)));
    const double z = 0.35 * JSNumberMath::sin(15 * fi);
    return point.set(x, y, z).multiplyScalar(scale);
}
