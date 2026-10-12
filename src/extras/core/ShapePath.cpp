#include "extras/core/ShapePath.h"
#include "core/ArraySort.h"
#include "extras/ShapeUtils.h"
#include "math/Box2.h"
#include <iostream>

namespace {
Path& requirePath(const std::shared_ptr<Path>& path) {
    if (!path) throw JSTypeError("ShapePath: currentPath is null");
    return *path;
}
bool pointInPolygon(const Vector2& p, const Path::Points& polygon) {
    bool inside = false;
    const auto n = polygon.size();
    for (std::size_t i = 0, j = n ? n - 1 : 0; i < n; j = i++) {
        const auto& a = *polygon[i]; const auto& b = *polygon[j];
        if ((a.y > p.y) != (b.y > p.y) && p.x < (b.x - a.x) * (p.y - a.y) / (b.y - a.y) + a.x) inside = !inside;
    }
    return inside;
}
Vector2 interiorPoint(const Path::Points& polygon, const Box2& box) {
    Vector2 point; box.getCenter(point);
    if (pointInPolygon(point, polygon)) return point;
    const auto y = point.y;
    std::vector<double> intercepts;
    for (std::size_t i = 0; i < polygon.size(); ++i) {
        const auto& a = *polygon[i]; const auto& b = *polygon[(i + 1) % polygon.size()];
        if ((a.y > y) != (b.y > y)) intercepts.push_back(a.x + (y - a.y) * (b.x - a.x) / (b.y - a.y));
    }
    if (intercepts.size() > 1) {
        v8ArraySort(intercepts, [](double a, double b) { return a - b; });
        point.x = (intercepts[0] + intercepts[1]) / 2;
    }
    return point;
}
}
ShapePath& ShapePath::moveTo(double x, double y) {
    currentPath = std::make_shared<Path>(); subPaths->push_back(currentPath); currentPath->moveTo(x, y); return *this;
}
ShapePath& ShapePath::lineTo(double x, double y) { requirePath(currentPath).lineTo(x, y); return *this; }
ShapePath& ShapePath::quadraticCurveTo(double cpx, double cpy, double x, double y) {
    requirePath(currentPath).quadraticCurveTo(cpx, cpy, x, y); return *this;
}
ShapePath& ShapePath::bezierCurveTo(double cp1x, double cp1y, double cp2x, double cp2y, double x, double y) {
    requirePath(currentPath).bezierCurveTo(cp1x, cp1y, cp2x, cp2y, x, y); return *this;
}
ShapePath& ShapePath::splineThru(const Path::Points& points) { requirePath(currentPath).splineThru(points); return *this; }
std::vector<std::shared_ptr<Shape>> ShapePath::toShapes() {
    // Preserve the getter order in (userData.style && userData.style.fillRule).
    auto fillRule = userData.get("style");
    if (fillRule.toBoolean()) fillRule = userData.get("style").get("fillRule");
    if (!fillRule.toBoolean()) fillRule = "nonzero";
    if (!fillRule.strictEquals("nonzero") && !fillRule.strictEquals("evenodd")) {
        const auto message = "THREE.Fill-rule \"" + fillRule.toPrimitive().toString() +
            "\" is not supported, falling back to \"nonzero\".\n";
        std::cerr << message;
        fillRule = "nonzero";
    }
    const bool evenOdd = fillRule.strictEquals("evenodd");
    const auto isInside = [evenOdd](std::int64_t winding) { return evenOdd ? winding % 2 != 0 : winding != 0; };
    struct Entry {
        std::shared_ptr<Path> subPath;
        Path::Points points;
        Box2 boundingBox;
        Vector2 interior;
        double absArea;
        std::int64_t winding;
        Entry* container = nullptr;
        bool exclude = false, hole = false;
        std::shared_ptr<Shape> shape;
    };
    std::vector<Entry> entries;
    const auto paths = subPaths;
    for (std::size_t i = 0; i < paths->size(); ++i) {
        const auto subPath = paths->at(i);
        if (!subPath) throw JSTypeError("Cannot read getPoints of missing subpath");
        auto points = subPath->getPoints();
        if (points.size() < 3) continue;
        const auto area = ShapeUtils::area(points);
        if (area == 0) continue;
        Box2 box;
        for (const auto& point : points) box.expandByPoint(*point);
        const auto inside = interiorPoint(points, box);
        entries.push_back({subPath, std::move(points), box, inside, std::abs(area), area < 0 ? -1 : 1});
    }
    v8ArraySort(entries, [](const Entry& a, const Entry& b) { return b.absArea - a.absArea; });
    for (std::size_t i = 0; i < entries.size(); ++i) {
        auto& entry = entries[i];
        std::int64_t containerWinding = 0;
        for (std::size_t j = i; j-- > 0;) {
            auto& candidate = entries[j];
            if (!candidate.boundingBox.containsBox(entry.boundingBox)) continue;
            if (!pointInPolygon(entry.interior, candidate.points)) continue;
            entry.container = candidate.exclude ? candidate.container : &candidate;
            containerWinding = candidate.winding;
            entry.winding += containerWinding;
            break;
        }
        if (isInside(entry.winding) == isInside(containerWinding)) entry.exclude = true;
    }
    for (auto& entry : entries) {
        if (entry.exclude) continue;
        entry.hole = entry.container && !entry.container->hole;
    }
    std::vector<std::shared_ptr<Shape>> shapes;
    for (auto& entry : entries) {
        if (entry.exclude || entry.hole) continue;
        entry.shape = std::make_shared<Shape>(); entry.shape->curves = entry.subPath->curves; shapes.push_back(entry.shape);
    }
    for (auto& entry : entries) {
        if (entry.exclude || !entry.hole) continue;
        const auto shape = entry.container ? entry.container->shape : nullptr;
        if (!shape) continue;
        auto hole = std::make_shared<Path>(); hole->curves = entry.subPath->curves; shape->holes->push_back(hole);
    }
    return shapes;
}
