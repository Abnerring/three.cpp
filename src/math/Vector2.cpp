#include "math/Vector2.h"
#include "math/Matrix3.h"
#include "math/MathUtils.h"
#include <algorithm>
#include <cmath>
#include <stdexcept>

Vector2::Vector2(HIGH_PRECISION x, HIGH_PRECISION y)
{
    m_x = x;
    m_y = y;
}

Vector2::~Vector2()
{
}

HIGH_PRECISION Vector2::x() const
{
    return m_x;
}
HIGH_PRECISION Vector2::y() const
{
    return m_y;
}

HIGH_PRECISION Vector2::width()
{
    return m_x;
}

void Vector2::width(HIGH_PRECISION value)
{
    m_x = value;
}

HIGH_PRECISION Vector2::height()
{
    return m_y;
}
void Vector2::height(HIGH_PRECISION value)
{
    m_y = value;
}

Vector2 &Vector2::set(HIGH_PRECISION x, HIGH_PRECISION y)
{
    m_x = x;
    m_y = y;
    return *this;
}

Vector2 &Vector2::setScalar(HIGH_PRECISION scalar)
{
    m_x = scalar;
    m_y = scalar;
    return *this;
}

Vector2 &Vector2::setX(HIGH_PRECISION x)
{
    m_x = x;
    return *this;
}

Vector2 &Vector2::setY(HIGH_PRECISION y)
{
    m_y = y;
    return *this;
}

Vector2 &Vector2::setComponent(size_t index, HIGH_PRECISION value)
{
    switch (index)
    {
    case 0:
        m_x = value;
        break;
    case 1:
        m_y = value;
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
        return m_x;
    case 1:
        return m_y;
    default:
        throw std::out_of_range("Index " + std::to_string(index) + " is out of range [0,1]");
    }
}

Vector2 Vector2::clone()
{
    return Vector2(m_x, m_y);
}

Vector2 &Vector2::copy(const Vector2 &v)
{
    m_x = v.x();
    m_y = v.y();
    return *this;
}

Vector2 &Vector2::add(const Vector2 &v)
{
    m_x += v.x();
    m_y += v.y();
    return *this;
}

Vector2 &Vector2::addScalar(HIGH_PRECISION s)
{
    m_x += s;
    m_y += s;
    return *this;
}

Vector2 &Vector2::addVectors(const Vector2 &a, const Vector2 &b)
{
    m_x = a.x() + b.x();
    m_y = a.y() + b.y();
    return *this;
}

Vector2 &Vector2::addScaledVector(const Vector2 &v, HIGH_PRECISION s)
{
    m_x += v.x() * s;
    m_y += v.y() * s;
    return *this;
}

Vector2 &Vector2::sub(const Vector2 &v)
{
    m_x -= v.x();
    m_y -= v.y();
    return *this;
}

Vector2 &Vector2::subScalar(HIGH_PRECISION s)
{
    m_x -= s;
    m_y -= s;
    return *this;
}

Vector2 &Vector2::subVectors(const Vector2 &a, const Vector2 &b)
{
    m_x = a.x() - b.x();
    m_y = a.y() - b.y();
    return *this;
}

Vector2 &Vector2::multiply(const Vector2 &v)
{
    m_x *= v.x();
    m_y *= v.y();
    return *this;
}

Vector2 &Vector2::multiplyScalar(HIGH_PRECISION scalar)
{
    m_x *= scalar;
    m_y *= scalar;
    return *this;
}

Vector2 &Vector2::divide(const Vector2 &v)
{
    m_x /= v.x();
    m_y /= v.y();
    return *this;
}

Vector2 &Vector2::divideScalar(HIGH_PRECISION scalar)
{
    return multiplyScalar(1 / scalar);
}

Vector2 &Vector2::applyMatrix3(const Matrix3 &m)
{
    auto &x = m_x, y = m_y;
    auto e = m.elements();

    m_x = e[0] * x + e[3] * y + e[6];
    m_y = e[1] * x + e[4] * y + e[7];
    return *this;
}

Vector2 &Vector2::min(const Vector2 &v)
{
    m_x = std::min(m_x, v.x());
    m_y = std::min(m_y, v.y());
    return *this;
}

Vector2 &Vector2::max(const Vector2 &v)
{
    m_x = std::max(m_x, v.x());
    m_y = std::max(m_y, v.y());
    return *this;
}

Vector2 &Vector2::clamp(const Vector2 &min, const Vector2 &max)
{
    m_x = std::max(min.x(), std::max(max.x(), m_x));
    m_y = std::max(min.y(), std::max(max.y(), m_y));
    return *this;
}

Vector2 &Vector2::clampScalar(HIGH_PRECISION minVal, HIGH_PRECISION maxVal)
{
    m_x = std::max(minVal, std::min(maxVal, m_x));
    m_y = std::max(minVal, std::min(maxVal, m_y));
    return *this;
}

Vector2 &Vector2::clampLength(HIGH_PRECISION min, HIGH_PRECISION max)
{
    auto length = this->length();
    this->divideScalar(length || 1);
    multiplyScalar(std::max(min, std::min(max, length)));
    return *this;
}

Vector2 &Vector2::floor()
{
    m_x = std::floor(m_x);
    m_y = std::floor(m_y);
    return *this;
}

