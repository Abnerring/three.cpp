#include "math/Vector3.h"
#include "math/JSNumberMath.h"
#include "cameras/Camera.h"
#include "math/JSNumberMath.h"
#include "core/VertexAttribute.h"
#include "math/JSNumberMath.h"
#include "math/Color.h"
#include "math/JSNumberMath.h"
#include "math/Cylindrical.h"
#include "math/JSNumberMath.h"
#include "math/Euler.h"
#include "math/JSNumberMath.h"
#include "math/MathUtils.h"
#include "math/JSNumberMath.h"
#include "math/Matrix3.h"
#include "math/JSNumberMath.h"
#include "math/Matrix4.h"
#include "math/JSNumberMath.h"
#include "math/Quaternion.h"
#include "math/JSNumberMath.h"
#include "math/Spherical.h"
#include "math/JSNumberMath.h"
#include <algorithm>
#include <stdexcept>
#include <string>

Vector3::Vector3(HIGH_PRECISION x, HIGH_PRECISION y, HIGH_PRECISION z) {
    this->set(x, y, z);
}

Vector3::~Vector3() {
    // Destructor implementation (if needed)
}

Vector3& Vector3::set(HIGH_PRECISION x, HIGH_PRECISION y, HIGH_PRECISION z) {
    this->x = x;
    this->y = y;
    this->z = z;
    return *this;
}

Vector3& Vector3::setScalar(HIGH_PRECISION scalar) {
    this->x = scalar;
    this->y = scalar;
    this->z = scalar;
    return *this;
}

Vector3& Vector3::setX(HIGH_PRECISION x) {
    this->x = x;
    return *this;
}
Vector3& Vector3::setY(HIGH_PRECISION y) {
    this->y = y;
    return *this;
}
Vector3& Vector3::setZ(HIGH_PRECISION z) {
    this->z = z;
    return *this;
}

Vector3& Vector3::setComponent(size_t index, HIGH_PRECISION value) {
    switch (index) {
    case 0:
        this->x = value;
        break;
    case 1:
        this->y = value;
        break;
    case 2:
        this->z = value;
        break;
    default:
        throw std::out_of_range("Index " + std::to_string(index) + " is out of range [0,2]");
    }
    return *this;
}

HIGH_PRECISION Vector3::getComponent(size_t index) const {
    switch (index) {
    case 0:
        return this->x;
    case 1:
        return this->y;
    case 2:
        return this->z;
    default:
        throw std::out_of_range("Index " + std::to_string(index) + " is out of range [0,2]");
    }
}

Vector3 Vector3::clone() const {
    return Vector3(this->x, this->y, this->z);
}

Vector3& Vector3::copy(const Vector3& v) {
    this->x = v.x;
    this->y = v.y;
    this->z = v.z;
    return *this;
}

Vector3& Vector3::add(const Vector3& v) {
    this->x += v.x;
    this->y += v.y;
    this->z += v.z;
    return *this;
}

Vector3& Vector3::addScalar(HIGH_PRECISION s) {
    this->x += s;
    this->y += s;
    this->z += s;
    return *this;
}

Vector3& Vector3::addVectors(const Vector3& a, const Vector3& b) {
    this->x = a.x + b.x;
    this->y = a.y + b.y;
    this->z = a.z + b.z;
    return *this;
}

Vector3& Vector3::addScaledVector(const Vector3& v, HIGH_PRECISION s) {
    this->x += v.x * s;
    this->y += v.y * s;
    this->z += v.z * s;
    return *this;
}

Vector3& Vector3::sub(const Vector3& v) {
    this->x -= v.x;
    this->y -= v.y;
    this->z -= v.z;
    return *this;
}

Vector3& Vector3::subScalar(HIGH_PRECISION s) {
    this->x -= s;
    this->y -= s;
    this->z -= s;
    return *this;
}

Vector3& Vector3::subVectors(const Vector3& a, const Vector3& b) {
    this->x = a.x - b.x;
    this->y = a.y - b.y;
    this->z = a.z - b.z;
    return *this;
}

