#pragma once
#include "math/Vector3.h"
class Matrix4;
class Line3 {
  public:
    Vector3 start, end;
    Line3(const Vector3& start = Vector3(), const Vector3& end = Vector3());
    Line3& set(const Vector3& start, const Vector3& end);
    Line3& copy(const Line3& line);
    Vector3& getCenter(Vector3& target) const;
    Vector3& delta(Vector3& target) const;
    HIGH_PRECISION distanceSq() const;
    HIGH_PRECISION distance() const;
    Vector3& at(HIGH_PRECISION t, Vector3& target) const;
    HIGH_PRECISION closestPointToPointParameter(const Vector3& point,
                                                bool clampToLine = false) const;
    Vector3& closestPointToPoint(const Vector3& point, bool clampToLine, Vector3& target) const;
    HIGH_PRECISION distanceSqToLine3(const Line3& line, Vector3* c1 = nullptr,
                                     Vector3* c2 = nullptr) const;
    HIGH_PRECISION distanceSqToLine3(const Line3& line, Vector3& c1, Vector3& c2) const {
        return distanceSqToLine3(line, &c1, &c2);
    }
    Line3& applyMatrix4(const Matrix4& matrix);
    bool equals(const Line3& line) const;
    Line3 clone() const;
};
