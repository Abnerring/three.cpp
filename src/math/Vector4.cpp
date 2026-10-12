#include "math/Vector4.h"
#include "core/VertexAttribute.h"
#include "math/Matrix4.h"
#include "math/Vector3.h"
#include "math/Quaternion.h"
#include "math/MathUtils.h"
#include <algorithm>
#include <cmath>
#include <stdexcept>

Vector4::Vector4(HIGH_PRECISION x, HIGH_PRECISION y, HIGH_PRECISION z, HIGH_PRECISION w)
    : x(x), y(y), z(z), w(w)
{
}

Vector4::~Vector4()
{
}

Vector4 &Vector4::set(HIGH_PRECISION x, HIGH_PRECISION y, HIGH_PRECISION z, HIGH_PRECISION w)
{
    this->x = x;
    this->y = y;
    this->z = z;
    this->w = w;
    return *this;
}

Vector4 &Vector4::setScalar(HIGH_PRECISION scalar)
{
    this->x = this->y = this->z = this->w = scalar;
    return *this;
}

Vector4 &Vector4::setX(HIGH_PRECISION x) { this->x = x; return *this; }
Vector4 &Vector4::setY(HIGH_PRECISION y) { this->y = y; return *this; }
Vector4 &Vector4::setZ(HIGH_PRECISION z) { this->z = z; return *this; }
Vector4 &Vector4::setW(HIGH_PRECISION w) { this->w = w; return *this; }

Vector4 &Vector4::setComponent(size_t index, HIGH_PRECISION value)
{
    switch (index)
    {
    case 0: this->x = value; break;
    case 1: this->y = value; break;
    case 2: this->z = value; break;
    case 3: this->w = value; break;
    default: throw std::out_of_range("Index out of range [0,3]");
    }
    return *this;
}

HIGH_PRECISION Vector4::getComponent(size_t index) const
{
    switch (index)
    {
    case 0: return this->x;
    case 1: return this->y;
    case 2: return this->z;
    case 3: return this->w;
    default: throw std::out_of_range("Index out of range [0,3]");
    }
}

Vector4 Vector4::clone() const
{
    return Vector4(this->x, this->y, this->z, this->w);
}

Vector4 &Vector4::copy(const Vector4 &v)
{
    this->x = v.x;
    this->y = v.y;
    this->z = v.z;
    this->w = v.w;
    return *this;
}

Vector4 &Vector4::add(const Vector4 &v)
{
    this->x += v.x;
    this->y += v.y;
    this->z += v.z;
    this->w += v.w;
    return *this;
}

Vector4 &Vector4::addScalar(HIGH_PRECISION s)
{
    this->x += s;
    this->y += s;
    this->z += s;
    this->w += s;
    return *this;
}

Vector4 &Vector4::addVectors(const Vector4 &a, const Vector4 &b)
{
    this->x = a.x + b.x;
    this->y = a.y + b.y;
    this->z = a.z + b.z;
    this->w = a.w + b.w;
    return *this;
}

Vector4 &Vector4::addScaledVector(const Vector4 &v, HIGH_PRECISION s)
{
    this->x += v.x * s;
    this->y += v.y * s;
    this->z += v.z * s;
    this->w += v.w * s;
    return *this;
}

Vector4 &Vector4::sub(const Vector4 &v)
{
    this->x -= v.x;
    this->y -= v.y;
    this->z -= v.z;
    this->w -= v.w;
    return *this;
}

Vector4 &Vector4::subScalar(HIGH_PRECISION s)
{
    this->x -= s;
    this->y -= s;
    this->z -= s;
    this->w -= s;
    return *this;
}

Vector4 &Vector4::subVectors(const Vector4 &a, const Vector4 &b)
{
    this->x = a.x - b.x;
    this->y = a.y - b.y;
    this->z = a.z - b.z;
    this->w = a.w - b.w;
    return *this;
}

Vector4 &Vector4::multiply(const Vector4 &v)
{
    this->x *= v.x;
    this->y *= v.y;
    this->z *= v.z;
    this->w *= v.w;
    return *this;
}

