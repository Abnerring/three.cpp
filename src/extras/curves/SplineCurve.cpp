#include "extras/curves/SplineCurve.h"
#include "extras/core/Interpolations.h"
#include "core/JSValue.h"
#include <utility>

namespace {
Vector2& at(const SplineCurve::PointsPtr& points, double index) {
    if (!points || !std::isfinite(index) || index < 0 || std::floor(index) != index || index >= points->size())
        throw JSTypeError("SplineCurve: control point is undefined");
    const auto& point = (*points)[static_cast<std::size_t>(index)];
    if (!point) throw JSTypeError("SplineCurve: control point is null");
    return *point;
}
}
SplineCurve::SplineCurve(PointsPtr points) : points(std::move(points)) { type = "SplineCurve"; }
Vector2& SplineCurve::getPoint(double t, Vector2& target) {
    if (!points) throw JSTypeError("SplineCurve: points is null");
    const double length = static_cast<double>(points->size());
    const double p = (length - 1) * t;
    const double intPoint = std::floor(p), weight = p - intPoint;
    const auto& p0 = at(points, intPoint == 0 ? intPoint : intPoint - 1);
    const auto& p1 = at(points, intPoint);
    const auto& p2 = at(points, intPoint > length - 2 ? length - 1 : intPoint + 1);
    const auto& p3 = at(points, intPoint > length - 3 ? length - 1 : intPoint + 2);
    return target.set(CatmullRom(weight, p0.x, p1.x, p2.x, p3.x), CatmullRom(weight, p0.y, p1.y, p2.y, p3.y));
}
std::shared_ptr<Curve<Vector2>> SplineCurve::clone() const {
    auto result = std::make_shared<SplineCurve>(); result->copy(*this); return result;
}
SplineCurve& SplineCurve::copy(const Curve<Vector2>& source) {
    Curve<Vector2>::copy(source);
    const auto& curve = dynamic_cast<const SplineCurve&>(source);
    points = std::make_shared<Points>();
    if (!curve.points) throw JSTypeError("SplineCurve: source points is null");
    const auto length = curve.points->size();
    for (std::size_t i = 0; i < length; ++i) points->push_back(std::make_shared<Vector2>(at(curve.points, static_cast<double>(i)).clone()));
    return *this;
}
JsonValue SplineCurve::toJSON() const {
    auto data = Curve<Vector2>::toJSON();
    if (!points) throw JSTypeError("SplineCurve: points is null");
    JsonValue::Array entries;
    for (std::size_t i = 0, length = points->size(); i < length; ++i)
        entries.push_back(Curve_detail::vectorJSON(at(points, static_cast<double>(i))));
    data["points"] = std::move(entries);
    return data;
}
SplineCurve& SplineCurve::fromJSON(const JsonValue& json) {
    Curve<Vector2>::fromJSON(json);
    points = std::make_shared<Points>();
    for (const auto& entry : json.at("points").get<JsonValue::Array>()) {
        auto point = std::make_shared<Vector2>(); Curve_detail::vectorFromJSON(*point, entry); points->push_back(std::move(point));
    }
    return *this;
}
