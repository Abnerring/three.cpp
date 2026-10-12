#pragma once
#include "core/JsonValue.h"
#include "math/Vector3.h"
class Line3;
class Box3;
class Sphere;
class Matrix3;
class Matrix4;
class Plane {
  public:
    static constexpr bool isPlane = true;
    Vector3 normal;
    HIGH_PRECISION constant;
    Plane(const Vector3& normal = Vector3(1, 0, 0), HIGH_PRECISION constant = 0);
    Plane& set(const Vector3& normal, HIGH_PRECISION constant);
    Plane& setComponents(HIGH_PRECISION x, HIGH_PRECISION y, HIGH_PRECISION z, HIGH_PRECISION w);
    Plane& setFromNormalAndCoplanarPoint(const Vector3& normal, const Vector3& point);
    Plane& setFromCoplanarPoints(const Vector3& a, const Vector3& b, const Vector3& c);
    Plane& copy(const Plane& plane);
    Plane& normalize();
    Plane& negate();
    HIGH_PRECISION distanceToPoint(const Vector3& point) const;
    HIGH_PRECISION distanceToSphere(const Sphere& sphere) const;
    Vector3& projectPoint(const Vector3& point, Vector3& target) const;
    // Returns the caller's target on a hit, nullptr on a miss. No allocation.
    Vector3* intersectLine(const Line3& line, Vector3& target, bool clampToLine = true) const;
    bool intersectsLine(const Line3& line) const;
    bool intersectsBox(const Box3& box) const;
    bool intersectsSphere(const Sphere& sphere) const;
    Vector3& coplanarPoint(Vector3& target) const;
    Plane& applyMatrix4(const Matrix4& matrix, const Matrix3* optionalNormalMatrix = nullptr);
    Plane& applyMatrix4(const Matrix4& matrix, const Matrix3& normalMatrix) {
        return applyMatrix4(matrix, &normalMatrix);
    }
    Plane& translate(const Vector3& offset);
    bool equals(const Plane& plane) const;
    Plane clone() const;
    JsonValue toJSON() const;
    Plane& fromJSON(const JsonValue& json);
};
