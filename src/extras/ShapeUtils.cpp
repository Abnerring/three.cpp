#include "extras/ShapeUtils.h"
#include "core/JSValue.h"
#include "extras/lib/earcut.h"

namespace {
const Vector2& value(const Vector2& point) { return point; }
const Vector2& value(const std::shared_ptr<Vector2>& point) {
    if (!point) throw JSTypeError("Cannot read coordinates of missing contour point");
    return *point;
}
template<class Points> double signedArea(const Points& contour) {
    double area = 0;
    for (std::size_t q = 0, p = contour.empty() ? 0 : contour.size() - 1; q < contour.size(); p = q++) {
        area += value(contour[p]).x * value(contour[q]).y - value(contour[q]).x * value(contour[p]).y;
    }
    return area * 0.5;
}
template<class Points> void removeDuplicateEnd(Points& points) {
    if (points.size() > 2 && value(points.back()).equals(value(points.front()))) points.pop_back();
}
template<class Points> void addContour(std::vector<double>& vertices, const Points& contour) {
    for (const auto& point : contour) { vertices.push_back(value(point).x); vertices.push_back(value(point).y); }
}
template<class Points> ShapeUtils::Faces triangulate(Points& contour, std::vector<Points>& holes) {
    std::vector<double> vertices;
    std::vector<std::size_t> holeIndices;
    removeDuplicateEnd(contour);
    addContour(vertices, contour);
    auto holeIndex = contour.size();
    for (auto& hole : holes) removeDuplicateEnd(hole);
    for (const auto& hole : holes) {
        holeIndices.push_back(holeIndex); holeIndex += hole.size(); addContour(vertices, hole);
    }
    const auto triangles = earcut(vertices, holeIndices, 2);
    ShapeUtils::Faces faces;
    for (std::size_t i = 0; i < triangles.size(); i += 3) faces.push_back({triangles[i], triangles[i + 1], triangles[i + 2]});
    return faces;
}
}
double ShapeUtils::area(const Points& contour) { return signedArea(contour); }
double ShapeUtils::area(const std::vector<Vector2>& contour) { return signedArea(contour); }
bool ShapeUtils::isClockWise(const Points& points) { return area(points) < 0; }
bool ShapeUtils::isClockWise(const std::vector<Vector2>& points) { return area(points) < 0; }
ShapeUtils::Faces ShapeUtils::triangulateShape(Points& contour, std::vector<Points>& holes) { return triangulate(contour, holes); }
ShapeUtils::Faces ShapeUtils::triangulateShape(std::vector<Vector2>& contour, std::vector<std::vector<Vector2>>& holes) { return triangulate(contour, holes); }
