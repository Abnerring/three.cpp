#include "math/Vector2.h"
#include <algorithm>

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

void Vector2::set(HIGH_PRECISION x, HIGH_PRECISION y)
{
    m_x = x;
    m_y = y;
}

void Vector2::setScalar(HIGH_PRECISION scalar)
{
    m_x = scalar;
    m_y = scalar;
}

void Vector2::setX(HIGH_PRECISION x)
{
    m_x = x;
}

void Vector2::setY(HIGH_PRECISION y)
{
    m_y = y;
}

Vector2 Vector2::clone()
{
    return Vector2(m_x, m_y);
}

void Vector2::copy(const Vector2 &v)
{
    m_x = v.x();
    m_y = v.y();
}

void Vector2::add(const Vector2 &v)
{
    m_x += v.x();
    m_y += v.y();
}

void Vector2::addScalar(HIGH_PRECISION s)
{
    m_x += s;
    m_y += s;
}

void Vector2::addVectors(const Vector2 &a, const Vector2 &b)
{
    m_x = a.x() + b.x();
    m_y = a.y() + b.y();
}

void Vector2::addScaledVector(const Vector2 &v, HIGH_PRECISION s)
{
    m_x += v.x() * s;
    m_y += v.y() * s;
}

void Vector2::sub(const Vector2 &v)
{
    m_x -= v.x();
    m_y -= v.y();
}

void Vector2::subScalar(HIGH_PRECISION s)
{
    m_x -= s;
    m_y -= s;
}

void Vector2::subVectors(const Vector2 &a, const Vector2 &b)
{
    m_x = a.x() - b.x();
    m_y = a.y() - b.y();
}

void Vector2::multiply(const Vector2 &v)
{
    m_x *= v.x();
    m_y *= v.y();
}

void Vector2::multiplyScalar(HIGH_PRECISION scalar)
{
    m_x *= scalar;
    m_y *= scalar;
}

void Vector2::divide(const Vector2 &v)
{
    m_x /= v.x();
    m_y /= v.y();
}

// void Vector2::applyMatrix3(m) {}

void Vector2::divideScalar(HIGH_PRECISION scalar)
{
    multiplyScalar(1 / scalar);
}

void Vector2::min(const Vector2 &v)
{
    m_x = std::min(m_x, v.x());
    m_y = std::min(m_y, v.y());
}

void Vector2::max(const Vector2 &v)
{
    m_x = std::max(m_x, v.x());
    m_y = std::max(m_y, v.y());
}

void Vector2::clamp(const Vector2& min, const Vector2& max)
{
    m_x = std::max(min.x(), std::max(max.x(), m_x));
    m_y = std::max(min.y(), std::max(max.y(), m_y));
}

Vector2 Vector2::operator+(const Vector2 &v) const
{
    return Vector2(m_x + v.x(), m_y + v.y());
}

Vector2 Vector2::operator-(const Vector2 &v) const
{
    return Vector2(m_x - v.x(), m_y - v.y());
}

Vector2 Vector2::operator*(const Vector2 &v) const
{
    return Vector2(m_x * v.x(), m_y * v.y());
}

Vector2 Vector2::operator/(const Vector2 &v) const
{
    return Vector2(m_x / v.x(), m_y / v.y());
}
