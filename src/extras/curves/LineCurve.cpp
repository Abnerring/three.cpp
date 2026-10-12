#include "extras/curves/LineCurve.h"
#include <utility>

LineCurve::LineCurve(std::shared_ptr<Vector2> v1, std::shared_ptr<Vector2> v2)
    : v1(std::move(v1)), v2(std::move(v2)) {
    type = "LineCurve";
}

Vector2& LineCurve::getPoint(double t, Vector2& target) {
    if (!v1 || !v2)
        throw std::invalid_argument("LineCurve: control points must be vectors");
    if (t == 1) target.copy(*v2);
    else {
        target.copy(*v2).sub(*v1);
        target.multiplyScalar(t).add(*v1);
    }
    return target;
}

Vector2& LineCurve::getPointAt(double u, Vector2& target) {
    return getPoint(u, target);
}
Vector2& LineCurve::getTangent(double, Vector2& target) {
    if (!v1 || !v2)
        throw std::invalid_argument("LineCurve: control points must be vectors");
    return target.subVectors(*v2, *v1).normalize();
}
Vector2& LineCurve::getTangentAt(double u, Vector2& target) {
    return getTangent(u, target);
}

std::shared_ptr<Curve<Vector2>> LineCurve::clone() const {
    auto result = std::make_shared<LineCurve>();
    result->copy(*this);
    return result;
}
LineCurve& LineCurve::copy(const Curve<Vector2>& source) {
    Curve<Vector2>::copy(source);
    const auto& curve = dynamic_cast<const LineCurve&>(source);
    if (!v1 || !v2)
        throw std::invalid_argument("LineCurve: control points must be vectors");
    if (!curve.v1) throw std::invalid_argument("LineCurve: source control point is null");
    v1->copy(*curve.v1);
    if (!curve.v2) throw std::invalid_argument("LineCurve: source control point is null");
    v2->copy(*curve.v2);
    return *this;
}
JsonValue LineCurve::toJSON() const {
    if (!v1 || !v2)
        throw std::invalid_argument("LineCurve: control points must be vectors");
    auto data = Curve<Vector2>::toJSON();
    data["v1"] = Curve_detail::vectorJSON(*v1);
    data["v2"] = Curve_detail::vectorJSON(*v2);
    return data;
}
LineCurve& LineCurve::fromJSON(const JsonValue& json) {
    Curve<Vector2>::fromJSON(json);
    if (!v1 || !v2)
        throw std::invalid_argument("LineCurve: control points must be vectors");
    Curve_detail::vectorFromJSON(*v1, json.at("v1"));
    Curve_detail::vectorFromJSON(*v2, json.at("v2"));
    return *this;
}
