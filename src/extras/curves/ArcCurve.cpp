#include "extras/curves/ArcCurve.h"

ArcCurve::ArcCurve(double aX, double aY, double aRadius, double aStartAngle, double aEndAngle, bool aClockwise)
    : EllipseCurve(aX, aY, aRadius, aRadius, aStartAngle, aEndAngle, aClockwise) { type = "ArcCurve"; }
std::shared_ptr<Curve<Vector2>> ArcCurve::clone() const {
    auto result = std::make_shared<ArcCurve>(); result->copy(*this); return result;
}
