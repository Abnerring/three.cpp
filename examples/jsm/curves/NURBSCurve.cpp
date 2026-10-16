#include "curves/NURBSCurve.h"
#include "core/JSValue.h"
#include <cmath>
#include <cstddef>
#include <limits>

namespace {
// `knots[ index ]` for a JavaScript array: a non-integral or out-of-range index is
// `undefined`, which the linear mapping and the point lookup carry as NaN.
double knotAt(const std::vector<double>& knots, double index) {
    if (!std::isfinite(index) || index < 0 || std::trunc(index) != index || index >= double(knots.size()))
        return std::numeric_limits<double>::quiet_NaN();
    return knots[std::size_t(index)];
}
}

NURBSCurve::NURBSCurve(double degree, std::vector<double> knots, std::vector<Vector4> controlPoints,
                       JSValue startKnot, JSValue endKnot)
    : degree(degree), knots(std::move(knots)), controlPoints(std::move(controlPoints)) {
    // `knots ? knots.length - 1 : 0`; a missing knot vector is not representable here,
    // so the native constructor always receives one.
    const double knotsLength = double(this->knots.size()) - 1;
    // `startKnot || 0` and `endKnot || knotsLength` tested by truthiness.
    this->startKnot = startKnot.toBoolean() ? startKnot.toNumber() : 0;
    this->endKnot = endKnot.toBoolean() ? endKnot.toNumber() : knotsLength;
}

Vector3& NURBSCurve::getPoint(double t, Vector3& optionalTarget) {
    Vector3& point = optionalTarget;
    // linear mapping t -> u
    const double start = knotAt(knots, startKnot);
    const double end = knotAt(knots, endKnot);
    const double u = start + t * (end - start);
    // following results in ( wx, wy, wz, w ) homogeneous point
    Vector4 hpoint = NURBSUtils::calcBSplinePoint(degree, knots, controlPoints, u);
    if (hpoint.w != 1.) hpoint.divideScalar(hpoint.w);
    return point.set(hpoint.x, hpoint.y, hpoint.z);
}

Vector3& NURBSCurve::getTangent(double t, Vector3& optionalTarget) {
    Vector3& tangent = optionalTarget;
    const double first = knotAt(knots, 0);
    const double last = knotAt(knots, double(knots.size()) - 1);
    const double u = first + t * (last - first);
    const std::vector<Vector3> ders = NURBSUtils::calcNURBSDerivatives(degree, knots, controlPoints, u, 1);
    if (ders.size() < 2) throw JSTypeError("NURBSCurve.getTangent: derivative entry is undefined");
    tangent.copy(ders[1]).normalize();
    return tangent;
}

std::shared_ptr<Curve<Vector3>> NURBSCurve::clone() const {
    auto result = std::make_shared<NURBSCurve>();
    result->copy(*this);
    return result;
}

NURBSCurve& NURBSCurve::copy(const Curve<Vector3>& source) {
    Curve<Vector3>::copy(source);
    const auto& curve = dynamic_cast<const NURBSCurve&>(source);
    degree = curve.degree;
    knots = curve.knots;
    controlPoints.clear();
    controlPoints.reserve(curve.controlPoints.size());
    for (const auto& point : curve.controlPoints) controlPoints.push_back(point.clone());
    startKnot = curve.startKnot;
    endKnot = curve.endKnot;
    return *this;
}

JsonValue NURBSCurve::toJSON() const {
    auto data = Curve<Vector3>::toJSON();
    JsonValue::Array knotValues;
    for (const auto value : knots) knotValues.emplace_back(value);
    JsonValue::Array points;
    for (const auto& point : controlPoints) {
        JsonValue::Array components;
        for (const auto value : point.toArray()) components.emplace_back(value);
        points.push_back(std::move(components));
    }
    data["degree"] = degree;
    data["knots"] = std::move(knotValues);
    data["controlPoints"] = std::move(points);
    data["startKnot"] = startKnot;
    data["endKnot"] = endKnot;
    return data;
}

NURBSCurve& NURBSCurve::fromJSON(const JsonValue& json) {
    Curve<Vector3>::fromJSON(json);
    degree = json.at("degree").get<double>();
    knots.clear();
    for (const auto& entry : json.at("knots").get<JsonValue::Array>()) knots.push_back(entry.get<double>());
    controlPoints.clear();
    for (const auto& entry : json.at("controlPoints").get<JsonValue::Array>()) {
        const auto& components = entry.get<JsonValue::Array>();
        controlPoints.emplace_back(components.size() > 0 ? components[0].get<double>() : 0.,
            components.size() > 1 ? components[1].get<double>() : 0.,
            components.size() > 2 ? components[2].get<double>() : 0.,
            components.size() > 3 ? components[3].get<double>() : 1.);
    }
    startKnot = json.at("startKnot").get<double>();
    endKnot = json.at("endKnot").get<double>();
    return *this;
}
