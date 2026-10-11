#include "math/Quaternion.h"
#include "core/BufferAttribute.h"
#include "math/Vector3.h"
#include "math/Euler.h"
#include "math/Matrix3.h"
#include "math/Matrix4.h"
#include <algorithm>
#include <cmath>
#include <utility>
#include "math/MathUtils.h"

Quaternion::Quaternion(HIGH_PRECISION x, HIGH_PRECISION y, HIGH_PRECISION z, HIGH_PRECISION w)
    : m_x(x), m_y(y), m_z(z), m_w(w)
{
}

Quaternion::Quaternion(const Quaternion &q)
    : Quaternion(q.x(), q.y(), q.z(), q.w())
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
    m_onChangeCallback();
    return *this;
}

Quaternion &Quaternion::setScalar(HIGH_PRECISION s)
{
    m_x = m_y = m_z = m_w = s;
    m_onChangeCallback();
    return *this;
}

Quaternion &Quaternion::setX(HIGH_PRECISION x) { m_x = x; m_onChangeCallback();
    return *this; }
Quaternion &Quaternion::setY(HIGH_PRECISION y) { m_y = y; m_onChangeCallback();
    return *this; }
Quaternion &Quaternion::setZ(HIGH_PRECISION z) { m_z = z; m_onChangeCallback();
    return *this; }
Quaternion &Quaternion::setW(HIGH_PRECISION w) { m_w = w; m_onChangeCallback();
    return *this; }

Quaternion Quaternion::clone() const
{
    return Quaternion(m_x, m_y, m_z, m_w);
}

Quaternion &Quaternion::copy(const Quaternion &q)
{
    m_x = q.x(); m_y = q.y(); m_z = q.z(); m_w = q.w();
    m_onChangeCallback();
    return *this;
}

Quaternion &Quaternion::setFromEuler(const Euler &euler, bool update)
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

    if (update) m_onChangeCallback();
    return *this;
}

Quaternion &Quaternion::setFromAxisAngle(const Vector3 &axis, HIGH_PRECISION angle)
{
    const auto halfAngle = angle / 2;
    const auto s = std::sin(halfAngle);
    m_x = axis.x() * s; m_y = axis.y() * s; m_z = axis.z() * s; m_w = std::cos(halfAngle);
    m_onChangeCallback();
    return *this;
}

Quaternion &Quaternion::setFromRotationMatrix(const Matrix4 &m)
{
    auto e = m.elements();

    HIGH_PRECISION trace = e[0] + e[5] + e[10];
    HIGH_PRECISION s;

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

    m_onChangeCallback();
    return *this;
}

Quaternion &Quaternion::setFromUnitVectors(const Vector3 &v1, const Vector3 &v2)
{
    auto r = v1.dot(v2) + 1;
    if (r < 1e-8) {
        r = 0;
        if (std::abs(v1.x()) > std::abs(v1.z())) { m_x = -v1.y(); m_y = v1.x(); m_z = 0; m_w = r; }
        else { m_x = 0; m_y = -v1.z(); m_z = v1.y(); m_w = r; }
    } else {
        m_x = v1.y() * v2.z() - v1.z() * v2.y();
        m_y = v1.z() * v2.x() - v1.x() * v2.z();
        m_z = v1.x() * v2.y() - v1.y() * v2.x();
        m_w = r;
    }
    return normalize();
}

bool Quaternion::isQuaternionEqual(const Quaternion &q, HIGH_PRECISION epsilon) const
{
    return equals(q, epsilon);
}

Quaternion &Quaternion::normalize()
{
    auto len = length();
    if (len == 0) { m_x = 0; m_y = 0; m_z = 0; m_w = 1; }
    else { len = 1 / len; m_x *= len; m_y *= len; m_z *= len; m_w *= len; }
    m_onChangeCallback();
    return *this;
}

Quaternion &Quaternion::conjugate()
{
    m_x = -m_x;
    m_y = -m_y;
    m_z = -m_z;
    m_onChangeCallback();
    return *this;
}

Quaternion &Quaternion::invert()
{
    return conjugate();
}

Quaternion &Quaternion::add(const Quaternion &q)
{
    m_x += q.x();
    m_y += q.y();
    m_z += q.z();
    m_w += q.w();
    m_onChangeCallback();
    return *this;
}

Quaternion &Quaternion::sub(const Quaternion &q)
{
    m_x -= q.x();
    m_y -= q.y();
    m_z -= q.z();
    m_w -= q.w();
    m_onChangeCallback();
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

    m_onChangeCallback();
    return *this;
}

