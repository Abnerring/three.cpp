#pragma once
#include "math/Vector2.h"
#include "math/Vector3.h"
#include "math/Vector4.h"
class Plane;
class Box3;
class BufferAttribute;
class Triangle {
  public:
    Vector3 a, b, c;
    Triangle(const Vector3& a = Vector3(), const Vector3& b = Vector3(),
             const Vector3& c = Vector3());
    static Vector3& getNormal(const Vector3& a, const Vector3& b, const Vector3& c,
                              Vector3& target);
    // The returned pointer refers to target. A degenerate triangle zeros target and returns
    // nullptr.
    static Vector3* getBarycoord(const Vector3& point, const Vector3& a, const Vector3& b,
                                 const Vector3& c, Vector3& target);
    static bool containsPoint(const Vector3& point, const Vector3& a, const Vector3& b,
                              const Vector3& c);
    template <class V>
    static V* getInterpolation(const Vector3& point, const Vector3& p1, const Vector3& p2,
                               const Vector3& p3, const V& v1, const V& v2, const V& v3,
                               V& target) {
        Vector3 barycoord;
        if (!getBarycoord(point, p1, p2, p3, barycoord)) {
            target.setScalar(0);
            return nullptr;
        }
        target.setScalar(0);
        target.addScaledVector(v1, barycoord.x());
        target.addScaledVector(v2, barycoord.y());
        target.addScaledVector(v3, barycoord.z());
        return &target;
    }
    static Vector2& getInterpolatedAttribute(const BufferAttribute& attr, size_t i1, size_t i2,
                                             size_t i3, const Vector3& barycoord, Vector2& target);
    static Vector3& getInterpolatedAttribute(const BufferAttribute& attr, size_t i1, size_t i2,
                                             size_t i3, const Vector3& barycoord, Vector3& target);
    static Vector4& getInterpolatedAttribute(const BufferAttribute& attr, size_t i1, size_t i2,
                                             size_t i3, const Vector3& barycoord, Vector4& target);
    static bool isFrontFacing(const Vector3& a, const Vector3& b, const Vector3& c,
                              const Vector3& direction);
    Triangle& set(const Vector3& a, const Vector3& b, const Vector3& c);
    Triangle& setFromPointsAndIndices(const std::vector<Vector3>& points, size_t i0, size_t i1,
                                      size_t i2);
    Triangle& setFromAttributeAndIndices(const BufferAttribute& attribute, size_t i0, size_t i1,
                                         size_t i2);
    Triangle clone() const;
    Triangle& copy(const Triangle& triangle);
    HIGH_PRECISION getArea() const;
    Vector3& getMidpoint(Vector3& target) const;
    Vector3& getNormal(Vector3& target) const;
    Plane& getPlane(Plane& target) const;
    Vector3* getBarycoord(const Vector3& point, Vector3& target) const;
    template <class V>
    V* getInterpolation(const Vector3& point, const V& v1, const V& v2, const V& v3,
                        V& target) const {
        return getInterpolation(point, a, b, c, v1, v2, v3, target);
    }
    bool containsPoint(const Vector3& point) const;
    bool isFrontFacing(const Vector3& direction) const;
    bool intersectsBox(const Box3& box) const;
    Vector3& closestPointToPoint(const Vector3& p, Vector3& target) const;
    bool equals(const Triangle& triangle) const;
};