Vector3& Vector3::multiply(const Vector3& v) {
    this->x *= v.x;
    this->y *= v.y;
    this->z *= v.z;
    return *this;
}

Vector3& Vector3::multiplyScalar(HIGH_PRECISION scalar) {
    this->x *= scalar;
    this->y *= scalar;
    this->z *= scalar;
    return *this;
}

Vector3& Vector3::multiplyVectors(const Vector3& a, const Vector3& b) {
    this->x = a.x * b.x;
    this->y = a.y * b.y;
    this->z = a.z * b.z;
    return *this;
}

Vector3& Vector3::applyEuler(const Euler& euler) {
    Quaternion q;
    q.setFromEuler(euler);
    applyQuaternion(q);
    return *this;
}

Vector3& Vector3::applyAxisAngle(const Vector3& axis, HIGH_PRECISION angle) {
    Quaternion q;
    q.setFromAxisAngle(axis, angle);
    applyQuaternion(q);
    return *this;
}

Vector3& Vector3::applyMatrix3(const Matrix3& m) {
    auto x = this->x, y = this->y, z = this->z;
    auto e = m.elements();

    this->x = e[0] * x + e[3] * y + e[6] * z;
    this->y = e[1] * x + e[4] * y + e[7] * z;
    this->z = e[2] * x + e[5] * y + e[8] * z;
    return *this;
}

Vector3& Vector3::applyNormalMatrix(const Matrix3& m) {
    this->applyMatrix3(m);
    this->normalize();
    return *this;
}

Vector3& Vector3::applyMatrix4(const Matrix4& m) {
    const auto x = this->x, y = this->y, z = this->z;
    const auto e = m.elements();
    const auto w = 1.0 / (e[3] * x + e[7] * y + e[11] * z + e[15]);
    this->x = (e[0] * x + e[4] * y + e[8] * z + e[12]) * w;
    this->y = (e[1] * x + e[5] * y + e[9] * z + e[13]) * w;
    this->z = (e[2] * x + e[6] * y + e[10] * z + e[14]) * w;
    return *this;
}

Vector3& Vector3::applyQuaternion(const Quaternion& q) {
    const auto vx = this->x, vy = this->y, vz = this->z;
    const auto qx = q.x, qy = q.y, qz = q.z, qw = q.w;
    const auto tx = 2 * (qy * vz - qz * vy);
    const auto ty = 2 * (qz * vx - qx * vz);
    const auto tz = 2 * (qx * vy - qy * vx);
    this->x = vx + qw * tx + qy * tz - qz * ty;
    this->y = vy + qw * ty + qz * tx - qx * tz;
    this->z = vz + qw * tz + qx * ty - qy * tx;
    return *this;
}

Vector3& Vector3::transformDirection(const Matrix4& m) {
    auto x = this->x, y = this->y, z = this->z;
    auto e = m.elements();

    this->x = e[0] * x + e[4] * y + e[8] * z;
    this->y = e[1] * x + e[5] * y + e[9] * z;
    this->z = e[2] * x + e[6] * y + e[10] * z;
    return normalize();
}

Vector3& Vector3::divide(const Vector3& v) {
    this->x /= v.x;
    this->y /= v.y;
    this->z /= v.z;
    return *this;
}

Vector3& Vector3::divideScalar(HIGH_PRECISION scalar) {
    this->multiplyScalar(1 / scalar);
    return *this;
}

Vector3& Vector3::min(const Vector3& v) {
    this->x = MathUtils::min(this->x, v.x);
    this->y = MathUtils::min(this->y, v.y);
    this->z = MathUtils::min(this->z, v.z);
    return *this;
}

Vector3& Vector3::max(const Vector3& v) {
    this->x = MathUtils::max(this->x, v.x);
    this->y = MathUtils::max(this->y, v.y);
    this->z = MathUtils::max(this->z, v.z);
    return *this;
}

