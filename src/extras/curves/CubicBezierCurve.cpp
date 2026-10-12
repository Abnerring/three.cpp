#include "extras/curves/CubicBezierCurve.h"
#include <utility>

CubicBezierCurve::CubicBezierCurve(std::shared_ptr<Vector2> v0, std::shared_ptr<Vector2> v1, std::shared_ptr<Vector2> v2, std::shared_ptr<Vector2> v3)
    : v0(std::move(v0)), v1(std::move(v1)), v2(std::move(v2)), v3(std::move(v3)) {
    type = "CubicBezierCurve";
}

Vector2& CubicBezierCurve::getPoint(double t, Vector2& target) {
    if (!v0 || !v1 || !v2 || !v3)
        throw std::invalid_argument("CubicBezierCurve: control points must be vectors");
    target.set(
        Curve_detail::cubicBezier(t, v0->x, v1->x, v2->x, v3->x),
        Curve_detail::cubicBezier(t, v0->y, v1->y, v2->y, v3->y));
    return target;
}

std::shared_ptr<Curve<Vector2>> CubicBezierCurve::clone() const {
    auto result = std::make_shared<CubicBezierCurve>();
    result->copy(*this);
    return result;
}
CubicBezierCurve& CubicBezierCurve::copy(const Curve<Vector2>& source) {
    Curve<Vector2>::copy(source);
    const auto& curve = dynamic_cast<const CubicBezierCurve&>(source);
    if (!v0 || !v1 || !v2 || !v3)
        throw std::invalid_argument("CubicBezierCurve: control points must be vectors");
    if (!curve.v0) throw std::invalid_argument("CubicBezierCurve: source control point is null");
    v0->copy(*curve.v0);
    if (!curve.v1) throw std::invalid_argument("CubicBezierCurve: source control point is null");
    v1->copy(*curve.v1);
    if (!curve.v2) throw std::invalid_argument("CubicBezierCurve: source control point is null");
    v2->copy(*curve.v2);
    if (!curve.v3) throw std::invalid_argument("CubicBezierCurve: source control point is null");
    v3->copy(*curve.v3);
    return *this;
}
JsonValue CubicBezierCurve::toJSON() const {
    if (!v0 || !v1 || !v2 || !v3)
        throw std::invalid_argument("CubicBezierCurve: control points must be vectors");
    auto data = Curve<Vector2>::toJSON();
    data["v0"] = Curve_detail::vectorJSON(*v0);
    data["v1"] = Curve_detail::vectorJSON(*v1);
    data["v2"] = Curve_detail::vectorJSON(*v2);
    data["v3"] = Curve_detail::vectorJSON(*v3);
    return data;
}
CubicBezierCurve& CubicBezierCurve::fromJSON(const JsonValue& json) {
    Curve<Vector2>::fromJSON(json);
    if (!v0 || !v1 || !v2 || !v3)
        throw std::invalid_argument("CubicBezierCurve: control points must be vectors");
    Curve_detail::vectorFromJSON(*v0, json.at("v0"));
    Curve_detail::vectorFromJSON(*v1, json.at("v1"));
    Curve_detail::vectorFromJSON(*v2, json.at("v2"));
    Curve_detail::vectorFromJSON(*v3, json.at("v3"));
    return *this;
}