Quaternion &Quaternion::slerp(const Quaternion &qb, HIGH_PRECISION t)
{
    auto x = qb.x(), y = qb.y(), z = qb.z(), w = qb.w();
    auto dotValue = dot(qb);
    if (dotValue < 0) { x = -x; y = -y; z = -z; w = -w; dotValue = -dotValue; }
    auto s = 1 - t;
    if (dotValue < 0.9995) {
        const auto theta = std::acos(dotValue);
        const auto sinTheta = std::sin(theta);
        s = std::sin(s * theta) / sinTheta;
        t = std::sin(t * theta) / sinTheta;
        m_x = m_x * s + x * t; m_y = m_y * s + y * t; m_z = m_z * s + z * t; m_w = m_w * s + w * t;
        m_onChangeCallback();
    } else {
        m_x = m_x * s + x * t; m_y = m_y * s + y * t; m_z = m_z * s + z * t; m_w = m_w * s + w * t;
        normalize();
    }
    return *this;
}

Quaternion &Quaternion::slerpQuaternions(const Quaternion &qa, const Quaternion &qb, HIGH_PRECISION t)
{
    copy(qa);
    return slerp(qb, t);
}

bool Quaternion::equals(const Quaternion &q, HIGH_PRECISION epsilon) const
{
    if (epsilon == 0) return m_x == q.x() && m_y == q.y() && m_z == q.z() && m_w == q.w();
    return std::abs(m_x - q.x()) <= epsilon && std::abs(m_y - q.y()) <= epsilon && std::abs(m_z - q.z()) <= epsilon && std::abs(m_w - q.w()) <= epsilon;
}

Quaternion &Quaternion::fromArray(const std::vector<HIGH_PRECISION> &array, size_t offset)
{
    m_x = array[offset];
    m_y = array[offset + 1];
    m_z = array[offset + 2];
    m_w = array[offset + 3];
    m_onChangeCallback();
    return *this;
}

std::vector<HIGH_PRECISION> &Quaternion::toArray(std::vector<HIGH_PRECISION> &array, size_t offset) const
{
    if (array.size() < offset + 4) array.resize(offset + 4);
    array[offset] = m_x;
    array[offset + 1] = m_y;
    array[offset + 2] = m_z;
    array[offset + 3] = m_w;
    return array;
}

Vector3 &Quaternion::applyToVector3(Vector3 &v) const
{
    return v.applyQuaternion(*this);
}

void Quaternion::toEuler(Euler &euler) const
{
    euler.setFromQuaternion(*this);
}

void Quaternion::toRotationMatrix(Matrix4 &m) const
{
    m.makeRotationFromQuaternion(*this);
}

// Operators
Quaternion Quaternion::operator+(const Quaternion &q) const { return clone().add(q); }
Quaternion Quaternion::operator-(const Quaternion &q) const { return clone().sub(q); }
Quaternion Quaternion::operator*(const Quaternion &q) const { return clone().multiply(q); }
Quaternion Quaternion::operator*(HIGH_PRECISION s) const { return clone().multiplyScalar(s); }
Quaternion Quaternion::operator/(HIGH_PRECISION s) const { return clone().divideScalar(s); }

Quaternion &Quaternion::operator=(const Quaternion &q) {
    return copy(q);
}
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
    m_onChangeCallback();
    return *this;
}

Quaternion &Quaternion::divideScalar(HIGH_PRECISION s)
{
    return multiplyScalar(1.0f / s);
}

Quaternion &Quaternion::_onChange(std::function<void()> callback)
{
    m_onChangeCallback = callback ? std::move(callback) : std::function<void()>([] {});
    return *this;
}
HIGH_PRECISION Quaternion::angleTo(const Quaternion &q) const
{
    return 2 * std::acos(std::abs(MathUtils::clamp(dot(q), -1, 1)));
}
Quaternion &Quaternion::rotateTowards(const Quaternion &q, HIGH_PRECISION step)
{
    const auto angle = angleTo(q);
    if (angle == 0) return *this;
    return slerp(q, std::min(1.0, step / angle));
}
Quaternion &Quaternion::identity()
{
    return set(0, 0, 0, 1);
}
Quaternion &Quaternion::random()
{
    const auto theta1 = 2 * MATH_PI * DIST_DOUBLE(RANDOM_ENGINE);
    const auto theta2 = 2 * MATH_PI * DIST_DOUBLE(RANDOM_ENGINE);
    const auto x0 = DIST_DOUBLE(RANDOM_ENGINE);
    const auto r1 = std::sqrt(1 - x0), r2 = std::sqrt(x0);
    return set(r1 * std::sin(theta1), r1 * std::cos(theta1), r2 * std::sin(theta2), r2 * std::cos(theta2));
}
std::vector<HIGH_PRECISION> Quaternion::toArray() const
{
    std::vector<HIGH_PRECISION> array;
    toArray(array);
    return array;
}

