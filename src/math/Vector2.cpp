#include "math/Vector2.h"
#include "math/JSNumberMath.h"
#include "core/VertexAttribute.h"
#include "math/JSNumberMath.h"
#include "math/Matrix3.h"
#include "math/JSNumberMath.h"
#include "math/MathUtils.h"
#include "math/JSNumberMath.h"
#include <algorithm>
#include <cmath>
#include <stdexcept>

Vector2::Vector2(HIGH_PRECISION x, HIGH_PRECISION y)
{
    this->x = x;
    this->y = y;
}

Vector2::~Vector2()
{
}

Vector2 &Vector2::set(HIGH_PRECISION x, HIGH_PRECISION y)
{
    this->x = x;
    this->y = y;
    return *this;
}

Vector2 &Vector2::setScalar(HIGH_PRECISION scalar)
{
    this->x = scalar;
    this->y = scalar;
    return *this;
}

Vector2 &Vector2::setX(HIGH_PRECISION x)
{
    this->x = x;
    return *this;
}

Vector2 &Vector2::setY(HIGH_PRECISION y)
{
    this->y = y;
    return *this;
}

Vector2 &Vector2::setComponent(size_t index, HIGH_PRECISION value)
{
    switch (index)
    {
    case 0:
        this->x = value;
        break;
    case 1:
        this->y = value;
        break;
    default:
        throw std::out_of_range("Index " + std::to_string(index) + " is out of range [0,1]");
    }
    return *this;
}

HIGH_PRECISION Vector2::getComponent(size_t index) const
{
    switch (index)
    {
    case 0:
        return this->x;
    case 1:
        return this->y;
    default:
        throw std::out_of_range("Index " + std::to_string(index) + " is out of range [0,1]");
    }
}

Vector2 Vector2::clone() const
{
    return Vector2(this->x, this->y);
}

Vector2 &Vector2::copy(const Vector2 &v)
{
    this->x = v.x;
    this->y = v.y;
    return *this;
}

Vector2 &Vector2::add(const Vector2 &v)
{
    this->x += v.x;
    this->y += v.y;
    return *this;
}

Vector2 &Vector2::addScalar(HIGH_PRECISION s)
{
    this->x += s;
    this->y += s;
    return *this;
}

Vector2 &Vector2::addVectors(const Vector2 &a, const Vector2 &b)
{
    this->x = a.x + b.x;
    this->y = a.y + b.y;
    return *this;
}

Vector2 &Vector2::addScaledVector(const Vector2 &v, HIGH_PRECISION s)
{
    this->x += v.x * s;
    this->y += v.y * s;
    return *this;
}

Vector2 &Vector2::sub(const Vector2 &v)
{
    this->x -= v.x;
    this->y -= v.y;
    return *this;
}

Vector2 &Vector2::subScalar(HIGH_PRECISION s)
{
    this->x -= s;
    this->y -= s;
    return *this;
}

Vector2 &Vector2::subVectors(const Vector2 &a, const Vector2 &b)
{
    this->x = a.x - b.x;
    this->y = a.y - b.y;
    return *this;
}

Vector2 &Vector2::multiply(const Vector2 &v)
{
    this->x *= v.x;
    this->y *= v.y;
    return *this;
}

Vector2 &Vector2::multiplyScalar(HIGH_PRECISION scalar)
{
    this->x *= scalar;
    this->y *= scalar;
    return *this;
}

Vector2 &Vector2::divide(const Vector2 &v)
{
    this->x /= v.x;
    this->y /= v.y;
    return *this;
}

Vector2 &Vector2::divideScalar(HIGH_PRECISION scalar)
{
    return multiplyScalar(1 / scalar);
}

Vector2 &Vector2::applyMatrix3(const Matrix3 &m)
{
    const auto x = this->x, y = this->y;
    auto e = m.elements();

    this->x = e[0] * x + e[3] * y + e[6];
    this->y = e[1] * x + e[4] * y + e[7];
    return *this;
}

Vector2 &Vector2::min(const Vector2 &v)
{
    this->x = MathUtils::min(this->x, v.x);
    this->y = MathUtils::min(this->y, v.y);
    return *this;
}

Vector2 &Vector2::max(const Vector2 &v)
{
    this->x = MathUtils::max(this->x, v.x);
    this->y = MathUtils::max(this->y, v.y);
    return *this;
}

Vector2 &Vector2::clamp(const Vector2 &min, const Vector2 &max)
{
    this->x = MathUtils::clamp(this->x, min.x, max.x);
    this->y = MathUtils::clamp(this->y, min.y, max.y);
    return *this;
}

Vector2 &Vector2::clampScalar(HIGH_PRECISION minVal, HIGH_PRECISION maxVal)
{
    this->x = MathUtils::max(minVal, MathUtils::min(maxVal, this->x));
    this->y = MathUtils::max(minVal, MathUtils::min(maxVal, this->y));
    return *this;
}

Vector2 &Vector2::clampLength(HIGH_PRECISION min, HIGH_PRECISION max)
{
    auto length = this->length();
    this->divideScalar((length == 0 || std::isnan(length)) ? 1.0 : length);
    multiplyScalar(MathUtils::max(min, MathUtils::min(max, length)));
    return *this;
}

Vector2 &Vector2::floor()
{
    this->x = std::floor(this->x);
    this->y = std::floor(this->y);
    return *this;
}

Vector2 &Vector2::ceil()
{
    this->x = std::ceil(this->x);
    this->y = std::ceil(this->y);
    return *this;
}

Vector2 &Vector2::round()
{
    this->x = MathUtils::round(this->x);
    this->y = MathUtils::round(this->y);
    return *this;
}

