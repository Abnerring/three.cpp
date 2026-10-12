#include "extras/core/Path.h"
#include "extras/curves/LineCurve.h"
#include "extras/curves/QuadraticBezierCurve.h"
#include "extras/curves/CubicBezierCurve.h"
#include "extras/curves/SplineCurve.h"
#include "extras/curves/EllipseCurve.h"

Path::Path(const PointsPtr& points) { type = "Path"; if (points) setFromPoints(*points); }
Path& Path::setFromPoints(const Points& points) {
    if (points.empty()) throw std::out_of_range("Path.setFromPoints: empty points");
    moveTo(points[0]->x, points[0]->y);
    for (std::size_t i = 1; i < points.size(); ++i) lineTo(points[i]->x, points[i]->y);
    return *this;
}
Path& Path::moveTo(double x, double y) { currentPoint->set(x, y); return *this; }
Path& Path::lineTo(double x, double y) {
    curves->push_back(std::make_shared<LineCurve>(std::make_shared<Vector2>(*currentPoint), std::make_shared<Vector2>(x, y)));
    currentPoint->set(x, y); return *this;
}
Path& Path::quadraticCurveTo(double cpx, double cpy, double x, double y) {
    curves->push_back(std::make_shared<QuadraticBezierCurve>(std::make_shared<Vector2>(*currentPoint),
        std::make_shared<Vector2>(cpx, cpy), std::make_shared<Vector2>(x, y)));
    currentPoint->set(x, y); return *this;
}
Path& Path::bezierCurveTo(double cp1x, double cp1y, double cp2x, double cp2y, double x, double y) {
    curves->push_back(std::make_shared<CubicBezierCurve>(std::make_shared<Vector2>(*currentPoint),
        std::make_shared<Vector2>(cp1x, cp1y), std::make_shared<Vector2>(cp2x, cp2y), std::make_shared<Vector2>(x, y)));
    currentPoint->set(x, y); return *this;
}
Path& Path::splineThru(const Points& points) {
    auto joined = std::make_shared<SplineCurve::Points>();
    joined->push_back(std::make_shared<Vector2>(*currentPoint));
    joined->insert(joined->end(), points.begin(), points.end());
    curves->push_back(std::make_shared<SplineCurve>(joined));
    if (points.empty()) throw std::out_of_range("Path.splineThru: empty points");
    currentPoint->copy(*points.back()); return *this;
}
Path& Path::arc(double x, double y, double radius, double start, double end, bool clockwise) {
    return absarc(x + currentPoint->x, y + currentPoint->y, radius, start, end, clockwise);
}
Path& Path::absarc(double x, double y, double radius, double start, double end, bool clockwise) {
    return absellipse(x, y, radius, radius, start, end, clockwise);
}
Path& Path::ellipse(double x, double y, double xr, double yr, double start, double end, bool clockwise, double rotation) {
    return absellipse(x + currentPoint->x, y + currentPoint->y, xr, yr, start, end, clockwise, rotation);
}
Path& Path::absellipse(double x, double y, double xr, double yr, double start, double end, bool clockwise, double rotation) {
    auto curve = std::make_shared<EllipseCurve>(x, y, xr, yr, start, end, clockwise, rotation);
    if (!curves->empty()) {
        const auto first = curve->getPoint(0);
        if (!first.equals(*currentPoint)) lineTo(first.x, first.y);
    }
    curves->push_back(curve);
    currentPoint->copy(curve->getPoint(1)); return *this;
}
std::shared_ptr<Curve<Vector2>> Path::clone() const { auto result = std::make_shared<Path>(); result->copy(*this); return result; }
Path& Path::copy(const Curve<Vector2>& source) {
    CurvePath<Vector2>::copy(source); currentPoint->copy(*dynamic_cast<const Path&>(source).currentPoint); return *this;
}
JsonValue Path::toJSON() const { auto json = CurvePath<Vector2>::toJSON(); json["currentPoint"] = Curve_detail::vectorJSON(*currentPoint); return json; }
Path& Path::fromJSON(const JsonValue& json) {
    CurvePath<Vector2>::fromJSON(json); Curve_detail::vectorFromJSON(*currentPoint, json.at("currentPoint")); return *this;
}
