#include "extras/curves/EllipseCurve.h"

EllipseCurve::EllipseCurve(double aX, double aY, double xRadius, double yRadius,
                           double aStartAngle, double aEndAngle, bool aClockwise, double aRotation)
    : aX(aX), aY(aY), xRadius(xRadius), yRadius(yRadius), aStartAngle(aStartAngle), aEndAngle(aEndAngle),
      aClockwise(aClockwise), aRotation(aRotation) { type = "EllipseCurve"; }

Vector2& EllipseCurve::getPoint(double t, Vector2& target) {
    const double twoPi = std::numbers::pi * 2;
    double deltaAngle = aEndAngle - aStartAngle;
    const bool samePoints = std::abs(deltaAngle) < std::numeric_limits<double>::epsilon();
    while (deltaAngle < 0) deltaAngle += twoPi;
    while (deltaAngle > twoPi) deltaAngle -= twoPi;
    if (deltaAngle < std::numeric_limits<double>::epsilon()) deltaAngle = samePoints ? 0 : twoPi;
    if (aClockwise == true && !samePoints) {
        if (deltaAngle == twoPi) deltaAngle = -twoPi;
        else deltaAngle = deltaAngle - twoPi;
    }
    const double angle = aStartAngle + t * deltaAngle;
    double x = aX + xRadius * std::cos(angle);
    double y = aY + yRadius * std::sin(angle);
    if (aRotation != 0) {
        const double cos = std::cos(aRotation), sin = std::sin(aRotation);
        const double tx = x - aX, ty = y - aY;
        x = tx * cos - ty * sin + aX;
        y = tx * sin + ty * cos + aY;
    }
    return target.set(x, y);
}
std::shared_ptr<Curve<Vector2>> EllipseCurve::clone() const {
    auto result = std::make_shared<EllipseCurve>(); result->copy(*this); return result;
}
EllipseCurve& EllipseCurve::copy(const Curve<Vector2>& source) {
    Curve<Vector2>::copy(source);
    const auto& curve = dynamic_cast<const EllipseCurve&>(source);
    aX = curve.aX; aY = curve.aY;
    xRadius = curve.xRadius; yRadius = curve.yRadius;
    aStartAngle = curve.aStartAngle; aEndAngle = curve.aEndAngle;
    aClockwise = curve.aClockwise; aRotation = curve.aRotation;
    return *this;
}
JsonValue EllipseCurve::toJSON() const {
    auto data = Curve<Vector2>::toJSON();
    data["aX"] = aX; data["aY"] = aY;
    data["xRadius"] = xRadius; data["yRadius"] = yRadius;
    data["aStartAngle"] = aStartAngle; data["aEndAngle"] = aEndAngle;
    data["aClockwise"] = aClockwise; data["aRotation"] = aRotation;
    return data;
}
EllipseCurve& EllipseCurve::fromJSON(const JsonValue& json) {
    Curve<Vector2>::fromJSON(json);
    aX = json.at("aX").get<double>(); aY = json.at("aY").get<double>();
    xRadius = json.at("xRadius").get<double>(); yRadius = json.at("yRadius").get<double>();
    aStartAngle = json.at("aStartAngle").get<double>(); aEndAngle = json.at("aEndAngle").get<double>();
    aClockwise = json.at("aClockwise").get<bool>(); aRotation = json.at("aRotation").get<double>();
    return *this;
}