Vector2 &Vector2::roundToZero()
{
    this->x = std::trunc(this->x);
    this->y = std::trunc(this->y);
    return *this;
}

Vector2 &Vector2::negate()
{
    this->x = -this->x;
    this->y = -this->y;
    return *this;
}

HIGH_PRECISION Vector2::dot(const Vector2 &v) const
{
    return this->x * v.x + this->y * v.y;
}

HIGH_PRECISION Vector2::cross(const Vector2 &v) const
{
    return this->x * v.y - this->y * v.x;
}

HIGH_PRECISION Vector2::lengthSq() const
{
    return this->x * this->x + this->y * this->y;
}

HIGH_PRECISION Vector2::length() const
{
    return std::sqrt(this->x * this->x + this->y * this->y);
}

HIGH_PRECISION Vector2::manhattanLength() const
{
    return std::abs(this->x) + std::abs(this->y);
}

Vector2 &Vector2::normalize()
{
    const auto len = length();
    return divideScalar((len == 0 || std::isnan(len)) ? 1.0 : len);
}

Vector2 Vector2::getNormal() const
{
    return clone().normalize();
}

HIGH_PRECISION Vector2::angle() const
{
    // computes the angle in radians with respect to the positive x-axis
    auto angle = JSNumberMath::atan2(-this->y, -this->x) + MATH_PI;
    return angle;
}

HIGH_PRECISION Vector2::angleTo(const Vector2 &v) const
{
    auto denominator = std::sqrt(this->lengthSq() * v.lengthSq());

    if (denominator == 0)
        return MATH_PI / 2;

    auto theta = this->dot(v) / denominator;

    // clamp, to handle numerical problems
    return JSNumberMath::acos(MathUtils::clamp(theta, -1, 1));
}

HIGH_PRECISION Vector2::distanceTo(const Vector2 &v) const
{
    return std::sqrt(this->distanceToSquared(v));
}

HIGH_PRECISION Vector2::distanceToSquared(const Vector2 &v) const
{
    auto dx = this->x - v.x;
    auto dy = this->y - v.y;
    return dx * dx + dy * dy;
}

HIGH_PRECISION Vector2::manhattanDistanceTo(const Vector2 &v) const
{
    return std::abs(this->x - v.x) + std::abs(this->y - v.y);
}

Vector2 &Vector2::setLength(HIGH_PRECISION length)
{
    return this->normalize().multiplyScalar(length);
}

Vector2 &Vector2::lerp(const Vector2 &v, HIGH_PRECISION alpha)
{
    this->x += (v.x - this->x) * alpha;
    this->y += (v.y - this->y) * alpha;
    return *this;
}

Vector2 &Vector2::lerpVectors(const Vector2 &v1, const Vector2 &v2, HIGH_PRECISION alpha)
{
    this->x = v1.x + (v2.x - v1.x) * alpha;
    this->y = v1.y + (v2.y - v1.y) * alpha;
    return *this;
}

bool Vector2::equals(const Vector2 &v, HIGH_PRECISION epsilon) const
{
    if (epsilon == 0) return this->x == v.x && this->y == v.y;
    return std::abs(this->x - v.x) <= epsilon && std::abs(this->y - v.y) <= epsilon;
}

Vector2 &Vector2::fromArray(const std::vector<HIGH_PRECISION> &array, size_t offset)
{
    this->x = array[offset];
    this->y = array[offset + 1];
    return *this;
}

std::vector<HIGH_PRECISION> &Vector2::toArray(std::vector<HIGH_PRECISION> &array, size_t offset) const
{
    if (array.size() < offset + 2) array.resize(offset + 2);
    array[offset] = this->x;
    array[offset + 1] = this->y;
    return array;
}

Vector2 &Vector2::rotateAround(const Vector2 &center, HIGH_PRECISION angle)
{
    auto c = JSNumberMath::cos(angle);
    auto s = JSNumberMath::sin(angle);

    auto x = this->x - center.x;
    auto y = this->y - center.y;

    this->x = x * c - y * s + center.x;
    this->y = x * s + y * c + center.y;
    return *this;
}

Vector2 &Vector2::random()
{
    this->x = DIST_DOUBLE(RANDOM_ENGINE);
    this->y = DIST_DOUBLE(RANDOM_ENGINE);
    return *this;
}

Vector2 Vector2::operator+(const Vector2 &v) const
{
    auto _vec2 = clone();
    return _vec2.add(v);
}

Vector2 Vector2::operator-(const Vector2 &v) const
{
    auto _vec2 = clone();
    return _vec2.sub(v);
}

Vector2 Vector2::operator*(const Vector2 &v) const
{
    auto _vec2 = clone();
    return _vec2.multiply(v);
}

Vector2 Vector2::operator/(const Vector2 &v) const
{
    auto _vec2 = clone();
    return _vec2.divide(v);
}

Vector2 &Vector2::operator=(const Vector2 &v)
{
    return this->copy(v);
}

bool Vector2::operator==(const Vector2 &v) const
{
    return this->equals(v);
}

HIGH_PRECISION Vector2::operator[](size_t index) const
{
    return this->getComponent(index);
}

std::vector<HIGH_PRECISION> Vector2::toArray() const
{
    std::vector<HIGH_PRECISION> array;
    toArray(array);
    return array;
}

Vector2 &Vector2::fromBufferAttribute(const VertexAttribute &attribute, double index)
{
    x = attribute.getX(index);
    y = attribute.getY(index);
    return *this;
}

Vector2::Vector2(const Vector2& source) : Vector2(source.x, source.y) {}
Vector2::Vector2(Vector2&& source) : Vector2(static_cast<const Vector2&>(source)) {}