Vector2 &Vector2::ceil()
{
    m_x = std::ceil(m_x);
    m_y = std::ceil(m_y);
    return *this;
}

Vector2 &Vector2::round()
{
    m_x = std::round(m_x);
    m_y = std::round(m_y);
    return *this;
}

Vector2 &Vector2::roundToZero()
{
    m_x = std::trunc(m_x);
    m_y = std::trunc(m_y);
    return *this;
}

Vector2 &Vector2::negate()
{
    m_x = -m_x;
    m_y = -m_y;
    return *this;
}

HIGH_PRECISION Vector2::dot(const Vector2 &v)
{
    return m_x * v.x() + m_y * v.y();
}

HIGH_PRECISION Vector2::cross(const Vector2 &v)
{
    return m_x * v.y() - m_y * v.x();
}

HIGH_PRECISION Vector2::lengthSq() const
{
    return m_x * m_x + m_y * m_y;
}

HIGH_PRECISION Vector2::length() const
{
    return std::sqrtf(m_x * m_x + m_y * m_y);
}

HIGH_PRECISION Vector2::manhattanLength()
{
    return std::abs(m_x) + std::abs(m_y);
}

Vector2 &Vector2::normalize()
{
    return this->divideScalar(this->length() || 1);
}

Vector2 Vector2::getNormal()
{
    Vector2 vec(m_x, m_y);
    return vec.divideScalar(vec.length() || 1);
}

HIGH_PRECISION Vector2::angle()
{
    // computes the angle in radians with respect to the positive x-axis
    auto angle = std::atan2(-m_y, -m_x) + MATH_PI;
    return angle;
}

HIGH_PRECISION Vector2::angleTo(const Vector2 &v)
{
    auto denominator = std::sqrt(this->lengthSq() * v.lengthSq());

    if (denominator == 0)
        return MATH_PI / 2;

    auto theta = this->dot(v) / denominator;

    // clamp, to handle numerical problems
    return std::acos(MathUtils::clamp(theta, -1, 1));
}

HIGH_PRECISION Vector2::distanceTo(const Vector2 &v)
{
    return std::sqrt(this->distanceToSquared(v));
}

HIGH_PRECISION Vector2::distanceToSquared(const Vector2 &v)
{
    auto dx = m_x - v.x();
    auto dy = m_y - v.y();
    return dx * dx + dy * dy;
}

HIGH_PRECISION Vector2::manhattanDistanceTo(const Vector2 &v)
{
    return std::abs(m_x - v.x()) + std::abs(m_y - v.y());
}

Vector2 &Vector2::setLength(HIGH_PRECISION length)
{
    return this->normalize().multiplyScalar(length);
}

Vector2 &Vector2::lerp(const Vector2 &v, HIGH_PRECISION alpha)
{
    m_x += (v.x() - m_x) * alpha;
    m_y += (v.y() - m_y) * alpha;
    return *this;
}

Vector2 &Vector2::lerpVectors(const Vector2 &v1, const Vector2 &v2, HIGH_PRECISION alpha)
{
    m_x = v1.x() + (v2.x() - v1.x()) * alpha;
    m_y = v1.y() + (v2.y() - v1.y()) * alpha;
    return *this;
}

bool Vector2::equals(const Vector2 &v, float epsilon) const
{
    const bool x_equal = std::abs(v.x() - m_x) <= epsilon;
    const bool y_equal = std::abs(v.y() - m_y) <= epsilon;
    return x_equal && y_equal;
}

Vector2 &Vector2::fromArray(const std::vector<HIGH_PRECISION> &array, size_t offset)
{
    m_x = array[offset];
    m_y = array[offset + 1];
    return *this;
}

std::vector<HIGH_PRECISION> &Vector2::toArray(std::vector<HIGH_PRECISION> &array, size_t offset)
{
    array[offset] = m_x;
    array[offset + 1] = m_y;
    return array;
}

Vector2 &Vector2::rotateAround(const Vector2 &center, float angle)
{
    auto c = std::cos(angle);
    auto s = std::sin(angle);

    auto x = m_x - center.x();
    auto y = m_y - center.y();

    m_x = x * c - y * s + center.x();
    m_y = x * s + y * c + center.y();
    return *this;
}

Vector2 &Vector2::random()
{
    m_x = DIST_DOUBLE(RANDOM_ENGINE);
    m_y = DIST_DOUBLE(RANDOM_ENGINE);
    return *this;
}

Vector2 Vector2::operator+(const Vector2 &v)
{
    auto _vec2 = clone();
    return _vec2.add(v);
}

Vector2 Vector2::operator-(const Vector2 &v)
{
    auto _vec2 = clone();
    return _vec2.sub(v);
}

Vector2 Vector2::operator*(const Vector2 &v)
{
    auto _vec2 = clone();
    return _vec2.multiply(v);
}

Vector2 Vector2::operator/(const Vector2 &v)
{
    auto _vec2 = clone();
    return _vec2.divide(v);
}

void Vector2::operator=(const Vector2 &v)
{
    this->copy(v);
}

bool Vector2::operator==(const Vector2 &v) const
{
    return this->equals(v);
}

HIGH_PRECISION Vector2::operator[](size_t index) const
{
    return this->getComponent(index);
}
