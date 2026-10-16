#include "math/Capsule.h"
#include "math/Box3.h"

namespace {

// The module scope helper of the source. The four strict `<` comparisons are the algorithm:
// a box face exactly `radius` away does not intersect.
bool checkAABBAxis(double p1x, double p1y, double p2x, double p2y, double minx, double maxx,
                   double miny, double maxy, double radius) {
    return (minx - p1x < radius || minx - p2x < radius) && (p1x - maxx < radius || p2x - maxx < radius) &&
           (miny - p1y < radius || miny - p2y < radius) && (p1y - maxy < radius || p2y - maxy < radius);
}

} // namespace

// Parameter names of the definitions are shortened the way the existing math classes do
// (see Sphere.cpp) so that no parameter shadows the member of the same upstream name.
Capsule::Capsule(const Vector3& s, const Vector3& e, double r) : start(s), end(e), radius(r) {}

Capsule Capsule::clone() const {
    return Capsule().copy(*this);
}

Capsule& Capsule::set(const Vector3& s, const Vector3& e, double r) {
    this->start.copy(s);
    this->end.copy(e);
    this->radius = r;

    return *this;
}

Capsule& Capsule::copy(const Capsule& capsule) {
    this->start.copy(capsule.start);
    this->end.copy(capsule.end);
    this->radius = capsule.radius;

    return *this;
}

Vector3& Capsule::getCenter(Vector3& target) const {
    return target.copy(this->end).add(this->start).multiplyScalar(0.5);
}

Capsule& Capsule::translate(const Vector3& v) {
    this->start.add(v);
    this->end.add(v);

    return *this;
}

bool Capsule::intersectsBox(const Box3& box) const {
    return (checkAABBAxis(this->start.x, this->start.y, this->end.x, this->end.y, box.min.x, box.max.x,
                          box.min.y, box.max.y, this->radius) &&
            checkAABBAxis(this->start.x, this->start.z, this->end.x, this->end.z, box.min.x, box.max.x,
                          box.min.z, box.max.z, this->radius) &&
            checkAABBAxis(this->start.y, this->start.z, this->end.y, this->end.z, box.min.y, box.max.y,
                          box.min.z, box.max.z, this->radius));
}