Vector3& Vector3::clamp(const Vector3& min, const Vector3& max) {
    // assumes min < max, componentwise
    this->x = MathUtils::clamp(this->x, min.x, max.x);
    this->y = MathUtils::clamp(this->y, min.y, max.y);
    this->z = MathUtils::clamp(this->z, min.z, max.z);
    return *this;
}

Vector3& Vector3::clampScalar(HIGH_PRECISION minVal, HIGH_PRECISION maxVal) {
    this->x = MathUtils::clamp(this->x, minVal, maxVal);
    this->y = MathUtils::clamp(this->y, minVal, maxVal);
    this->z = MathUtils::clamp(this->z, minVal, maxVal);
    return *this;
}

Vector3& Vector3::clampLength(HIGH_PRECISION min, HIGH_PRECISION max) {
    auto length = this->length();
    divideScalar((length == 0 || std::isnan(length)) ? 1.0 : length);
    multiplyScalar(MathUtils::clamp(length, min, max));
    return *this;
}

Vector3& Vector3::floor() {
    this->x = std::floor(this->x);
    this->y = std::floor(this->y);
    this->z = std::floor(this->z);
    return *this;
}

Vector3& Vector3::ceil() {
    this->x = std::ceil(this->x);
    this->y = std::ceil(this->y);
    this->z = std::ceil(this->z);
    return *this;
}

Vector3& Vector3::round() {
    this->x = MathUtils::round(this->x);
    this->y = MathUtils::round(this->y);
    this->z = MathUtils::round(this->z);
    return *this;
}

Vector3& Vector3::roundToZero() {
    this->x = std::trunc(this->x);
    this->y = std::trunc(this->y);
    this->z = std::trunc(this->z);
    return *this;
}

Vector3& Vector3::negate() {
    this->x = -this->x;
    this->y = -this->y;
    this->z = -this->z;
    return *this;
}

HIGH_PRECISION Vector3::dot(const Vector3& v) const {
    return this->x * v.x + this->y * v.y + this->z * v.z;
}

HIGH_PRECISION Vector3::lengthSq() const {
    return this->x * this->x + this->y * this->y + this->z * this->z;
}

HIGH_PRECISION Vector3::length() const {
    return std::sqrt(this->x * this->x + this->y * this->y + this->z * this->z);
}

HIGH_PRECISION Vector3::manhattanLength() const {
    return std::abs(this->x) + std::abs(this->y) + std::abs(this->z);
}

Vector3& Vector3::normalize() {
    auto length = this->length();
    this->divideScalar((length == 0 || std::isnan(length)) ? 1.0 : length);
    return *this;
}

Vector3 Vector3::normalized() {
    Vector3 vec(this->x, this->y, this->z);
    auto length = vec.length();
    vec.divideScalar((length == 0 || std::isnan(length)) ? 1.0 : length);
    return vec;
}

Vector3& Vector3::setLength(HIGH_PRECISION length) {
    normalize();
    multiplyScalar(length);
    return *this;
}

Vector3& Vector3::lerp(const Vector3& v, HIGH_PRECISION alpha) {
    this->x += (v.x - this->x) * alpha;
    this->y += (v.y - this->y) * alpha;
    this->z += (v.z - this->z) * alpha;
    return *this;
}

Vector3& Vector3::lerpVectors(const Vector3& v1, const Vector3& v2, HIGH_PRECISION alpha) {
    this->x = v1.x + (v2.x - v1.x) * alpha;
    this->y = v1.y + (v2.y - v1.y) * alpha;
    this->z = v1.z + (v2.z - v1.z) * alpha;
    return *this;
}

Vector3& Vector3::cross(const Vector3& v) {
    crossVectors(*this, v);
    return *this;
}

Vector3& Vector3::crossVectors(const Vector3& a, const Vector3& b) {
    auto ax = a.x, ay = a.y, az = a.z;
    auto bx = b.x, by = b.y, bz = b.z;

    this->x = ay * bz - az * by;
    this->y = az * bx - ax * bz;
    this->z = ax * by - ay * bx;
    return *this;
}

