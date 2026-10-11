#include "math/Ray.h"
#include "math/Box3.h"
#include "math/MathUtils.h"
#include "math/Matrix4.h"
#include "math/Plane.h"
#include "math/Sphere.h"
#include <cmath>

Ray& Ray::set(const Vector3& origin, const Vector3& direction) {
    this->origin.copy(origin);
    this->direction.copy(direction);
    return *this;
}

Ray& Ray::copy(const Ray& ray) {
    this->origin.copy(ray.origin);
    this->direction.copy(ray.direction);
    return *this;
}

Vector3& Ray::at(HIGH_PRECISION t, Vector3& target) const {
    return target.copy(this->origin).addScaledVector(this->direction, t);
}

Ray& Ray::lookAt(const Vector3& v) {
    this->direction.copy(v).sub(this->origin).normalize();
    return *this;
}

Ray& Ray::recast(HIGH_PRECISION t) {
    Vector3 _vector;
    this->origin.copy(this->at(t, _vector));
    return *this;
}

Vector3& Ray::closestPointToPoint(const Vector3& point, Vector3& target) const {
    target.subVectors(point, this->origin);
    const auto directionDistance = target.dot(this->direction);
    if (directionDistance < 0) {
        return target.copy(this->origin);
    }
    return target.copy(this->origin).addScaledVector(this->direction, directionDistance);
}

HIGH_PRECISION Ray::distanceToPoint(const Vector3& point) const {
    return std::sqrt(this->distanceSqToPoint(point));
}

HIGH_PRECISION Ray::distanceSqToPoint(const Vector3& point) const {
    Vector3 _vector;
    const auto directionDistance = _vector.subVectors(point, this->origin).dot(this->direction);
    if (directionDistance < 0) {
        return this->origin.distanceToSquared(point);
    }
    _vector.copy(this->origin).addScaledVector(this->direction, directionDistance);
    return _vector.distanceToSquared(point);
}

HIGH_PRECISION Ray::distanceSqToSegment(const Vector3& v0, const Vector3& v1,
                                        Vector3* optionalPointOnRay,
                                        Vector3* optionalPointOnSegment) const {
    Vector3 _segCenter, _segDir, _diff;
    _segCenter.copy(v0).add(v1).multiplyScalar(0.5);
    _segDir.copy(v1).sub(v0).normalize();
    _diff.copy(this->origin).sub(_segCenter);
    const auto segExtent = v0.distanceTo(v1) * 0.5;
    const auto a01 = -this->direction.dot(_segDir);
    const auto b0 = _diff.dot(this->direction);
    const auto b1 = -_diff.dot(_segDir);
    const auto c = _diff.lengthSq();
    const auto det = std::abs(1 - a01 * a01);
    HIGH_PRECISION s0, s1, sqrDist, extDet;
    if (det > 0) {
        s0 = a01 * b1 - b0;
        s1 = a01 * b0 - b1;
        extDet = segExtent * det;
        if (s0 >= 0) {
            if (s1 >= -extDet) {
                if (s1 <= extDet) {
                    const auto invDet = 1 / det;
                    s0 *= invDet;
                    s1 *= invDet;
                    sqrDist = s0 * (s0 + a01 * s1 + 2 * b0) + s1 * (a01 * s0 + s1 + 2 * b1) + c;
                } else {
                    s1 = segExtent;
                    s0 = MathUtils::max(0, -(a01 * s1 + b0));
                    sqrDist = -s0 * s0 + s1 * (s1 + 2 * b1) + c;
                }
            } else {
                s1 = -segExtent;
                s0 = MathUtils::max(0, -(a01 * s1 + b0));
                sqrDist = -s0 * s0 + s1 * (s1 + 2 * b1) + c;
            }
        } else {
            if (s1 <= -extDet) {
                s0 = MathUtils::max(0, -(-a01 * segExtent + b0));
                s1 = (s0 > 0) ? -segExtent
                              : MathUtils::min(MathUtils::max(-segExtent, -b1), segExtent);
                sqrDist = -s0 * s0 + s1 * (s1 + 2 * b1) + c;
            } else if (s1 <= extDet) {
                s0 = 0;
                s1 = MathUtils::min(MathUtils::max(-segExtent, -b1), segExtent);
                sqrDist = s1 * (s1 + 2 * b1) + c;
            } else {
                s0 = MathUtils::max(0, -(a01 * segExtent + b0));
                s1 = (s0 > 0) ? segExtent
                              : MathUtils::min(MathUtils::max(-segExtent, -b1), segExtent);
                sqrDist = -s0 * s0 + s1 * (s1 + 2 * b1) + c;
            }
        }
    } else {
        s1 = (a01 > 0) ? -segExtent : segExtent;
        s0 = MathUtils::max(0, -(a01 * s1 + b0));
        sqrDist = -s0 * s0 + s1 * (s1 + 2 * b1) + c;
    }
    if (optionalPointOnRay) {
        optionalPointOnRay->copy(this->origin).addScaledVector(this->direction, s0);
    }
    if (optionalPointOnSegment) {
        optionalPointOnSegment->copy(_segCenter).addScaledVector(_segDir, s1);
    }
    return sqrDist;
}

