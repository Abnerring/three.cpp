#include "math/Quaternion.h"
#include "math/Vector3.h"
#include "math/Euler.h"
#include "math/Matrix3.h"
#include "math/Matrix4.h"
#include <algorithm>
#include <cmath>

Quaternion::Quaternion(HIGH_PRECISION x, HIGH_PRECISION y, HIGH_PRECISION z, HIGH_PRECISION w)
    : m_x(x), m_y(y), m_z(z), m_w(w)
{
}

Quaternion::~Quaternion()
{
}

HIGH_PRECISION Quaternion::x() const { return m_x; }
HIGH_PRECISION Quaternion::y() const { return m_y; }
HIGH_PRECISION Quaternion::z() const { return m_z; }
HIGH_PRECISION Quaternion::w() const { return m_w; }

Quaternion &Quaternion::set(HIGH_PRECISION x, HIGH_PRECISION y, HIGH_PRECISION z, HIGH_PRECISION w)
{
    m_x = x;
    m_y = y;
    m_z = z;
    m_w = w;
    return *this;
}

Quaternion &Quaternion::setScalar(HIGH_PRECISION s)
{
    m_x = m_y = m_z = s;
    m_w = 0;
    return *this;
}

Quaternion &Quaternion::setX(HIGH_PRECISION x) { m_x = x; return *this; }
Quaternion &Quaternion::setY(HIGH_PRECISION y) { m_y = y; return *this; }
Quaternion &Quaternion::setZ(HIGH_PRECISION z) { m_z = z; return *this; }
Quaternion &Quaternion::setW(HIGH_PRECISION w) { m_w = w; return *this; }

Quaternion Quaternion::clone() const
{
    return Quaternion(m_x, m_y, m_z, m_w);
}

void Quaternion::copy(const Quaternion &q)
{
    m_x = q.x();
    m_y = q.y();
    m_z = q.z();
    m_w = q.w();
}

Quaternion &Quaternion::setFromEuler(const Euler &euler)
{
    auto x = euler.x();
    auto y = euler.y();
    auto z = euler.z();
    auto order = euler.order();

    auto c1 = std::cos(x / 2.0f);
    auto c2 = std::cos(y / 2.0f);
    auto c3 = std::cos(z / 2.0f);

    auto s1 = std::sin(x / 2.0f);
    auto s2 = std::sin(y / 2.0f);
    auto s3 = std::sin(z / 2.0f);

    if (order == EulerOrder::XYZ)
    {
        m_x = s1 * c2 * c3 + c1 * s2 * s3;
        m_y = c1 * s2 * c3 - s1 * c2 * s3;
        m_z = c1 * c2 * s3 + s1 * s2 * c3;
        m_w = c1 * c2 * c3 - s1 * s2 * s3;
    }
    else if (order == EulerOrder::YXZ)
    {
        m_x = s1 * c2 * c3 + c1 * s2 * s3;
        m_y = c1 * s2 * c3 - s1 * c2 * s3;
        m_z = c1 * c2 * s3 - s1 * s2 * c3;
        m_w = c1 * c2 * c3 + s1 * s2 * s3;
    }
    else if (order == EulerOrder::ZXY)
    {
        m_x = s1 * c2 * c3 - c1 * s2 * s3;
        m_y = c1 * s2 * c3 + s1 * c2 * s3;
        m_z = c1 * c2 * s3 + s1 * s2 * c3;
        m_w = c1 * c2 * c3 - s1 * s2 * s3;
    }
    else if (order == EulerOrder::ZYX)
    {
        m_x = s1 * c2 * c3 - c1 * s2 * s3;
        m_y = c1 * s2 * c3 + s1 * c2 * s3;
        m_z = c1 * c2 * s3 - s1 * s2 * c3;
        m_w = c1 * c2 * c3 + s1 * s2 * s3;
    }
    else if (order == EulerOrder::YZX)
    {
        m_x = s1 * c2 * c3 + c1 * s2 * s3;
        m_y = c1 * s2 * c3 + s1 * c2 * s3;
        m_z = c1 * c2 * s3 - s1 * s2 * c3;
        m_w = c1 * c2 * c3 - s1 * s2 * s3;
    }
    else if (order == EulerOrder::XZY)
    {
        m_x = s1 * c2 * c3 - c1 * s2 * s3;
        m_y = c1 * s2 * c3 - s1 * c2 * s3;
        m_z = c1 * c2 * s3 + s1 * s2 * c3;
        m_w = c1 * c2 * c3 + s1 * s2 * s3;
    }

    return *this;
}