Vector4 &Vector4::multiplyScalar(HIGH_PRECISION scalar)
{
    this->x *= scalar;
    this->y *= scalar;
    this->z *= scalar;
    this->w *= scalar;
    return *this;
}

Vector4 &Vector4::applyMatrix4(const Matrix4 &m)
{
    auto e = m.elements();
    HIGH_PRECISION x = this->x, y = this->y, z = this->z, w = this->w;

    this->x = e[0] * x + e[4] * y + e[8] * z + e[12] * w;
    this->y = e[1] * x + e[5] * y + e[9] * z + e[13] * w;
    this->z = e[2] * x + e[6] * y + e[10] * z + e[14] * w;
    this->w = e[3] * x + e[7] * y + e[11] * z + e[15] * w;

    return *this;
}

Vector4 &Vector4::divide(const Vector4 &v)
{
    this->x /= v.x;
    this->y /= v.y;
    this->z /= v.z;
    this->w /= v.w;
    return *this;
}

Vector4 &Vector4::divideScalar(HIGH_PRECISION scalar)
{
    return multiplyScalar(1.0 / scalar);
}

Vector4 &Vector4::min(const Vector4 &v)
{
    this->x = MathUtils::min(this->x, v.x);
    this->y = MathUtils::min(this->y, v.y);
    this->z = MathUtils::min(this->z, v.z);
    this->w = MathUtils::min(this->w, v.w);
    return *this;
}

Vector4 &Vector4::max(const Vector4 &v)
{
    this->x = MathUtils::max(this->x, v.x);
    this->y = MathUtils::max(this->y, v.y);
    this->z = MathUtils::max(this->z, v.z);
    this->w = MathUtils::max(this->w, v.w);
    return *this;
}

Vector4 &Vector4::clamp(const Vector4 &min, const Vector4 &max)
{
    this->x = MathUtils::clamp(this->x, min.x, max.x);
    this->y = MathUtils::clamp(this->y, min.y, max.y);
    this->z = MathUtils::clamp(this->z, min.z, max.z);
    this->w = MathUtils::clamp(this->w, min.w, max.w);
    return *this;
}

Vector4 &Vector4::clampScalar(HIGH_PRECISION minVal, HIGH_PRECISION maxVal)
{
    this->x = MathUtils::clamp(this->x, minVal, maxVal);
    this->y = MathUtils::clamp(this->y, minVal, maxVal);
    this->z = MathUtils::clamp(this->z, minVal, maxVal);
    this->w = MathUtils::clamp(this->w, minVal, maxVal);
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
    this->x = std::floor(this->x);
    this->y = std::floor(this->y);
    this->z = std::floor(this->z);
    this->w = std::floor(this->w);
    return *this;
}

Vector4 &Vector4::ceil()
{
    this->x = std::ceil(this->x);
    this->y = std::ceil(this->y);
    this->z = std::ceil(this->z);
    this->w = std::ceil(this->w);
    return *this;
}

Vector4 &Vector4::round()
{
    this->x = MathUtils::round(this->x);
    this->y = MathUtils::round(this->y);
    this->z = MathUtils::round(this->z);
    this->w = MathUtils::round(this->w);
    return *this;
}

Vector4 &Vector4::roundToZero()
{
    this->x = std::trunc(this->x);
    this->y = std::trunc(this->y);
    this->z = std::trunc(this->z);
    this->w = std::trunc(this->w);
    return *this;
}

Vector4 &Vector4::negate()
{
    this->x = -this->x;
    this->y = -this->y;
    this->z = -this->z;
    this->w = -this->w;
    return *this;
}

HIGH_PRECISION Vector4::dot(const Vector4 &v) const
{
    return this->x * v.x + this->y * v.y + this->z * v.z + this->w * v.w;
}

HIGH_PRECISION Vector4::lengthSq() const
{
    return this->x * this->x + this->y * this->y + this->z * this->z + this->w * this->w;
}

HIGH_PRECISION Vector4::length() const
{
    return std::sqrt(lengthSq());
}