Vector3* Ray::intersectSphere(const Sphere& sphere, Vector3& target) const {
    Vector3 _vector;
    if (sphere.radius < 0)
        return nullptr;
    _vector.subVectors(sphere.center, this->origin);
    const auto tca = _vector.dot(this->direction);
    const auto d2 = _vector.dot(_vector) - tca * tca;
    const auto radius2 = sphere.radius * sphere.radius;
    if (d2 > radius2)
        return nullptr;
    const auto thc = std::sqrt(radius2 - d2);
    const auto t0 = tca - thc;
    const auto t1 = tca + thc;
    if (t1 < 0)
        return nullptr;
    if (t0 < 0)
        return &this->at(t1, target);
    return &this->at(t0, target);
}

bool Ray::intersectsSphere(const Sphere& sphere) const {
    if (sphere.radius < 0)
        return false;
    return this->distanceSqToPoint(sphere.center) <= (sphere.radius * sphere.radius);
}

bool Ray::intersectsPlane(const Plane& plane) const {
    const auto distToPoint = plane.distanceToPoint(this->origin);
    if (distToPoint == 0) {
        return true;
    }
    const auto denominator = plane.normal.dot(this->direction);
    if (denominator * distToPoint < 0) {
        return true;
    }
    return false;
}

Vector3* Ray::intersectBox(const Box3& box, Vector3& target) const {
    HIGH_PRECISION tmin, tmax, tymin, tymax, tzmin, tzmax;
    const auto invdirx = 1 / this->direction.x(), invdiry = 1 / this->direction.y(),
               invdirz = 1 / this->direction.z();
    const auto& origin = this->origin;
    if (invdirx >= 0) {
        tmin = (box.min.x() - origin.x()) * invdirx;
        tmax = (box.max.x() - origin.x()) * invdirx;
    } else {
        tmin = (box.max.x() - origin.x()) * invdirx;
        tmax = (box.min.x() - origin.x()) * invdirx;
    }
    if (invdiry >= 0) {
        tymin = (box.min.y() - origin.y()) * invdiry;
        tymax = (box.max.y() - origin.y()) * invdiry;
    } else {
        tymin = (box.max.y() - origin.y()) * invdiry;
        tymax = (box.min.y() - origin.y()) * invdiry;
    }
    if ((tmin > tymax) || (tymin > tmax))
        return nullptr;
    if (tymin > tmin || std::isnan(tmin))
        tmin = tymin;
    if (tymax < tmax || std::isnan(tmax))
        tmax = tymax;
    if (invdirz >= 0) {
        tzmin = (box.min.z() - origin.z()) * invdirz;
        tzmax = (box.max.z() - origin.z()) * invdirz;
    } else {
        tzmin = (box.max.z() - origin.z()) * invdirz;
        tzmax = (box.min.z() - origin.z()) * invdirz;
    }
    if ((tmin > tzmax) || (tzmin > tmax))
        return nullptr;
    if (tzmin > tmin || tmin != tmin)
        tmin = tzmin;
    if (tzmax < tmax || tmax != tmax)
        tmax = tzmax;
    if (tmax < 0)
        return nullptr;
    return &this->at(tmin >= 0 ? tmin : tmax, target);
}

bool Ray::intersectsBox(const Box3& box) const {
    Vector3 _vector;
    return this->intersectBox(box, _vector) != nullptr;
}

