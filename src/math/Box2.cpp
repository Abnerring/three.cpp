#include "math/Box2.h"

Box2::Box2(const Vector2& minimum, const Vector2& maximum) : min(minimum), max(maximum) {}
Box2& Box2::set(const Vector2& minimum, const Vector2& maximum) {
    min.copy(minimum);
    max.copy(maximum);
    return *this;
}
Box2& Box2::setFromPoints(const std::vector<Vector2>& points) {
    makeEmpty();
    for (const auto& point : points)
        expandByPoint(point);
    return *this;
}
Box2& Box2::setFromCenterAndSize(const Vector2& center, const Vector2& size) {
    Vector2 halfSize;
    halfSize.copy(size).multiplyScalar(0.5);
    min.copy(center).sub(halfSize);
    max.copy(center).add(halfSize);
    return *this;
}
Box2 Box2::clone() const {
    return Box2().copy(*this);
}
Box2& Box2::copy(const Box2& box) {
    return set(box.min, box.max);
}
Box2& Box2::makeEmpty() {
    min.set(INFINITY, INFINITY);
    max.set(-INFINITY, -INFINITY);
    return *this;
}
bool Box2::isEmpty() const {
    return max.x() < min.x() || max.y() < min.y();
}
Vector2& Box2::getCenter(Vector2& target) const {
    return isEmpty() ? target.set(0, 0) : target.addVectors(min, max).multiplyScalar(0.5);
}
Vector2& Box2::getSize(Vector2& target) const {
    return isEmpty() ? target.set(0, 0) : target.subVectors(max, min);
}
Box2& Box2::expandByPoint(const Vector2& point) {
    min.min(point);
    max.max(point);
    return *this;
}
Box2& Box2::expandByVector(const Vector2& vector) {
    min.sub(vector);
    max.add(vector);
    return *this;
}
Box2& Box2::expandByScalar(double scalar) {
    min.addScalar(-scalar);
    max.addScalar(scalar);
    return *this;
}
bool Box2::containsPoint(const Vector2& point) const {
    return point.x() >= min.x() && point.x() <= max.x() && point.y() >= min.y() &&
           point.y() <= max.y();
}
bool Box2::containsBox(const Box2& box) const {
    return min.x() <= box.min.x() && box.max.x() <= max.x() && min.y() <= box.min.y() &&
           box.max.y() <= max.y();
}
Vector2& Box2::getParameter(const Vector2& point, Vector2& target) const {
    return target.set((point.x() - min.x()) / (max.x() - min.x()),
                      (point.y() - min.y()) / (max.y() - min.y()));
}
bool Box2::intersectsBox(const Box2& box) const {
    return box.max.x() >= min.x() && box.min.x() <= max.x() && box.max.y() >= min.y() &&
           box.min.y() <= max.y();
}
Vector2& Box2::clampPoint(const Vector2& point, Vector2& target) const {
    return target.copy(point).clamp(min, max);
}
double Box2::distanceToPoint(const Vector2& point) const {
    Vector2 vector;
    return clampPoint(point, vector).distanceTo(point);
}
Box2& Box2::intersect(const Box2& box) {
    min.max(box.min);
    max.min(box.max);
    if (isEmpty())
        makeEmpty();
    return *this;
}
Box2& Box2::unionWith(const Box2& box) {
    min.min(box.min);
    max.max(box.max);
    return *this;
}
Box2& Box2::translate(const Vector2& offset) {
    min.add(offset);
    max.add(offset);
    return *this;
}
bool Box2::equals(const Box2& box) const {
    return box.min.equals(min) && box.max.equals(max);
}