Vector3& Vector3::projectOnVector(const Vector3& v) {
    auto denominator = v.lengthSq();

    if (denominator == 0)
        return this->set(0, 0, 0);

    auto scalar = v.dot(*this) / denominator;
    copy(v);
    multiplyScalar(scalar);
    return *this;
}

Vector3& Vector3::projectOnPlane(const Vector3& planeNormal) {
    auto _vector = Vector3(0, 0, 0);
    _vector.copy(*this);
    _vector.projectOnVector(planeNormal);
    this->sub(_vector);
    return *this;
}

Vector3& Vector3::reflect(const Vector3& normal) {
    auto _vector = Vector3(0, 0, 0);
    _vector.copy(normal);
    _vector.multiplyScalar(2 * this->dot(normal));
    sub(_vector);
    return *this;
}

HIGH_PRECISION Vector3::angleTo(const Vector3& v) const {
    auto denominator = std::sqrt(lengthSq() * v.lengthSq());

    if (denominator == 0)
        return MATH_PI / 2;

    auto theta = dot(v) / denominator;

    // clamp, to handle numerical problems

    return JSNumberMath::acos(MathUtils::clamp(theta, -1, 1));
}

HIGH_PRECISION Vector3::distanceTo(const Vector3& v) const {
    return std::sqrt(distanceToSquared(v));
}

HIGH_PRECISION Vector3::distanceToSquared(const Vector3& v) const {
    auto dx = this->x - v.x, dy = this->y - v.y, dz = this->z - v.z;

    return dx * dx + dy * dy + dz * dz;
}

HIGH_PRECISION Vector3::manhattanDistanceTo(const Vector3& v) const {
    return std::abs(this->x - v.x) + std::abs(this->y - v.y) + std::abs(this->z - v.z);
}

Vector3& Vector3::setFromSphericalCoords(HIGH_PRECISION radius, HIGH_PRECISION phi,
                                         HIGH_PRECISION theta) {
    auto sinPhiRadius = JSNumberMath::sin(phi) * radius;

    this->x = sinPhiRadius * JSNumberMath::sin(theta);
    this->y = JSNumberMath::cos(phi) * radius;
    this->z = sinPhiRadius * JSNumberMath::cos(theta);
    return *this;
}

Vector3& Vector3::setFromCylindricalCoords(HIGH_PRECISION radius, HIGH_PRECISION theta,
                                           HIGH_PRECISION y) {
    this->x = radius * JSNumberMath::sin(theta);
    this->y = y;
    this->z = radius * JSNumberMath::cos(theta);
    return *this;
}

Vector3& Vector3::setFromMatrixColumn(const Matrix4& m, size_t index) {
    auto ele_list = MathUtils::convertSpanToVector(m.elements());
    fromArray(ele_list, index * 4);
    return *this;
}

Vector3& Vector3::setFromMatrix3Column(const Matrix3& m, size_t index) {
    auto ele_list = MathUtils::convertSpanToVector(m.elements());
    fromArray(ele_list, index * 3);
    return *this;
}

bool Vector3::equals(const Vector3& v, HIGH_PRECISION epsilon) const {
    if (epsilon == 0)
        return this->x == v.x && this->y == v.y && this->z == v.z;
    return std::abs(this->x - v.x) <= epsilon && std::abs(this->y - v.y) <= epsilon &&
           std::abs(this->z - v.z) <= epsilon;
}

Vector3& Vector3::fromArray(const std::vector<HIGH_PRECISION>& array, size_t offset) {
    this->x = array[offset];
    this->y = array[offset + 1];
    this->z = array[offset + 2];
    return *this;
}

std::vector<HIGH_PRECISION>& Vector3::toArray(std::vector<HIGH_PRECISION>& array,
                                              size_t offset) const {
    if (array.size() < offset + 3)
        array.resize(offset + 3);
    array[offset] = this->x;
    array[offset + 1] = this->y;
    array[offset + 2] = this->z;
    return array;
}

