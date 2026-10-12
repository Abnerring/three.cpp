#pragma once
#include "math/Vector2.h"
#include <limits>
#include <vector>

class Box2 {
  public:
    static constexpr bool isBox2 = true;
    Vector2 min, max;
    Box2(const Vector2& minimum = Vector2(INFINITY, INFINITY),
         const Vector2& maximum = Vector2(-INFINITY, -INFINITY));
    Box2& set(const Vector2& minimum, const Vector2& maximum);
    Box2& setFromPoints(const std::vector<Vector2>& points);
    Box2& setFromCenterAndSize(const Vector2& center, const Vector2& size);
    Box2 clone() const;
    Box2& copy(const Box2& box);
    Box2& makeEmpty();
    bool isEmpty() const;
    Vector2& getCenter(Vector2& target) const;
    Vector2& getSize(Vector2& target) const;
    Box2& expandByPoint(const Vector2& point);
    Box2& expandByVector(const Vector2& vector);
    Box2& expandByScalar(double scalar);
    bool containsPoint(const Vector2& point) const;
    bool containsBox(const Box2& box) const;
    Vector2& getParameter(const Vector2& point, Vector2& target) const;
    bool intersectsBox(const Box2& box) const;
    Vector2& clampPoint(const Vector2& point, Vector2& target) const;
    double distanceToPoint(const Vector2& point) const;
    Box2& intersect(const Box2& box);
    Box2& unionWith(const Box2& box);
    Box2& translate(const Vector2& offset);
    bool equals(const Box2& box) const;
};