HIGH_PRECISION Vector4::manhattanLength() const
{
    return std::abs(this->x) + std::abs(this->y) + std::abs(this->z) + std::abs(this->w);
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
    this->x += (v.x - this->x) * alpha;
    this->y += (v.y - this->y) * alpha;
    this->z += (v.z - this->z) * alpha;
    this->w += (v.w - this->w) * alpha;
    return *this;
}

Vector4 &Vector4::lerpVectors(const Vector4 &v1, const Vector4 &v2, HIGH_PRECISION alpha)
{
    this->x = v1.x + (v2.x - v1.x) * alpha;
    this->y = v1.y + (v2.y - v1.y) * alpha;
    this->z = v1.z + (v2.z - v1.z) * alpha;
    this->w = v1.w + (v2.w - v1.w) * alpha;
    return *this;
}

bool Vector4::equals(const Vector4 &v, HIGH_PRECISION epsilon) const
{
    if (epsilon == 0) return this->x == v.x && this->y == v.y && this->z == v.z && this->w == v.w;
    return std::abs(this->x - v.x) <= epsilon && std::abs(this->y - v.y) <= epsilon && std::abs(this->z - v.z) <= epsilon && std::abs(this->w - v.w) <= epsilon;
}

Vector4 &Vector4::fromArray(const std::vector<HIGH_PRECISION> &array, size_t offset)
{
    this->x = array[offset];
    this->y = array[offset + 1];
    this->z = array[offset + 2];
    this->w = array[offset + 3];
    return *this;
}

std::vector<HIGH_PRECISION> &Vector4::toArray(std::vector<HIGH_PRECISION> &array, size_t offset) const
{
    if (array.size() < offset + 4) array.resize(offset + 4);
    array[offset] = this->x;
    array[offset + 1] = this->y;
    array[offset + 2] = this->z;
    array[offset + 3] = this->w;
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
    this->x = e[12];
    this->y = e[13];
    this->z = e[14];
    this->w = e[15];
    return *this;
}

Vector4 &Vector4::setFromMatrixScale(const Matrix4 &m)
{
    auto e = m.elements();
    this->x = std::sqrt(e[0] * e[0] + e[1] * e[1] + e[2] * e[2]);
    this->y = std::sqrt(e[4] * e[4] + e[5] * e[5] + e[6] * e[6]);
    this->z = std::sqrt(e[8] * e[8] + e[9] * e[9] + e[10] * e[10]);
    this->w = 1.0;
    return *this;
}

Vector4 &Vector4::random()
{
    this->x = DIST_DOUBLE(RANDOM_ENGINE);
    this->y = DIST_DOUBLE(RANDOM_ENGINE);
    this->z = DIST_DOUBLE(RANDOM_ENGINE);
    this->w = DIST_DOUBLE(RANDOM_ENGINE);
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

		this->w = 2 * std::acos( q.w );

		const auto s = std::sqrt( 1 - q.w * q.w );

		if ( s < 0.0001 ) {

			this->x = 1;
			this->y = 0;
			this->z = 0;

		} else {

			this->x = q.x / s;
			this->y = q.y / s;
			this->z = q.z / s;

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

		this->x = ( m32 - m23 ) / s;
		this->y = ( m13 - m31 ) / s;
		this->z = ( m21 - m12 ) / s;
		this->w = std::acos( ( m11 + m22 + m33 - 1 ) / 2 );

		return *this;


}

Vector4 &Vector4::set(HIGH_PRECISION x, HIGH_PRECISION y, HIGH_PRECISION z) { return set(x, y, z, this->w); }
Vector4 &Vector4::copy(const Vector3 &v) { return set(v.x, v.y, v.z, 1); }
Vector4 &Vector4::setFromMatrixColumn(const Matrix4 &m, size_t index) { return fromMatrixColumn(m, index); }

Vector4 &Vector4::fromBufferAttribute(const VertexAttribute &attribute, double index)
{
    x = attribute.getX(index);
    y = attribute.getY(index);
    z = attribute.getZ(index);
    w = attribute.getW(index);
    return *this;
}

Vector4::Vector4(const Vector4& source) : Vector4(source.x, source.y, source.z, source.w) {}
Vector4::Vector4(Vector4&& source) : Vector4(static_cast<const Vector4&>(source)) {}