Quaternion &Quaternion::setFromAxisAngle(const Vector3 &axis, float angle)
{
    auto halfAngle = angle / 2.0f;
    auto s = std::sin(halfAngle);

    m_x = axis.x() * s;
    m_y = axis.y() * s;
    m_z = axis.z() * s;
    m_w = std::cos(halfAngle);

    return normalize();
}

Quaternion &Quaternion::setFromRotationMatrix(const Matrix4 &m)
{
    auto e = m.elements();

    float trace = e[0] + e[5] + e[10];
    float s;

    if (trace > 0.0f)
    {
        s = 0.5f / std::sqrt(trace + 1.0f);
        m_w = 0.25f / s;
        m_x = (e[6] - e[9]) * s;
        m_y = (e[8] - e[2]) * s;
        m_z = (e[1] - e[4]) * s;
    }
    else if (e[0] > e[5] && e[0] > e[10])
    {
        s = 2.0f * std::sqrt(1.0f + e[0] - e[5] - e[10]);
        m_w = (e[6] - e[9]) / s;
        m_x = 0.25f * s;
        m_y = (e[1] + e[4]) / s;
        m_z = (e[8] + e[2]) / s;
    }
    else if (e[5] > e[10])
    {
        s = 2.0f * std::sqrt(1.0f + e[5] - e[0] - e[10]);
        m_w = (e[8] - e[2]) / s;
        m_x = (e[1] + e[4]) / s;
        m_y = 0.25f * s;
        m_z = (e[6] + e[9]) / s;
    }
    else
    {
        s = 2.0f * std::sqrt(1.0f + e[10] - e[0] - e[5]);
        m_w = (e[1] - e[4]) / s;
        m_x = (e[8] + e[2]) / s;
        m_y = (e[6] + e[9]) / s;
        m_z = 0.25f * s;
    }

    return normalize();
}

Quaternion &Quaternion::setFromUnitVectors(const Vector3 &v1, const Vector3 &v2)
{
    auto v1x = v1.x(), v1y = v1.y(), v1z = v1.z();
    auto v2x = v2.x(), v2y = v2.y(), v2z = v2.z();

    auto dot = v1x * v2x + v1y * v2y + v1z * v2z;

    if (dot < -0.999999f)
    {
        // Vectors are opposite, find rotation axis
        Vector3 tmp(1, 0, 0);
        Vector3 axis;
        axis.crossVectors(tmp, v1);
        if (axis.lengthSq() < 0.000001f)
            axis.crossVectors(Vector3(0, 1, 0), v1);

        axis.normalize();
        setFromAxisAngle(axis, MATH_PI);
        return *this;
    }

    m_x = v1y * v2z - v1z * v2y + v2x * std::sqrt(1.0f + dot * 2.0f);
    m_y = v1z * v2x - v1x * v2z + v2y * std::sqrt(1.0f + dot * 2.0f);
    m_z = v1x * v2y - v1y * v2x + v2z * std::sqrt(1.0f + dot * 2.0f);
    m_w = std::sqrt(1.0f + dot * 2.0f);

    return normalize();
}

bool Quaternion::isQuaternionEqual(const Quaternion &q, float epsilon) const
{
    return equals(q, epsilon);
}

