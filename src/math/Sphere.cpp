#include "math/Sphere.h"
#include "math/Box3.h"
#include "math/MathUtils.h"
#include "math/Matrix4.h"
#include "math/Plane.h"
#include <algorithm>
#include <cmath>
Sphere::Sphere(const Vector3& c, double r) : center(c), radius(r) {}
Sphere& Sphere::set(const Vector3& c, double r) {
    center.copy(c);
    radius = r;
    return *this;
}
Sphere& Sphere::copy(const Sphere& s) {
    return set(s.center, s.radius);
}
Sphere Sphere::clone() const {
    return Sphere(center, radius);
}
Sphere& Sphere::setFromPoints(const std::vector<Vector3>& p, const Vector3* c) {
    if (c)
        center.copy(*c);
    else {
        Box3 box;
        box.setFromPoints(p).getCenter(center);
    }
    double maxRadiusSq = 0;
    for (const auto& v : p)
        maxRadiusSq = MathUtils::max(maxRadiusSq, center.distanceToSquared(v));
    radius = std::sqrt(maxRadiusSq);
    return *this;
}
Sphere& Sphere::makeEmpty() {
    center.set(0, 0, 0);
    radius = -1;
    return *this;
}
bool Sphere::isEmpty() const {
    return radius < 0;
}
bool Sphere::containsPoint(const Vector3& p) const {
    return p.distanceToSquared(center) <= radius * radius;
}
double Sphere::distanceToPoint(const Vector3& p) const {
    return p.distanceTo(center) - radius;
}
bool Sphere::intersectsSphere(const Sphere& s) const {
    const double r = radius + s.radius;
    return s.center.distanceToSquared(center) <= r * r;
}
bool Sphere::intersectsBox(const Box3& b) const {
    return b.intersectsSphere(*this);
}
bool Sphere::intersectsPlane(const Plane& plane) const {
    return std::abs(plane.distanceToPoint(center)) <= radius;
}
Vector3& Sphere::clampPoint(const Vector3& p, Vector3& t) const {
    const double distanceSq = center.distanceToSquared(p);
    t.copy(p);
    if (distanceSq > radius * radius) {
        t.sub(center);
        t.normalize();
        t.multiplyScalar(radius);
        t.add(center);
    }
    return t;
}
Box3& Sphere::getBoundingBox(Box3& b) const {
    if (isEmpty())
        b.makeEmpty();
    else
        b.set(center, center).expandByScalar(radius);
    return b;
}
Sphere& Sphere::applyMatrix4(const Matrix4& m) {
    center.applyMatrix4(m);
    const auto e = m.elements();
    const double sx = e[0] * e[0] + e[1] * e[1] + e[2] * e[2],
                 sy = e[4] * e[4] + e[5] * e[5] + e[6] * e[6],
                 sz = e[8] * e[8] + e[9] * e[9] + e[10] * e[10];
    radius *= std::sqrt(MathUtils::max(sx, MathUtils::max(sy, sz)));
    return *this;
}
Sphere& Sphere::translate(const Vector3& v) {
    center.add(v);
    return *this;
}
Sphere& Sphere::expandByPoint(const Vector3& p) {
    if (isEmpty())
        return set(p, 0);
    Vector3 v;
    v.subVectors(p, center);
    const double lengthSq = v.lengthSq();
    if (lengthSq > radius * radius) {
        const double length = std::sqrt(lengthSq);
        const double delta = (length - radius) * .5;
        v.multiplyScalar(delta / length);
        center.add(v);
        radius += delta;
    }
    return *this;
}
Sphere& Sphere::unionWith(const Sphere& s) {
    if (s.isEmpty())
        return *this;
    if (isEmpty())
        return copy(s);
    if (center.equals(s.center)) {
        radius = MathUtils::max(radius, s.radius);
        return *this;
    }
    Vector3 v;
    v.subVectors(s.center, center);
    v.normalize();
    v.multiplyScalar(s.radius);
    Vector3 p(s.center);
    p.add(v);
    expandByPoint(p);
    p.copy(s.center);
    p.sub(v);
    expandByPoint(p);
    return *this;
}
bool Sphere::equals(const Sphere& s) const {
    return center.equals(s.center) && radius == s.radius;
}
