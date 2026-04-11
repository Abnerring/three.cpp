#include "math/Vector4.h"
#include "math/Matrix4.h"
#include "math/MathUtils.h"
#include <algorithm>
#include <cmath>
#include <stdexcept>

Vector4::Vector4(HIGH_PRECISION x, HIGH_PRECISION y, HIGH_PRECISION z, HIGH_PRECISION w)
    : m_x(x), m_y(y), m_z(z), m_w(w)
{
}

Vector4::~Vector4()
{
}

HIGH_PRECISION Vector4::x() const { return m_x; }
HIGH_PRECISION Vector4::y() const { return m_y; }
HIGH_PRECISION Vector4::z() const { return m_z; }
HIGH_PRECISION Vector4::w() const { return m_w; }

Vector4 &Vector4::set(HIGH_PRECISION x, HIGH_PRECISION y, HIGH_PRECISION z, HIGH_PRECISION w)
{
    m_x = x;
    m_y = y;
    m_z = z;
    m_w = w;
    return *this;
}

Vector4 &Vector4::setScalar(HIGH_PRECISION scalar)
{
    m_x = m_y = m_z = m_w = scalar;
    return *this;
}

Vector4 &Vector4::setX(HIGH_PRECISION x) { m_x = x; return *this; }
Vector4 &Vector4::setY(HIGH_PRECISION y) { m_y = y; return *this; }
Vector4 &Vector4::setZ(HIGH_PRECISION z) { m_z = z; return *this; }
Vector4 &Vector4::setW(HIGH_PRECISION w) { m_w = w; return *this; }

Vector4 &Vector4::setComponent(size_t index, HIGH_PRECISION value)
{
    switch (index)
    {
    case 0: m_x = value; break;
    case 1: m_y = value; break;
    case 2: m_z = value; break;
    case 3: m_w = value; break;
    default: throw std::out_of_range("Index out of range [0,3]");
    }
    return *this;
}

HIGH_PRECISION Vector4::getComponent(size_t index) const
{
    switch (index)
    {
    case 0: return m_x;
    case 1: return m_y;
    case 2: return m_z;
    case 3: return m_w;
    default: throw std::out_of_range("Index out of range [0,3]");
    }
}

Vector4 Vector4::clone() const
{
    return Vector4(m_x, m_y, m_z, m_w);
}

void Vector4::copy(const Vector4 &v)
{
    m_x = v.x();
    m_y = v.y();
    m_z = v.z();
    m_w = v.w();
}

Vector4 &Vector4::add(const Vector4 &v)
{
    m_x += v.x();
    m_y += v.y();
    m_z += v.z();
    m_w += v.w();
    return *this;
}

Vector4 &Vector4::addScalar(HIGH_PRECISION s)
{
    m_x += s;
    m_y += s;
    m_z += s;
    m_w += s;
    return *this;
}

Vector4 &Vector4::addVectors(const Vector4 &a, const Vector4 &b)
{
    m_x = a.x() + b.x();
    m_y = a.y() + b.y();
    m_z = a.z() + b.z();
    m_w = a.w() + b.w();
    return *this;
}

Vector4 &Vector4::addScaledVector(const Vector4 &v, HIGH_PRECISION s)
{
    m_x += v.x() * s;
    m_y += v.y() * s;
    m_z += v.z() * s;
    m_w += v.w() * s;
    return *this;
}

Vector4 &Vector4::sub(const Vector4 &v)
{
    m_x -= v.x();
    m_y -= v.y();
    m_z -= v.z();
    m_w -= v.w();
    return *this;
}

Vector4 &Vector4::subScalar(HIGH_PRECISION s)
{
    m_x -= s;
    m_y -= s;
    m_z -= s;
    m_w -= s;
    return *this;
}

Vector4 &Vector4::subVectors(const Vector4 &a, const Vector4 &b)
{
    m_x = a.x() - b.x();
    m_y = a.y() - b.y();
    m_z = a.z() - b.z();
    m_w = a.w() - b.w();
    return *this;
}

Vector4 &Vector4::multiply(const Vector4 &v)
{
    m_x *= v.x();
    m_y *= v.y();
    m_z *= v.z();
    m_w *= v.w();
    return *this;
}

