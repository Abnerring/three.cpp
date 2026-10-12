#pragma once
#include "math/Vector3.h"
#include <limits>
#include <vector>
class Matrix4;
class VertexAttribute;
class Sphere;
class Plane;
class Triangle;
class Object3D;

class Box3 {
  public:
    const bool isBox3 = true;
    Vector3 min, max;
    Box3(const Vector3& minimum = Vector3(INFINITY, INFINITY, INFINITY),
         const Vector3& maximum = Vector3(-INFINITY, -INFINITY, -INFINITY));
    Box3& operator=(const Box3& source) { return copy(source); }
    Box3& set(const Vector3& minimum, const Vector3& maximum);
    Box3& copy(const Box3& box);
    Box3 clone() const;
    Box3& makeEmpty();
    bool isEmpty() const;
    Box3& setFromArray(const std::vector<double>& array);
    Box3& setFromPoints(const std::vector<Vector3>& points);
    Box3& setFromBufferAttribute(const VertexAttribute& attribute);
    Box3& setFromObject(Object3D& object, bool precise = false);
    Box3& setFromCenterAndSize(const Vector3& center, const Vector3& size);
    Vector3& getCenter(Vector3& target) const;
    Vector3& getSize(Vector3& target) const;
    Box3& expandByPoint(const Vector3& point);
    Box3& expandByVector(const Vector3& vector);
    Box3& expandByScalar(double scalar);
    Box3& expandByObject(Object3D& object, bool precise = false);
    bool containsPoint(const Vector3& point) const;
    bool containsBox(const Box3& box) const;
    bool intersectsBox(const Box3& box) const;
    bool intersectsSphere(const Sphere& sphere) const;
    bool intersectsPlane(const Plane& plane) const;
    bool intersectsTriangle(const Triangle& triangle) const;
    Vector3& clampPoint(const Vector3& point, Vector3& target) const;
    double distanceToPoint(const Vector3& point) const;
    Vector3& getParameter(const Vector3& point, Vector3& target) const;
    Sphere& getBoundingSphere(Sphere& target) const;
    Box3& intersect(const Box3& box);
    Box3& unionWith(const Box3& box);
    Box3& applyMatrix4(const Matrix4& matrix);
    Box3& translate(const Vector3& offset);
    bool equals(const Box3& box) const;
};
