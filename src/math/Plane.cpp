#include "math/Plane.h"
#include "math/Box3.h"
#include "math/Line3.h"
#include "math/Matrix3.h"
#include "math/Matrix4.h"
#include "math/Sphere.h"
#include <cmath>

Plane& Plane::set(const Vector3& normal, HIGH_PRECISION constant) {
    this->normal.copy(normal);
    this->constant = constant;
    return *this;
}

Plane& Plane::setComponents(HIGH_PRECISION x, HIGH_PRECISION y, HIGH_PRECISION z,
                            HIGH_PRECISION w) {
    this->normal.set(x, y, z);
    this->constant = w;
    return *this;
}

Plane& Plane::setFromNormalAndCoplanarPoint(const Vector3& normal, const Vector3& point) {
    this->normal.copy(normal);
    this->constant = -point.dot(this->normal);
    return *this;
}

Plane& Plane::setFromCoplanarPoints(const Vector3& a, const Vector3& b, const Vector3& c) {
    Vector3 _vector1, _vector2;
    const auto normal = _vector1.subVectors(c, b).cross(_vector2.subVectors(a, b)).normalize();
    this->setFromNormalAndCoplanarPoint(normal, a);
    return *this;
}

Plane& Plane::copy(const Plane& plane) {
    this->normal.copy(plane.normal);
    this->constant = plane.constant;
    return *this;
}

Plane& Plane::normalize() {
    const auto inverseNormalLength = 1.0 / this->normal.length();
    this->normal.multiplyScalar(inverseNormalLength);
    this->constant *= inverseNormalLength;
    return *this;
}

Plane& Plane::negate() {
    this->constant *= -1;
    this->normal.negate();
    return *this;
}

HIGH_PRECISION Plane::distanceToPoint(const Vector3& point) const {
    return this->normal.dot(point) + this->constant;
}

HIGH_PRECISION Plane::distanceToSphere(const Sphere& sphere) const {
    return this->distanceToPoint(sphere.center) - sphere.radius;
}

Vector3& Plane::projectPoint(const Vector3& point, Vector3& target) const {
    return target.copy(point).addScaledVector(this->normal, -this->distanceToPoint(point));
}

Vector3* Plane::intersectLine(const Line3& line, Vector3& target, bool clampToLine) const {
    Vector3 _vector1;
    const auto direction = line.delta(_vector1);
    const auto denominator = this->normal.dot(direction);
    if (denominator == 0) {
        if (this->distanceToPoint(line.start) == 0) {
            return &target.copy(line.start);
        }
        return nullptr;
    }
    const auto t = -(line.start.dot(this->normal) + this->constant) / denominator;
    if ((clampToLine == true) && (t < 0 || t > 1)) {
        return nullptr;
    }
    return &target.copy(line.start).addScaledVector(direction, t);
}

bool Plane::intersectsLine(const Line3& line) const {
    const auto startSign = this->distanceToPoint(line.start);
    const auto endSign = this->distanceToPoint(line.end);
    return (startSign < 0 && endSign > 0) || (endSign < 0 && startSign > 0);
}

bool Plane::intersectsBox(const Box3& box) const {
    return box.intersectsPlane(*this);
}

bool Plane::intersectsSphere(const Sphere& sphere) const {
    return sphere.intersectsPlane(*this);
}

Vector3& Plane::coplanarPoint(Vector3& target) const {
    return target.copy(this->normal).multiplyScalar(-this->constant);
}

Plane& Plane::translate(const Vector3& offset) {
    this->constant -= offset.dot(this->normal);
    return *this;
}

bool Plane::equals(const Plane& plane) const {
    return plane.normal.equals(this->normal) && (plane.constant == this->constant);
}

Plane::Plane(const Vector3& normal, HIGH_PRECISION constant) : normal(normal), constant(constant) {}
Plane Plane::clone() const {
    return Plane().copy(*this);
}
Plane& Plane::applyMatrix4(const Matrix4& matrix, const Matrix3* optionalNormalMatrix) {
    Matrix3 scratchNormalMatrix;
    const Matrix3& normalMatrix =
        optionalNormalMatrix ? *optionalNormalMatrix : scratchNormalMatrix.getNormalMatrix(matrix);
    Vector3 referencePoint;
    coplanarPoint(referencePoint).applyMatrix4(matrix);
    auto& transformedNormal = normal.applyMatrix3(normalMatrix).normalize();
    constant = -referencePoint.dot(transformedNormal);
    return *this;
}
JsonValue Plane::toJSON() const {
    return JsonValue::Object{{"normal", JsonValue::Array{normal.x, normal.y, normal.z}},
                             {"constant", constant}};
}
Plane& Plane::fromJSON(const JsonValue& json) {
    const auto& array = json.at("normal").get<JsonValue::Array>();
    normal.set(array.at(0).get<double>(), array.at(1).get<double>(), array.at(2).get<double>());
    constant = json.at("constant").get<double>();
    return *this;
}