Vector4 &Vector4::multiplyScalar(HIGH_PRECISION scalar)
{
    m_x *= scalar;
    m_y *= scalar;
    m_z *= scalar;
    m_w *= scalar;
    return *this;
}

Vector4 &Vector4::applyMatrix4(const Matrix4 &m)
{
    auto e = m.elements();
    HIGH_PRECISION x = m_x, y = m_y, z = m_z, w = m_w;

    m_x = e[0] * x + e[4] * y + e[8] * z + e[12] * w;
    m_y = e[1] * x + e[5] * y + e[9] * z + e[13] * w;
    m_z = e[2] * x + e[6] * y + e[10] * z + e[14] * w;
    m_w = e[3] * x + e[7] * y + e[11] * z + e[15] * w;

    return *this;
}

Vector4 &Vector4::divide(const Vector4 &v)
{
    m_x /= v.x();
    m_y /= v.y();
    m_z /= v.z();
    m_w /= v.w();
    return *this;
}

Vector4 &Vector4::divideScalar(HIGH_PRECISION scalar)
{
    return multiplyScalar(1.0 / scalar);
}

Vector4 &Vector4::min(const Vector4 &v)
{
    m_x = std::min(m_x, v.x());
    m_y = std::min(m_y, v.y());
    m_z = std::min(m_z, v.z());
    m_w = std::min(m_w, v.w());
    return *this;
}

Vector4 &Vector4::max(const Vector4 &v)
{
    m_x = std::max(m_x, v.x());
    m_y = std::max(m_y, v.y());
    m_z = std::max(m_z, v.z());
    m_w = std::max(m_w, v.w());
    return *this;
}

Vector4 &Vector4::clamp(const Vector4 &min, const Vector4 &max)
{
    m_x = MathUtils::clamp(m_x, min.x(), max.x());
    m_y = MathUtils::clamp(m_y, min.y(), max.y());
    m_z = MathUtils::clamp(m_z, min.z(), max.z());
    m_w = MathUtils::clamp(m_w, min.w(), max.w());
    return *this;
}

Vector4 &Vector4::clampScalar(HIGH_PRECISION minVal, HIGH_PRECISION maxVal)
{
    m_x = MathUtils::clamp(m_x, minVal, maxVal);
    m_y = MathUtils::clamp(m_y, minVal, maxVal);
    m_z = MathUtils::clamp(m_z, minVal, maxVal);
    m_w = MathUtils::clamp(m_w, minVal, maxVal);
    return *this;
}

Vector4 &Vector4::clampLength(HIGH_PRECISION min, HIGH_PRECISION max)
{
    auto len = length();
    divideScalar(len ? len : 1.0);
    multiplyScalar(MathUtils::clamp(len, min, max));
    return *this;
}

Vector4 &Vector4::floor()
{
    m_x = std::floor(m_x);
    m_y = std::floor(m_y);
    m_z = std::floor(m_z);
    m_w = std::floor(m_w);
    return *this;
}

Vector4 &Vector4::ceil()
{
    m_x = std::ceil(m_x);
    m_y = std::ceil(m_y);
    m_z = std::ceil(m_z);
    m_w = std::ceil(m_w);
    return *this;
}

Vector4 &Vector4::round()
{
    m_x = std::round(m_x);
    m_y = std::round(m_y);
    m_z = std::round(m_z);
    m_w = std::round(m_w);
    return *this;
}

Vector4 &Vector4::roundToZero()
{
    m_x = std::trunc(m_x);
    m_y = std::trunc(m_y);
    m_z = std::trunc(m_z);
    m_w = std::trunc(m_w);
    return *this;
}

Vector4 &Vector4::negate()
{
    m_x = -m_x;
    m_y = -m_y;
    m_z = -m_z;
    m_w = -m_w;
    return *this;
}

HIGH_PRECISION Vector4::dot(const Vector4 &v) const
{
    return m_x * v.x() + m_y * v.y() + m_z * v.z() + m_w * v.w();
}

HIGH_PRECISION Vector4::lengthSq() const
{
    return m_x * m_x + m_y * m_y + m_z * m_z + m_w * m_w;
}

HIGH_PRECISION Vector4::length() const
{
    return std::sqrt(lengthSq());
}