Vector3& Vector3::random() {
    this->x = DIST_DOUBLE(RANDOM_ENGINE);
    this->y = DIST_DOUBLE(RANDOM_ENGINE);
    this->z = DIST_DOUBLE(RANDOM_ENGINE);
    return *this;
}

Vector3& Vector3::randomDirection() {
    // https://mathworld.wolfram.com/SpherePointPicking.html

    auto theta = DIST_DOUBLE(RANDOM_ENGINE) * MATH_PI * 2;
    auto u = DIST_DOUBLE(RANDOM_ENGINE) * 2 - 1;
    auto c = std::sqrt(1 - u * u);

    this->x = c * JSNumberMath::cos(theta);
    this->y = u;
    this->z = c * JSNumberMath::sin(theta);
    return *this;
}

Vector3& Vector3::setFromMatrixPosition(const Matrix4& m) {
    auto e = m.elements();
    this->x = e[12];
    this->y = e[13];
    this->z = e[14];
    return *this;
}

Vector3& Vector3::setFromMatrixScale(const Matrix4& m) {
    auto e = m.elements();
    this->x = std::sqrt(e[0] * e[0] + e[1] * e[1] + e[2] * e[2]);
    this->y = std::sqrt(e[4] * e[4] + e[5] * e[5] + e[6] * e[6]);
    this->z = std::sqrt(e[8] * e[8] + e[9] * e[9] + e[10] * e[10]);
    return *this;
}

Vector3& Vector3::setFromEuler(const Euler& euler) {
    return set(euler.x, euler.y, euler.z);
}

Vector3& Vector3::setFromColor(const Color& c) {
    this->x = c.r;
    this->y = c.g;
    this->z = c.b;
    return *this;
}

Vector3 Vector3::operator+(const Vector3& v) const {
    Vector3 _vector;
    _vector.copy(*this);
    _vector.add(v);
    return _vector;
}

Vector3 Vector3::operator-(const Vector3& v) const {
    Vector3 _vector;
    _vector.copy(*this);
    _vector.sub(v);
    return _vector;
}

Vector3 Vector3::operator*(const Vector3& v) const {
    Vector3 _vector;
    _vector.copy(*this);
    _vector.multiply(v);
    return _vector;
}

Vector3 Vector3::operator/(const Vector3& v) const {
    Vector3 _vector;
    _vector.copy(*this);
    _vector.divide(v);
    return _vector;
}

Vector3& Vector3::operator=(const Vector3& v) {
    this->copy(v);
    return *this;
}

bool Vector3::operator==(const Vector3& v) const {
    return this->equals(v);
}

HIGH_PRECISION Vector3::operator[](size_t index) const {
    return this->getComponent(index);
}

std::vector<HIGH_PRECISION> Vector3::toArray() const {
    std::vector<HIGH_PRECISION> array;
    toArray(array);
    return array;
}

Vector3& Vector3::set(HIGH_PRECISION x, HIGH_PRECISION y) {
    return set(x, y, this->z);
}

Vector3& Vector3::fromBufferAttribute(const VertexAttribute& attribute, double index) {
    x = attribute.getX(index);
    y = attribute.getY(index);
    z = attribute.getZ(index);
    return *this;
}

Vector3& Vector3::project(const Camera& camera) {
    return applyMatrix4(camera.matrixWorldInverse).applyMatrix4(camera.projectionMatrix);
}
Vector3& Vector3::unproject(const Camera& camera) {
    return applyMatrix4(camera.projectionMatrixInverse).applyMatrix4(camera.matrixWorld);
}

Vector3& Vector3::setFromSpherical(const Spherical& spherical) {
    return setFromSphericalCoords(spherical.radius, spherical.phi, spherical.theta);
}
Vector3& Vector3::setFromCylindrical(const Cylindrical& cylindrical) {
    return setFromCylindricalCoords(cylindrical.radius, cylindrical.theta, cylindrical.y);
}
