#include "extras/curves/CatmullRomCurve3.h"
#include "core/JSValue.h"
#include <utility>

namespace {
struct CubicPoly {
    double c0 = 0, c1 = 0, c2 = 0, c3 = 0;
    void init(double x0, double x1, double t0, double t1) {
        c0 = x0; c1 = t0;
        c2 = -3 * x0 + 3 * x1 - 2 * t0 - t1;
        c3 = 2 * x0 - 2 * x1 + t0 + t1;
    }
    void initCatmullRom(double x0, double x1, double x2, double x3, double tension) {
        init(x1, x2, tension * (x2 - x0), tension * (x3 - x1));
    }
    void initNonuniformCatmullRom(double x0, double x1, double x2, double x3, double dt0, double dt1, double dt2) {
        double t1 = (x1 - x0) / dt0 - (x2 - x0) / (dt0 + dt1) + (x2 - x1) / dt1;
        double t2 = (x2 - x1) / dt1 - (x3 - x1) / (dt1 + dt2) + (x3 - x2) / dt2;
        t1 *= dt1; t2 *= dt1;
        init(x1, x2, t1, t2);
    }
    double calc(double t) const {
        const double t2 = t * t, t3 = t2 * t;
        return c0 + c1 * t + c2 * t2 + c3 * t3;
    }
};
// These variables are module state upstream, including the coefficients used
// unchanged when curveType is invalid. Each C++ thread acts as its own module.
thread_local Vector3 tmp, tmp2;
thread_local CubicPoly px, py, pz;
Vector3* lookup(const CatmullRomCurve3::PointsPtr& points, double index) {
    if (!points || !std::isfinite(index) || index < 0 || std::floor(index) != index || index >= points->size())
        return nullptr;
    const auto& point = (*points)[static_cast<std::size_t>(index)];
    return point.get();
}
Vector3& require(Vector3* point) {
    if (!point) throw JSTypeError("CatmullRomCurve3: control point is null or undefined");
    return *point;
}
Vector3& at(const CatmullRomCurve3::PointsPtr& points, double index) {
    return require(lookup(points, index));
}
}
CatmullRomCurve3::CatmullRomCurve3(PointsPtr points, bool closed, std::string curveType, double tension)
    : points(std::move(points)), closed(closed), curveType(std::move(curveType)), tension(tension) { type = "CatmullRomCurve3"; }
Vector3& CatmullRomCurve3::getPoint(double t, Vector3& target) {
    if (!points) throw JSTypeError("CatmullRomCurve3: points is null");
    const double length = static_cast<double>(points->size());
    const double p = (length - (closed ? 0 : 1)) * t;
    double intPoint = std::floor(p), weight = p - intPoint;
    if (closed) intPoint += intPoint > 0 ? 0 : (std::floor(std::abs(intPoint) / length) + 1) * length;
    else if (weight == 0 && intPoint == length - 1) { intPoint = length - 2; weight = 1; }
    Vector3* p0;
    if (closed || intPoint > 0) p0 = lookup(points, std::fmod(intPoint - 1, length));
    else { tmp2.subVectors(at(points, 0), at(points, 1)).add(at(points, 0)); p0 = &tmp2; }
    const auto p1 = lookup(points, std::fmod(intPoint, length));
    const auto p2 = lookup(points, std::fmod(intPoint + 1, length));
    Vector3* p3;
    if (closed || intPoint + 2 < length) p3 = lookup(points, std::fmod(intPoint + 2, length));
    else { tmp.subVectors(at(points, length - 1), at(points, length - 2)).add(at(points, length - 1)); p3 = &tmp; }
    if (curveType == "centripetal" || curveType == "chordal") {
        const double pow = curveType == "chordal" ? 0.5 : 0.25;
        double dt0 = std::pow(require(p0).distanceToSquared(require(p1)), pow);
        double dt1 = std::pow(require(p1).distanceToSquared(require(p2)), pow);
        double dt2 = std::pow(require(p2).distanceToSquared(require(p3)), pow);
        if (dt1 < 1e-4) dt1 = 1.0;
        if (dt0 < 1e-4) dt0 = dt1;
        if (dt2 < 1e-4) dt2 = dt1;
        px.initNonuniformCatmullRom(p0->x, p1->x, p2->x, p3->x, dt0, dt1, dt2);
        py.initNonuniformCatmullRom(p0->y, p1->y, p2->y, p3->y, dt0, dt1, dt2);
        pz.initNonuniformCatmullRom(p0->z, p1->z, p2->z, p3->z, dt0, dt1, dt2);
    } else if (curveType == "catmullrom") {
        px.initCatmullRom(require(p0).x, require(p1).x, require(p2).x, require(p3).x, tension);
        py.initCatmullRom(p0->y, p1->y, p2->y, p3->y, tension);
        pz.initCatmullRom(p0->z, p1->z, p2->z, p3->z, tension);
    }
    return target.set(px.calc(weight), py.calc(weight), pz.calc(weight));
}
std::shared_ptr<Curve<Vector3>> CatmullRomCurve3::clone() const {
    auto result = std::make_shared<CatmullRomCurve3>(); result->copy(*this); return result;
}
CatmullRomCurve3& CatmullRomCurve3::copy(const Curve<Vector3>& source) {
    Curve<Vector3>::copy(source);
    const auto& curve = dynamic_cast<const CatmullRomCurve3&>(source);
    points = std::make_shared<Points>();
    if (!curve.points) throw JSTypeError("CatmullRomCurve3: source points is null");
    const auto length = curve.points->size();
    for (std::size_t i = 0; i < length; ++i) points->push_back(std::make_shared<Vector3>(at(curve.points, static_cast<double>(i)).clone()));
    closed = curve.closed; curveType = curve.curveType; tension = curve.tension;
    return *this;
}
JsonValue CatmullRomCurve3::toJSON() const {
    auto data = Curve<Vector3>::toJSON();
    if (!points) throw JSTypeError("CatmullRomCurve3: points is null");
    JsonValue::Array entries;
    for (std::size_t i = 0, length = points->size(); i < length; ++i)
        entries.push_back(Curve_detail::vectorJSON(at(points, static_cast<double>(i))));
    data["points"] = std::move(entries);
    data["closed"] = closed; data["curveType"] = curveType; data["tension"] = tension;
    return data;
}
CatmullRomCurve3& CatmullRomCurve3::fromJSON(const JsonValue& json) {
    Curve<Vector3>::fromJSON(json);
    points = std::make_shared<Points>();
    for (const auto& entry : json.at("points").get<JsonValue::Array>()) {
        auto point = std::make_shared<Vector3>(); Curve_detail::vectorFromJSON(*point, entry); points->push_back(std::move(point));
    }
    closed = json.at("closed").get<bool>(); curveType = json.at("curveType").get<std::string>(); tension = json.at("tension").get<double>();
    return *this;
}
