#include "math/Vector4.h"
#include "core/BufferAttribute.h"
#include "math/Matrix4.h"
#include "math/Vector3.h"
#include "math/Quaternion.h"
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

Vector4 &Vector4::copy(const Vector4 &v)
{
    m_x = v.x();
    m_y = v.y();
    m_z = v.z();
    m_w = v.w();
    return *this;
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
    m_x = MathUtils::min(m_x, v.x());
    m_y = MathUtils::min(m_y, v.y());
    m_z = MathUtils::min(m_z, v.z());
    m_w = MathUtils::min(m_w, v.w());
    return *this;
}

Vector4 &Vector4::max(const Vector4 &v)
{
    m_x = MathUtils::max(m_x, v.x());
    m_y = MathUtils::max(m_y, v.y());
    m_z = MathUtils::max(m_z, v.z());
    m_w = MathUtils::max(m_w, v.w());
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
    divideScalar((len == 0 || std::isnan(len)) ? 1.0 : len);
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
    m_x = MathUtils::round(m_x);
    m_y = MathUtils::round(m_y);
    m_z = MathUtils::round(m_z);
    m_w = MathUtils::round(m_w);
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
    return divideScalar((len == 0 || std::isnan(len)) ? 1.0 : len);
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

bool Vector4::equals(const Vector4 &v, HIGH_PRECISION epsilon) const
{
    if (epsilon == 0) return m_x == v.x() && m_y == v.y() && m_z == v.z() && m_w == v.w();
    return std::abs(m_x - v.x()) <= epsilon && std::abs(m_y - v.y()) <= epsilon && std::abs(m_z - v.z()) <= epsilon && std::abs(m_w - v.w()) <= epsilon;
}

Vector4 &Vector4::fromArray(const std::vector<HIGH_PRECISION> &array, size_t offset)
{
    m_x = array[offset];
    m_y = array[offset + 1];
    m_z = array[offset + 2];
    m_w = array[offset + 3];
    return *this;
}

std::vector<HIGH_PRECISION> &Vector4::toArray(std::vector<HIGH_PRECISION> &array, size_t offset) const
{
    if (array.size() < offset + 4) array.resize(offset + 4);
    array[offset] = m_x;
    array[offset + 1] = m_y;
    array[offset + 2] = m_z;
    array[offset + 3] = m_w;
    return array;
}

Vector4 &Vector4::fromMatrixColumn(const Matrix4 &m, size_t index)
{
    fromArray(std::vector<HIGH_PRECISION>(m.elements().begin(), m.elements().end()), index * 4);
    return *this;
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

Vector4 &Vector4::operator=(const Vector4 &v) { copy(v);
    return *this;
}
bool Vector4::operator==(const Vector4 &v) const { return equals(v); }
HIGH_PRECISION Vector4::operator[](size_t index) const { return getComponent(index); }

std::vector<HIGH_PRECISION> Vector4::toArray() const
{
    std::vector<HIGH_PRECISION> array;
    toArray(array);
    return array;
}

Vector4 &Vector4::setAxisAngleFromQuaternion(const Quaternion &q)
{


		// q is assumed to be normalized

		m_w = 2 * std::acos( q.w() );

		const auto s = std::sqrt( 1 - q.w() * q.w() );

		if ( s < 0.0001 ) {

			m_x = 1;
			m_y = 0;
			m_z = 0;

		} else {

			m_x = q.x() / s;
			m_y = q.y() / s;
			m_z = q.z() / s;

		}

		return *this;


}

Vector4 &Vector4::setAxisAngleFromRotationMatrix(const Matrix4 &m)
{


		// assumes the upper 3x3 of m is a pure rotation matrix (i.e, unscaled)

		HIGH_PRECISION angle, x, y, z; // variables for result
		const auto te = m.elements();
        const auto epsilon = 0.01,		// margin to allow for rounding errors
			epsilon2 = 0.1,		// margin to distinguish between 0 and 180 degrees



			m11 = te[ 0 ], m12 = te[ 4 ], m13 = te[ 8 ],
			m21 = te[ 1 ], m22 = te[ 5 ], m23 = te[ 9 ],
			m31 = te[ 2 ], m32 = te[ 6 ], m33 = te[ 10 ];

		if ( ( std::abs( m12 - m21 ) < epsilon ) &&
		     ( std::abs( m13 - m31 ) < epsilon ) &&
		     ( std::abs( m23 - m32 ) < epsilon ) ) {

			// singularity found
			// first check for identity matrix which must have +1 for all terms
			// in leading diagonal and zero in other terms

			if ( ( std::abs( m12 + m21 ) < epsilon2 ) &&
			     ( std::abs( m13 + m31 ) < epsilon2 ) &&
			     ( std::abs( m23 + m32 ) < epsilon2 ) &&
			     ( std::abs( m11 + m22 + m33 - 3 ) < epsilon2 ) ) {

				// this singularity is identity matrix so angle = 0

				set( 1, 0, 0, 0 );

				return *this; // zero angle, arbitrary axis

			}

			// otherwise this singularity is angle = 180

			angle = MATH_PI;

			const auto xx = ( m11 + 1 ) / 2;
			const auto yy = ( m22 + 1 ) / 2;
			const auto zz = ( m33 + 1 ) / 2;
			const auto xy = ( m12 + m21 ) / 4;
			const auto xz = ( m13 + m31 ) / 4;
			const auto yz = ( m23 + m32 ) / 4;

			if ( ( xx > yy ) && ( xx > zz ) ) {

				// m11 is the largest diagonal term

				if ( xx < epsilon ) {

					x = 0;
					y = 0.707106781;
					z = 0.707106781;

				} else {

					x = std::sqrt( xx );
					y = xy / x;
					z = xz / x;

				}

			} else if ( yy > zz ) {

				// m22 is the largest diagonal term

				if ( yy < epsilon ) {

					x = 0.707106781;
					y = 0;
					z = 0.707106781;

				} else {

					y = std::sqrt( yy );
					x = xy / y;
					z = yz / y;

				}

			} else {

				// m33 is the largest diagonal term so base result on this

				if ( zz < epsilon ) {

					x = 0.707106781;
					y = 0.707106781;
					z = 0;

				} else {

					z = std::sqrt( zz );
					x = xz / z;
					y = yz / z;

				}

			}

			set( x, y, z, angle );

			return *this; // return 180 deg rotation

		}

		// as we have reached here there are no singularities so we can handle normally

		auto s = std::sqrt( ( m32 - m23 ) * ( m32 - m23 ) +
			( m13 - m31 ) * ( m13 - m31 ) +
			( m21 - m12 ) * ( m21 - m12 ) ); // used to normalize

		if ( std::abs( s ) < 0.001 ) s = 1;

		// prevent divide by zero, should not happen if matrix is orthogonal and should be
		// caught by singularity test above, but I've left it in just in case

		m_x = ( m32 - m23 ) / s;
		m_y = ( m13 - m31 ) / s;
		m_z = ( m21 - m12 ) / s;
		m_w = std::acos( ( m11 + m22 + m33 - 1 ) / 2 );

		return *this;


}

Vector4 &Vector4::set(HIGH_PRECISION x, HIGH_PRECISION y, HIGH_PRECISION z) { return set(x, y, z, m_w); }
Vector4 &Vector4::copy(const Vector3 &v) { return set(v.x(), v.y(), v.z(), 1); }
Vector4 &Vector4::setFromMatrixColumn(const Matrix4 &m, size_t index) { return fromMatrixColumn(m, index); }

Vector4 &Vector4::fromBufferAttribute(const BufferAttribute &attribute, size_t index)
{
    return set(attribute.getX(index), attribute.getY(index), attribute.getZ(index), attribute.getW(index));
}