Vector3* Ray::intersectTriangle(const Vector3& a, const Vector3& b, const Vector3& c,
                                bool backfaceCulling, Vector3& target) const {
    const auto& origin = this->origin;
    const auto& direction = this->direction;
    const auto dx = direction.x();
    const auto dy = direction.y();
    const auto dz = direction.z();
    const auto aox = a.x() - origin.x(), aoy = a.y() - origin.y(), aoz = a.z() - origin.z();
    const auto box = b.x() - origin.x(), boy = b.y() - origin.y(), boz = b.z() - origin.z();
    const auto cox = c.x() - origin.x(), coy = c.y() - origin.y(), coz = c.z() - origin.z();
    const auto adx = std::abs(dx), ady = std::abs(dy), adz = std::abs(dz);
    HIGH_PRECISION dkx, dky, dkz;
    HIGH_PRECISION akx, aky, akz, bkx, bky, bkz, ckx, cky, ckz;
    if (adx >= ady && adx >= adz) {
        dkz = dx;
        akz = aox;
        bkz = box;
        ckz = cox;
        if (dx >= 0) {
            dkx = dy;
            dky = dz;
            akx = aoy;
            aky = aoz;
            bkx = boy;
            bky = boz;
            ckx = coy;
            cky = coz;
        } else {
            dkx = dz;
            dky = dy;
            akx = aoz;
            aky = aoy;
            bkx = boz;
            bky = boy;
            ckx = coz;
            cky = coy;
        }
    } else if (ady >= adz) {
        dkz = dy;
        akz = aoy;
        bkz = boy;
        ckz = coy;
        if (dy >= 0) {
            dkx = dz;
            dky = dx;
            akx = aoz;
            aky = aox;
            bkx = boz;
            bky = box;
            ckx = coz;
            cky = cox;
        } else {
            dkx = dx;
            dky = dz;
            akx = aox;
            aky = aoz;
            bkx = box;
            bky = boz;
            ckx = cox;
            cky = coz;
        }
    } else {
        dkz = dz;
        akz = aoz;
        bkz = boz;
        ckz = coz;
        if (dz >= 0) {
            dkx = dx;
            dky = dy;
            akx = aox;
            aky = aoy;
            bkx = box;
            bky = boy;
            ckx = cox;
            cky = coy;
        } else {
            dkx = dy;
            dky = dx;
            akx = aoy;
            aky = aox;
            bkx = boy;
            bky = box;
            ckx = coy;
            cky = cox;
        }
    }
    if (dkz == 0)
        return nullptr;
    const auto sx = dkx / dkz, sy = dky / dkz, sz = 1 / dkz;
    const auto ax = akx - sx * akz, ay = aky - sy * akz;
    const auto bx = bkx - sx * bkz, by = bky - sy * bkz;
    const auto cx = ckx - sx * ckz, cy = cky - sy * ckz;
    const auto u = cx * by - cy * bx;
    const auto v = ax * cy - ay * cx;
    const auto w = bx * ay - by * ax;
    if (backfaceCulling) {
        if (u < 0 || v < 0 || w < 0)
            return nullptr;
    } else {
        if ((u < 0 || v < 0 || w < 0) && (u > 0 || v > 0 || w > 0))
            return nullptr;
    }
    const auto det = u + v + w;
    if (det == 0)
        return nullptr;
    const auto tScaled = sz * (u * akz + v * bkz + w * ckz);
    if (det > 0 ? tScaled < 0 : tScaled > 0)
        return nullptr;
    return &this->at(tScaled / det, target);
}

Ray& Ray::applyMatrix4(const Matrix4& matrix4) {
    this->origin.applyMatrix4(matrix4);
    this->direction.transformDirection(matrix4);
    return *this;
}

bool Ray::equals(const Ray& ray) const {
    return ray.origin.equals(this->origin) && ray.direction.equals(this->direction);
}

Ray::Ray(const Vector3& origin, const Vector3& direction) : origin(origin), direction(direction) {}
Ray Ray::clone() const {
    return Ray().copy(*this);
}
std::optional<HIGH_PRECISION> Ray::distanceToPlane(const Plane& plane) const {
    const auto denominator = plane.normal.dot(direction);
    if (denominator == 0) {
        if (plane.distanceToPoint(origin) == 0)
            return 0;
        return std::nullopt;
    }
    const auto t = -(origin.dot(plane.normal) + plane.constant) / denominator;
    if (t >= 0)
        return t;
    return std::nullopt;
}
Vector3* Ray::intersectPlane(const Plane& plane, Vector3& target) const {
    const auto t = distanceToPlane(plane);
    if (!t)
        return nullptr;
    return &at(*t, target);
}
