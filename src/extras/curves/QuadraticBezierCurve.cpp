#include "extras/curves/QuadraticBezierCurve.h"
#include <utility>

QuadraticBezierCurve::QuadraticBezierCurve(std::shared_ptr<Vector2> v0, std::shared_ptr<Vector2> v1, std::shared_ptr<Vector2> v2)
    : v0(std::move(v0)), v1(std::move(v1)), v2(std::move(v2)) {
    type = "QuadraticBezierCurve";
}

Vector2& QuadraticBezierCurve::getPoint(double t, Vector2& target) {
    if (!v0 || !v1 || !v2)
        throw std::invalid_argument("QuadraticBezierCurve: control points must be vectors");
    target.set(
        Curve_detail::quadraticBezier(t, v0->x, v1->x, v2->x),
        Curve_detail::quadraticBezier(t, v0->y, v1->y, v2->y));
    return target;
}

std::shared_ptr<Curve<Vector2>> QuadraticBezierCurve::clone() const {
    auto result = std::make_shared<QuadraticBezierCurve>();
    result->copy(*this);
    return result;
}
QuadraticBezierCurve& QuadraticBezierCurve::copy(const Curve<Vector2>& source) {
    Curve<Vector2>::copy(source);
    const auto& curve = dynamic_cast<const QuadraticBezierCurve&>(source);
    if (!v0 || !v1 || !v2)
        throw std::invalid_argument("QuadraticBezierCurve: control points must be vectors");
    if (!curve.v0) throw std::invalid_argument("QuadraticBezierCurve: source control point is null");
    v0->copy(*curve.v0);
    if (!curve.v1) throw std::invalid_argument("QuadraticBezierCurve: source control point is null");
    v1->copy(*curve.v1);
    if (!curve.v2) throw std::invalid_argument("QuadraticBezierCurve: source control point is null");
    v2->copy(*curve.v2);
    return *this;
}
JsonValue QuadraticBezierCurve::toJSON() const {
    if (!v0 || !v1 || !v2)
        throw std::invalid_argument("QuadraticBezierCurve: control points must be vectors");
    auto data = Curve<Vector2>::toJSON();
    data["v0"] = Curve_detail::vectorJSON(*v0);
    data["v1"] = Curve_detail::vectorJSON(*v1);
    data["v2"] = Curve_detail::vectorJSON(*v2);
    return data;
}
QuadraticBezierCurve& QuadraticBezierCurve::fromJSON(const JsonValue& json) {
    Curve<Vector2>::fromJSON(json);
    if (!v0 || !v1 || !v2)
        throw std::invalid_argument("QuadraticBezierCurve: control points must be vectors");
    Curve_detail::vectorFromJSON(*v0, json.at("v0"));
    Curve_detail::vectorFromJSON(*v1, json.at("v1"));
    Curve_detail::vectorFromJSON(*v2, json.at("v2"));
    return *this;
}
