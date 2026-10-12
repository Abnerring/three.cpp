#pragma once
#include "math/Vector3.h"
#include <optional>
class Matrix4;
class Plane;
class Sphere;
class Box3;
class Ray {
  public:
    Vector3 origin, direction;
    Ray(const Vector3& origin = Vector3(), const Vector3& direction = Vector3(0, 0, -1));
    Ray& set(const Vector3& origin, const Vector3& direction);
    Ray& copy(const Ray& ray);
    Vector3& at(HIGH_PRECISION t, Vector3& target) const;
    Ray& lookAt(const Vector3& v);
    Ray& recast(HIGH_PRECISION t);
    Vector3& closestPointToPoint(const Vector3& point, Vector3& target) const;
    HIGH_PRECISION distanceToPoint(const Vector3& point) const;
    HIGH_PRECISION distanceSqToPoint(const Vector3& point) const;
    HIGH_PRECISION distanceSqToSegment(const Vector3& v0, const Vector3& v1,
                                       Vector3* optionalPointOnRay = nullptr,
                                       Vector3* optionalPointOnSegment = nullptr) const;
    // Nullable intersections return the caller's target address; misses do not allocate.
    Vector3* intersectSphere(const Sphere& sphere, Vector3& target) const;
    bool intersectsSphere(const Sphere& sphere) const;
    std::optional<HIGH_PRECISION> distanceToPlane(const Plane& plane) const;
    Vector3* intersectPlane(const Plane& plane, Vector3& target) const;
    bool intersectsPlane(const Plane& plane) const;
    Vector3* intersectBox(const Box3& box, Vector3& target) const;
    bool intersectsBox(const Box3& box) const;
    Vector3* intersectTriangle(const Vector3& a, const Vector3& b, const Vector3& c,
                               bool backfaceCulling, Vector3& target) const;
    Ray& applyMatrix4(const Matrix4& matrix4);
    bool equals(const Ray& ray) const;
    Ray clone() const;
};
