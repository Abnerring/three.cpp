#include "math/Line3.h"
#include "math/MathUtils.h"
#include "math/Matrix4.h"
#include <cmath>

Line3& Line3::set(const Vector3& start, const Vector3& end) {
    this->start.copy(start);
    this->end.copy(end);
    return *this;
}

Line3& Line3::copy(const Line3& line) {
    this->start.copy(line.start);
    this->end.copy(line.end);
    return *this;
}

Vector3& Line3::getCenter(Vector3& target) const {
    return target.addVectors(this->start, this->end).multiplyScalar(0.5);
}

Vector3& Line3::delta(Vector3& target) const {
    return target.subVectors(this->end, this->start);
}

HIGH_PRECISION Line3::distanceSq() const {
    return this->start.distanceToSquared(this->end);
}

HIGH_PRECISION Line3::distance() const {
    return this->start.distanceTo(this->end);
}

Vector3& Line3::at(HIGH_PRECISION t, Vector3& target) const {
    return this->delta(target).multiplyScalar(t).add(this->start);
}

HIGH_PRECISION Line3::closestPointToPointParameter(const Vector3& point, bool clampToLine) const {
    Vector3 _startP, _startEnd;
    _startP.subVectors(point, this->start);
    _startEnd.subVectors(this->end, this->start);
    const auto startEnd2 = _startEnd.dot(_startEnd);
    if (startEnd2 == 0)
        return 0;
    const auto startEnd_startP = _startEnd.dot(_startP);
    HIGH_PRECISION t = startEnd_startP / startEnd2;
    if (clampToLine) {
        t = MathUtils::clamp(t, 0, 1);
    }
    return t;
}

Vector3& Line3::closestPointToPoint(const Vector3& point, bool clampToLine, Vector3& target) const {
    const auto t = this->closestPointToPointParameter(point, clampToLine);
    return this->delta(target).multiplyScalar(t).add(this->start);
}

HIGH_PRECISION Line3::distanceSqToLine3(const Line3& line, Vector3* optionalC1,
                                        Vector3* optionalC2) const {
    Vector3 scratchC1, scratchC2;
    Vector3& c1 = optionalC1 ? *optionalC1 : scratchC1;
    Vector3& c2 = optionalC2 ? *optionalC2 : scratchC2;
    Vector3 _d1, _d2, _r;
    const auto EPSILON = 1e-8 * 1e-8;
    HIGH_PRECISION s, t;
    const auto& p1 = this->start;
    const auto& p2 = line.start;
    const auto& q1 = this->end;
    const auto& q2 = line.end;
    _d1.subVectors(q1, p1);
    _d2.subVectors(q2, p2);
    _r.subVectors(p1, p2);
    const auto a = _d1.dot(_d1);
    const auto e = _d2.dot(_d2);
    const auto f = _d2.dot(_r);
    if (a <= EPSILON && e <= EPSILON) {
        c1.copy(p1);
        c2.copy(p2);
        return c1.distanceToSquared(c2);
    }
    if (a <= EPSILON) {
        s = 0;
        t = f / e;
        t = MathUtils::clamp(t, 0, 1);
    } else {
        const auto c = _d1.dot(_r);
        if (e <= EPSILON) {
            t = 0;
            s = MathUtils::clamp(-c / a, 0, 1);
        } else {
            const auto b = _d1.dot(_d2);
            const auto denom = a * e - b * b;
            if (denom != 0) {
                s = MathUtils::clamp((b * f - c * e) / denom, 0, 1);
            } else {
                s = 0;
            }
            t = (b * s + f) / e;
            if (t < 0) {
                t = 0.;
                s = MathUtils::clamp(-c / a, 0, 1);
            } else if (t > 1) {
                t = 1;
                s = MathUtils::clamp((b - c) / a, 0, 1);
            }
        }
    }
    c1.copy(p1).addScaledVector(_d1, s);
    c2.copy(p2).addScaledVector(_d2, t);
    return c1.distanceToSquared(c2);
}

Line3& Line3::applyMatrix4(const Matrix4& matrix) {
    this->start.applyMatrix4(matrix);
    this->end.applyMatrix4(matrix);
    return *this;
}

bool Line3::equals(const Line3& line) const {
    return line.start.equals(this->start) && line.end.equals(this->end);
}

Line3::Line3(const Vector3& start, const Vector3& end) : start(start), end(end) {}
Line3 Line3::clone() const {
    return Line3().copy(*this);
}
