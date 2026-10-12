#include "extras/curves/QuadraticBezierCurve3.h"
#include <utility>

QuadraticBezierCurve3::QuadraticBezierCurve3(std::shared_ptr<Vector3> v0, std::shared_ptr<Vector3> v1, std::shared_ptr<Vector3> v2)
    : v0(std::move(v0)), v1(std::move(v1)), v2(std::move(v2)) {
    type = "QuadraticBezierCurve3";
}

Vector3& QuadraticBezierCurve3::getPoint(double t, Vector3& target) {
    if (!v0 || !v1 || !v2)
        throw std::invalid_argument("QuadraticBezierCurve3: control points must be vectors");
    target.set(
        Curve_detail::quadraticBezier(t, v0->x, v1->x, v2->x),
        Curve_detail::quadraticBezier(t, v0->y, v1->y, v2->y),
        Curve_detail::quadraticBezier(t, v0->z, v1->z, v2->z));
    return target;
}

std::shared_ptr<Curve<Vector3>> QuadraticBezierCurve3::clone() const {
    auto result = std::make_shared<QuadraticBezierCurve3>();
    result->copy(*this);
    return result;
}
QuadraticBezierCurve3& QuadraticBezierCurve3::copy(const Curve<Vector3>& source) {
    Curve<Vector3>::copy(source);
    const auto& curve = dynamic_cast<const QuadraticBezierCurve3&>(source);
    if (!v0 || !v1 || !v2)
        throw std::invalid_argument("QuadraticBezierCurve3: control points must be vectors");
    if (!curve.v0) throw std::invalid_argument("QuadraticBezierCurve3: source control point is null");
    v0->copy(*curve.v0);
    if (!curve.v1) throw std::invalid_argument("QuadraticBezierCurve3: source control point is null");
    v1->copy(*curve.v1);
    if (!curve.v2) throw std::invalid_argument("QuadraticBezierCurve3: source control point is null");
    v2->copy(*curve.v2);
    return *this;
}
JsonValue QuadraticBezierCurve3::toJSON() const {
    if (!v0 || !v1 || !v2)
        throw std::invalid_argument("QuadraticBezierCurve3: control points must be vectors");
    auto data = Curve<Vector3>::toJSON();
    data["v0"] = Curve_detail::vectorJSON(*v0);
    data["v1"] = Curve_detail::vectorJSON(*v1);
    data["v2"] = Curve_detail::vectorJSON(*v2);
    return data;
}
QuadraticBezierCurve3& QuadraticBezierCurve3::fromJSON(const JsonValue& json) {
    Curve<Vector3>::fromJSON(json);
    if (!v0 || !v1 || !v2)
        throw std::invalid_argument("QuadraticBezierCurve3: control points must be vectors");
    Curve_detail::vectorFromJSON(*v0, json.at("v0"));
    Curve_detail::vectorFromJSON(*v1, json.at("v1"));
    Curve_detail::vectorFromJSON(*v2, json.at("v2"));
    return *this;
}