Quaternion &Quaternion::normalize()
{
    auto len = length();
    if (len > 0.0f)
    {
        m_x /= len;
        m_y /= len;
        m_z /= len;
        m_w /= len;
    }
    return *this;
}

Quaternion &Quaternion::conjugate()
{
    m_x = -m_x;
    m_y = -m_y;
    m_z = -m_z;
    return *this;
}

Quaternion &Quaternion::invert()
{
    conjugate();
    normalize();
    return *this;
}

Quaternion &Quaternion::add(const Quaternion &q)
{
    m_x += q.x();
    m_y += q.y();
    m_z += q.z();
    m_w += q.w();
    return *this;
}

Quaternion &Quaternion::sub(const Quaternion &q)
{
    m_x -= q.x();
    m_y -= q.y();
    m_z -= q.z();
    m_w -= q.w();
    return *this;
}

HIGH_PRECISION Quaternion::dot(const Quaternion &q) const
{
    return m_x * q.x() + m_y * q.y() + m_z * q.z() + m_w * q.w();
}

HIGH_PRECISION Quaternion::lengthSq() const
{
    return m_x * m_x + m_y * m_y + m_z * m_z + m_w * m_w;
}

HIGH_PRECISION Quaternion::length() const
{
    return std::sqrt(lengthSq());
}

Quaternion &Quaternion::multiply(const Quaternion &q)
{
    multiplyQuaternions(*this, q);
    return *this;
}

Quaternion &Quaternion::premultiply(const Quaternion &q)
{
    multiplyQuaternions(q, *this);
    return *this;
}

Quaternion &Quaternion::multiplyQuaternions(const Quaternion &a, const Quaternion &b)
{
    auto qax = a.x(), qay = a.y(), qaz = a.z(), qaw = a.w();
    auto qbx = b.x(), qby = b.y(), qbz = b.z(), qbw = b.w();

    m_x = qax * qbw + qaw * qbx + qay * qbz - qaz * qby;
    m_y = qay * qbw + qaw * qby + qaz * qbx - qax * qbz;
    m_z = qaz * qbw + qaw * qbz + qax * qby - qay * qbx;
    m_w = qaw * qbw - qax * qbx - qay * qby - qaz * qbz;

    return *this;
}

Quaternion &Quaternion::slerp(const Quaternion &qb, HIGH_PRECISION t)
{
    if (t == 0.0f) return *this;
    if (t == 1.0f)
    {
        copy(qb);
        return *this;
    }

    auto cosHalfTheta = dot(qb);

    if (cosHalfTheta < 0.999999f)
    {
        float halfTheta = std::acos(cosHalfTheta);
        float sinHalfTheta = std::sqrt(1.0f - cosHalfTheta * cosHalfTheta);

        if (std::abs(sinHalfTheta) < 0.001f)
        {
            m_x = m_x * 0.5f + qb.x() * 0.5f;
            m_y = m_y * 0.5f + qb.y() * 0.5f;
            m_z = m_z * 0.5f + qb.z() * 0.5f;
            m_w = m_w * 0.5f + qb.w() * 0.5f;
            normalize();
            return *this;
        }

        float ratioA = std::sin((1.0f - t) * halfTheta) / sinHalfTheta;
        float ratioB = std::sin(t * halfTheta) / sinHalfTheta;

        m_x = m_x * ratioA + qb.x() * ratioB;
        m_y = m_y * ratioA + qb.y() * ratioB;
        m_z = m_z * ratioA + qb.z() * ratioB;
        m_w = m_w * ratioA + qb.w() * ratioB;
    }
    else
    {
        return lerp(qb, t);
    }

    return *this;
}

Quaternion &Quaternion::slerpQuaternions(const Quaternion &qa, const Quaternion &qb, HIGH_PRECISION t)
{
    copy(qa);
    return slerp(qb, t);
}

bool Quaternion::equals(const Quaternion &q, float epsilon) const
{
    return std::abs(m_x - q.x()) <= epsilon &&
           std::abs(m_y - q.y()) <= epsilon &&
           std::abs(m_z - q.z()) <= epsilon &&
           std::abs(m_w - q.w()) <= epsilon;
}

