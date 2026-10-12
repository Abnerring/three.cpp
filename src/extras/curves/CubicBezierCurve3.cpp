#include "extras/curves/CubicBezierCurve3.h"
#include <utility>

CubicBezierCurve3::CubicBezierCurve3(std::shared_ptr<Vector3> v0, std::shared_ptr<Vector3> v1, std::shared_ptr<Vector3> v2, std::shared_ptr<Vector3> v3)
    : v0(std::move(v0)), v1(std::move(v1)), v2(std::move(v2)), v3(std::move(v3)) {
    type = "CubicBezierCurve3";
}

Vector3& CubicBezierCurve3::getPoint(double t, Vector3& target) {
    if (!v0 || !v1 || !v2 || !v3)
        throw std::invalid_argument("CubicBezierCurve3: control points must be vectors");
    target.set(
        Curve_detail::cubicBezier(t, v0->x, v1->x, v2->x, v3->x),
        Curve_detail::cubicBezier(t, v0->y, v1->y, v2->y, v3->y),
        Curve_detail::cubicBezier(t, v0->z, v1->z, v2->z, v3->z));
    return target;
}

std::shared_ptr<Curve<Vector3>> CubicBezierCurve3::clone() const {
    auto result = std::make_shared<CubicBezierCurve3>();
    result->copy(*this);
    return result;
}
CubicBezierCurve3& CubicBezierCurve3::copy(const Curve<Vector3>& source) {
    Curve<Vector3>::copy(source);
    const auto& curve = dynamic_cast<const CubicBezierCurve3&>(source);
    if (!v0 || !v1 || !v2 || !v3)
        throw std::invalid_argument("CubicBezierCurve3: control points must be vectors");
    if (!curve.v0) throw std::invalid_argument("CubicBezierCurve3: source control point is null");
    v0->copy(*curve.v0);
    if (!curve.v1) throw std::invalid_argument("CubicBezierCurve3: source control point is null");
    v1->copy(*curve.v1);
    if (!curve.v2) throw std::invalid_argument("CubicBezierCurve3: source control point is null");
    v2->copy(*curve.v2);
    if (!curve.v3) throw std::invalid_argument("CubicBezierCurve3: source control point is null");
    v3->copy(*curve.v3);
    return *this;
}
JsonValue CubicBezierCurve3::toJSON() const {
    if (!v0 || !v1 || !v2 || !v3)
        throw std::invalid_argument("CubicBezierCurve3: control points must be vectors");
    auto data = Curve<Vector3>::toJSON();
    data["v0"] = Curve_detail::vectorJSON(*v0);
    data["v1"] = Curve_detail::vectorJSON(*v1);
    data["v2"] = Curve_detail::vectorJSON(*v2);
    data["v3"] = Curve_detail::vectorJSON(*v3);
    return data;
}
CubicBezierCurve3& CubicBezierCurve3::fromJSON(const JsonValue& json) {
    Curve<Vector3>::fromJSON(json);
    if (!v0 || !v1 || !v2 || !v3)
        throw std::invalid_argument("CubicBezierCurve3: control points must be vectors");
    Curve_detail::vectorFromJSON(*v0, json.at("v0"));
    Curve_detail::vectorFromJSON(*v1, json.at("v1"));
    Curve_detail::vectorFromJSON(*v2, json.at("v2"));
    Curve_detail::vectorFromJSON(*v3, json.at("v3"));
    return *this;
}