Quaternion &Quaternion::fromBufferAttribute(const BufferAttribute &attribute, size_t index)
{
    return set(attribute.getX(index), attribute.getY(index), attribute.getZ(index), attribute.getW(index));
}

void Quaternion::slerpFlat(std::vector<HIGH_PRECISION> &dst, size_t dstOffset, const std::vector<HIGH_PRECISION> &src0, size_t srcOffset0, const std::vector<HIGH_PRECISION> &src1, size_t srcOffset1, HIGH_PRECISION t)
{


		auto x0 = src0[ srcOffset0 + 0 ],
			y0 = src0[ srcOffset0 + 1 ],
			z0 = src0[ srcOffset0 + 2 ],
			w0 = src0[ srcOffset0 + 3 ];

		auto x1 = src1[ srcOffset1 + 0 ],
			y1 = src1[ srcOffset1 + 1 ],
			z1 = src1[ srcOffset1 + 2 ],
			w1 = src1[ srcOffset1 + 3 ];

		if ( w0 != w1 || x0 != x1 || y0 != y1 || z0 != z1 ) {

			auto dot = x0 * x1 + y0 * y1 + z0 * z1 + w0 * w1;

			if ( dot < 0 ) {

				x1 = - x1;
				y1 = - y1;
				z1 = - z1;
				w1 = - w1;

				dot = - dot;

			}

			auto s = 1 - t;

			if ( dot < 0.9995 ) {

				// slerp

				const auto theta = std::acos( dot );
				const auto sin = std::sin( theta );

				s = std::sin( s * theta ) / sin;
				t = std::sin( t * theta ) / sin;

				x0 = x0 * s + x1 * t;
				y0 = y0 * s + y1 * t;
				z0 = z0 * s + z1 * t;
				w0 = w0 * s + w1 * t;

			} else {

				// for small angles, lerp then normalize

				x0 = x0 * s + x1 * t;
				y0 = y0 * s + y1 * t;
				z0 = z0 * s + z1 * t;
				w0 = w0 * s + w1 * t;

				const auto f = 1 / std::sqrt( x0 * x0 + y0 * y0 + z0 * z0 + w0 * w0 );

				x0 *= f;
				y0 *= f;
				z0 *= f;
				w0 *= f;

			}

		}

		if (dst.size() < dstOffset + 4) dst.resize(dstOffset + 4);
        dst[ dstOffset ] = x0;
		dst[ dstOffset + 1 ] = y0;
		dst[ dstOffset + 2 ] = z0;
		dst[ dstOffset + 3 ] = w0;


}

std::vector<HIGH_PRECISION> & Quaternion::multiplyQuaternionsFlat(std::vector<HIGH_PRECISION> &dst, size_t dstOffset, const std::vector<HIGH_PRECISION> &src0, size_t srcOffset0, const std::vector<HIGH_PRECISION> &src1, size_t srcOffset1)
{


		const auto x0 = src0[ srcOffset0 ];
		const auto y0 = src0[ srcOffset0 + 1 ];
		const auto z0 = src0[ srcOffset0 + 2 ];
		const auto w0 = src0[ srcOffset0 + 3 ];

		const auto x1 = src1[ srcOffset1 ];
		const auto y1 = src1[ srcOffset1 + 1 ];
		const auto z1 = src1[ srcOffset1 + 2 ];
		const auto w1 = src1[ srcOffset1 + 3 ];

		if (dst.size() < dstOffset + 4) dst.resize(dstOffset + 4);
        dst[ dstOffset ] = x0 * w1 + w0 * x1 + y0 * z1 - z0 * y1;
		dst[ dstOffset + 1 ] = y0 * w1 + w0 * y1 + z0 * x1 - x0 * z1;
		dst[ dstOffset + 2 ] = z0 * w1 + w0 * z1 + x0 * y1 - y0 * x1;
		dst[ dstOffset + 3 ] = w0 * w1 - x0 * x1 - y0 * y1 - z0 * z1;

		return dst;


}

std::vector<HIGH_PRECISION> Quaternion::toJSON() const { return toArray(); }