Quaternion &Quaternion::fromArray(const std::vector<HIGH_PRECISION> &array, size_t offset)
{
    m_x = array[offset];
    m_y = array[offset + 1];
    m_z = array[offset + 2];
    m_w = array[offset + 3];
    return *this;
}

std::vector<HIGH_PRECISION> &Quaternion::toArray(std::vector<HIGH_PRECISION> &array, size_t offset) const
{
    array[offset] = m_x;
    array[offset + 1] = m_y;
    array[offset + 2] = m_z;
    array[offset + 3] = m_w;
    return array;
}

Vector3 &Quaternion::applyToVector3(Vector3 &v) const
{
    auto x = v.x(), y = v.y(), z = v.z();

    auto qx = m_x, qy = m_y, qz = m_z, qw = m_w;

    // v' = q * v * q^-1
    auto ix = qw * x + qy * z - qz * y;
    auto iy = qw * y + qz * x - qx * z;
    auto iz = qw * z + qx * y - qy * x;
    auto iw = -qx * x - qy * y - qz * z;

    v.setX(qw * ix + qy * iz - qz * iy - iw * qx);
    v.setY(qw * iy + qz * ix - qx * iz - iw * qy);
    v.setZ(qw * iz + qx * iy - qy * ix - iw * qz);

    return v;
}

void Quaternion::toEuler(Euler &euler) const
{
    auto m = Matrix4();
    toRotationMatrix(m);
    m.extractRotation(m);
    euler.setFromRotationMatrix(m);
}

void Quaternion::toRotationMatrix(Matrix4 &m) const
{
    auto x2 = m_x * m_x, y2 = m_y * m_y, z2 = m_z * m_z;
    auto xy = m_x * m_y, xz = m_x * m_z, yz = m_y * m_z;
    auto wx = m_w * m_x, wy = m_w * m_y, wz = m_w * m_z;

    m.set(
        1.0f - 2.0f * (y2 + z2), 2.0f * (xy - wz), 2.0f * (xz + wy), 0,
        2.0f * (xy + wz), 1.0f - 2.0f * (x2 + z2), 2.0f * (yz - wx), 0,
        2.0f * (xz - wy), 2.0f * (yz + wx), 1.0f - 2.0f * (x2 + y2), 0,
        0, 0, 0, 1
    );
}

// Operators
Quaternion Quaternion::operator+(const Quaternion &q) const { return clone().add(const_cast<Quaternion&>(q)); }
Quaternion Quaternion::operator-(const Quaternion &q) const { return clone().sub(const_cast<Quaternion&>(q)); }
Quaternion Quaternion::operator*(const Quaternion &q) const { return clone().multiply(q); }
Quaternion Quaternion::operator*(HIGH_PRECISION s) const { return clone().multiplyScalar(s); }
Quaternion Quaternion::operator/(HIGH_PRECISION s) const { return clone().divideScalar(s); }

void Quaternion::operator=(const Quaternion &q) { copy(q); }
bool Quaternion::operator==(const Quaternion &q) const { return equals(q); }

Quaternion &Quaternion::lerp(const Quaternion &qb, HIGH_PRECISION t)
{
    if (t <= 0.0f) return *this;
    if (t >= 1.0f) { copy(qb); return *this; }

    m_x += (qb.x() - m_x) * t;
    m_y += (qb.y() - m_y) * t;
    m_z += (qb.z() - m_z) * t;
    m_w += (qb.w() - m_w) * t;

    return normalize();
}

Quaternion &Quaternion::multiplyScalar(HIGH_PRECISION s)
{
    m_x *= s;
    m_y *= s;
    m_z *= s;
    m_w *= s;
    return *this;
}

Quaternion &Quaternion::divideScalar(HIGH_PRECISION s)
{
    return multiplyScalar(1.0f / s);
}
