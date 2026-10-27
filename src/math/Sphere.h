#pragma once
#include "core/JSValue.h"
#include "math/Vector3.h"
#include <vector>
JSValue nativeSphereConstructor();
class Box3;
class Matrix4;
class Plane;

class Sphere {
  public:
    const bool isSphere = true;
    Vector3 center;
    double radius;
    Sphere(const Vector3& center = Vector3(), double radius = -1);
    Sphere& operator=(const Sphere& source) { return copy(source); }
    Sphere& set(const Vector3& center, double radius);
    Sphere& copy(const Sphere& sphere);
    Sphere clone() const;
    Sphere& setFromPoints(const std::vector<Vector3>& points,
                          const Vector3* optionalCenter = nullptr);
    Sphere& makeEmpty();
    bool isEmpty() const;
    bool containsPoint(const Vector3& point) const;
    double distanceToPoint(const Vector3& point) const;
    bool intersectsSphere(const Sphere& sphere) const;
    bool intersectsBox(const Box3& box) const;
    bool intersectsPlane(const Plane& plane) const;
    Vector3& clampPoint(const Vector3& point, Vector3& target) const;
    Box3& getBoundingBox(Box3& target) const;
    Sphere& applyMatrix4(const Matrix4& matrix);
    Sphere& translate(const Vector3& offset);
    Sphere& expandByPoint(const Vector3& point);
    // union is a C++ keyword, so the upstream name cannot be used here; see the Box3 note.
    Sphere& unionWith(const Sphere& sphere);
    JSValue toJSON() const;
    Sphere& fromJSON(const JSValue& json);
    bool equals(const Sphere& sphere) const;
};
