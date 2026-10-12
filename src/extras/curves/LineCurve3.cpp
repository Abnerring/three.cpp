#include "extras/curves/LineCurve3.h"
#include <utility>

LineCurve3::LineCurve3(std::shared_ptr<Vector3> v1, std::shared_ptr<Vector3> v2)
    : v1(std::move(v1)), v2(std::move(v2)) {
    type = "LineCurve3";
}

Vector3& LineCurve3::getPoint(double t, Vector3& target) {
    if (!v1 || !v2)
        throw std::invalid_argument("LineCurve3: control points must be vectors");
    if (t == 1) target.copy(*v2);
    else {
        target.copy(*v2).sub(*v1);
        target.multiplyScalar(t).add(*v1);
    }
    return target;
}

Vector3& LineCurve3::getPointAt(double u, Vector3& target) {
    return getPoint(u, target);
}
Vector3& LineCurve3::getTangent(double, Vector3& target) {
    if (!v1 || !v2)
        throw std::invalid_argument("LineCurve3: control points must be vectors");
    return target.subVectors(*v2, *v1).normalize();
}
Vector3& LineCurve3::getTangentAt(double u, Vector3& target) {
    return getTangent(u, target);
}

std::shared_ptr<Curve<Vector3>> LineCurve3::clone() const {
    auto result = std::make_shared<LineCurve3>();
    result->copy(*this);
    return result;
}
LineCurve3& LineCurve3::copy(const Curve<Vector3>& source) {
    Curve<Vector3>::copy(source);
    const auto& curve = dynamic_cast<const LineCurve3&>(source);
    if (!v1 || !v2)
        throw std::invalid_argument("LineCurve3: control points must be vectors");
    if (!curve.v1) throw std::invalid_argument("LineCurve3: source control point is null");
    v1->copy(*curve.v1);
    if (!curve.v2) throw std::invalid_argument("LineCurve3: source control point is null");
    v2->copy(*curve.v2);
    return *this;
}
JsonValue LineCurve3::toJSON() const {
    if (!v1 || !v2)
        throw std::invalid_argument("LineCurve3: control points must be vectors");
    auto data = Curve<Vector3>::toJSON();
    data["v1"] = Curve_detail::vectorJSON(*v1);
    data["v2"] = Curve_detail::vectorJSON(*v2);
    return data;
}
LineCurve3& LineCurve3::fromJSON(const JsonValue& json) {
    Curve<Vector3>::fromJSON(json);
    if (!v1 || !v2)
        throw std::invalid_argument("LineCurve3: control points must be vectors");
    Curve_detail::vectorFromJSON(*v1, json.at("v1"));
    Curve_detail::vectorFromJSON(*v2, json.at("v2"));
    return *this;
}