HIGH_PRECISION Vector4::manhattanLength() const
{
    return std::abs(m_x) + std::abs(m_y) + std::abs(m_z) + std::abs(m_w);
}

Vector4 &Vector4::normalize()
{
    auto len = length();
    return divideScalar(len ? len : 1.0);
}

Vector4 &Vector4::setLength(HIGH_PRECISION length)
{
    normalize();
    return multiplyScalar(length);
}

Vector4 &Vector4::lerp(const Vector4 &v, HIGH_PRECISION alpha)
{
    m_x += (v.x() - m_x) * alpha;
    m_y += (v.y() - m_y) * alpha;
    m_z += (v.z() - m_z) * alpha;
    m_w += (v.w() - m_w) * alpha;
    return *this;
}

Vector4 &Vector4::lerpVectors(const Vector4 &v1, const Vector4 &v2, HIGH_PRECISION alpha)
{
    m_x = v1.x() + (v2.x() - v1.x()) * alpha;
    m_y = v1.y() + (v2.y() - v1.y()) * alpha;
    m_z = v1.z() + (v2.z() - v1.z()) * alpha;
    m_w = v1.w() + (v2.w() - v1.w()) * alpha;
    return *this;
}

bool Vector4::equals(const Vector4 &v, float epsilon) const
{
    return std::abs(m_x - v.x()) <= epsilon &&
           std::abs(m_y - v.y()) <= epsilon &&
           std::abs(m_z - v.z()) <= epsilon &&
           std::abs(m_w - v.w()) <= epsilon;
}

Vector4 &Vector4::fromArray(const std::vector<HIGH_PRECISION> &array, size_t offset)
{
    m_x = array[offset];
    m_y = array[offset + 1];
    m_z = array[offset + 2];
    m_w = array[offset + 3];
    return *this;
}

std::vector<HIGH_PRECISION> &Vector4::toArray(std::vector<HIGH_PRECISION> &array, size_t offset)
{
    array[offset] = m_x;
    array[offset + 1] = m_y;
    array[offset + 2] = m_z;
    array[offset + 3] = m_w;
    return array;
}

void Vector4::fromMatrixColumn(const Matrix4 &m, size_t index)
{
    fromArray(std::vector<HIGH_PRECISION>(m.elements().begin(), m.elements().end()), index * 4);
}

Vector4 &Vector4::setFromMatrixPosition(const Matrix4 &m)
{
    auto e = m.elements();
    m_x = e[12];
    m_y = e[13];
    m_z = e[14];
    m_w = e[15];
    return *this;
}

Vector4 &Vector4::setFromMatrixScale(const Matrix4 &m)
{
    auto e = m.elements();
    m_x = std::sqrt(e[0] * e[0] + e[1] * e[1] + e[2] * e[2]);
    m_y = std::sqrt(e[4] * e[4] + e[5] * e[5] + e[6] * e[6]);
    m_z = std::sqrt(e[8] * e[8] + e[9] * e[9] + e[10] * e[10]);
    m_w = 1.0;
    return *this;
}

Vector4 &Vector4::random()
{
    m_x = DIST_DOUBLE(RANDOM_ENGINE);
    m_y = DIST_DOUBLE(RANDOM_ENGINE);
    m_z = DIST_DOUBLE(RANDOM_ENGINE);
    m_w = DIST_DOUBLE(RANDOM_ENGINE);
    return *this;
}

// Operators
Vector4 Vector4::operator+(const Vector4 &v) const { return clone().add(v); }
Vector4 Vector4::operator-(const Vector4 &v) const { return clone().sub(v); }
Vector4 Vector4::operator*(const Vector4 &v) const { return clone().multiply(v); }
Vector4 Vector4::operator/(const Vector4 &v) const { return clone().divide(v); }
Vector4 Vector4::operator*(HIGH_PRECISION scalar) const { return clone().multiplyScalar(scalar); }
Vector4 Vector4::operator/(HIGH_PRECISION scalar) const { return clone().divideScalar(scalar); }

void Vector4::operator=(const Vector4 &v) { copy(v); }
bool Vector4::operator==(const Vector4 &v) const { return equals(v); }
HIGH_PRECISION Vector4::operator[](size_t index) const { return getComponent(index); }
