#include "math/Vector3.h"
#include "math/Matrix3.h"
#include "math/MathUtils.h"
#include <stdexcept>
#include <string>

Vector3::Vector3(HIGH_PRECISION x, HIGH_PRECISION y, HIGH_PRECISION z)
{
    this->set(x, y, z);
}

Vector3::~Vector3()
{
    // Destructor implementation (if needed)
}

HIGH_PRECISION Vector3::x() const
{
    return m_x;
}
HIGH_PRECISION Vector3::y() const
{
    return m_y;
}
HIGH_PRECISION Vector3::z() const
{
    return m_z;
}

void Vector3::set(HIGH_PRECISION x, HIGH_PRECISION y, HIGH_PRECISION z)
{
    this->m_x = x;
    this->m_y = y;
    this->m_z = z;
}

void Vector3::setScalar(HIGH_PRECISION scalar)
{
    this->m_x = scalar;
    this->m_y = scalar;
    this->m_z = scalar;
}

void Vector3::setX(HIGH_PRECISION x)
{
    this->m_x = x;
}
void Vector3::setY(HIGH_PRECISION y)
{
    this->m_y = y;
}
void Vector3::setZ(HIGH_PRECISION z)
{
    this->m_z = z;
}

void Vector3::setComponent(size_t index, HIGH_PRECISION value)
{
    switch (index)
    {
    case 0:
        m_x = value;
        break;
    case 1:
        m_y = value;
        break;
    case 2:
        m_z = value;
        break;
    default:
        throw std::out_of_range("Index " + std::to_string(index) + " is out of range [0,2]");
    }
}

HIGH_PRECISION Vector3::getComponent(size_t index) const
{
    switch (index)
    {
    case 0:
        return m_x;
    case 1:
        return m_y;
    case 2:
        return m_z;
    default:
        throw std::out_of_range("Index " + std::to_string(index) + " is out of range [0,2]");
    }
}

Vector3 Vector3::clone()
{
    return Vector3(this->m_x, this->m_y, this->m_z);
}

void Vector3::copy(const Vector3 &v)
{
    this->m_x = v.x();
    this->m_y = v.y();
    this->m_z = v.z();
}

void Vector3::add(const Vector3 &v)
{
    this->m_x += v.x();
    this->m_y += v.y();
    this->m_z += v.z();
}

void Vector3::addScalar(HIGH_PRECISION s)
{
    this->m_x += s;
    this->m_y += s;
    this->m_z += s;
}

void Vector3::addVectors(const Vector3 &a, const Vector3 &b)
{
    this->m_x = a.x() + b.x();
    this->m_y = a.y() + b.y();
    this->m_z = a.z() + b.z();
}

void Vector3::addScaledVector(const Vector3 &v, HIGH_PRECISION s)
{
    this->m_x += v.x() * s;
    this->m_y += v.y() * s;
    this->m_z += v.z() * s;
}

void Vector3::sub(const Vector3 &v)
{
    this->m_x -= v.x();
    this->m_y -= v.y();
    this->m_z -= v.z();
}

void Vector3::subScalar(HIGH_PRECISION s)
{
    this->m_x -= s;
    this->m_y -= s;
    this->m_z -= s;
}

void Vector3::subVectors(const Vector3 &a, const Vector3 &b)
{
    this->m_x = a.x() - b.x();
    this->m_y = a.y() - b.y();
    this->m_z = a.z() - b.z();
}

void Vector3::multiply(const Vector3 &v)
{
    this->m_x *= v.x();
    this->m_y *= v.y();
    this->m_z *= v.z();
}

void Vector3::multiplyScalar(HIGH_PRECISION scalar)
{
    this->m_x *= scalar;
    this->m_y *= scalar;
    this->m_z *= scalar;
}

void Vector3::multiplyVectors(const Vector3 &a, const Vector3 &b)
{
    this->m_x = a.x() * b.x();
    this->m_y = a.y() * b.y();
    this->m_z = a.z() * b.z();
}

void Vector3::applyMatrix3(Matrix3 &m)
{
    auto x = this->m_x, y = this->m_y, z = this->m_z;
    auto e = m.elements();

    this->m_x = e[0] * x + e[3] * y + e[6] * z;
    this->m_y = e[1] * x + e[4] * y + e[7] * z;
    this->m_z = e[2] * x + e[5] * y + e[8] * z;
}

void Vector3::applyNormalMatrix(Matrix3 &m)
{
    this->applyMatrix3(m);
    this->normalize();
}

void Vector3::divide(const Vector3 &v)
{
    this->m_x /= v.x();
    this->m_y /= v.y();
    this->m_z /= v.z();
}

void Vector3::divideScalar(HIGH_PRECISION scalar)
{
    this->multiplyScalar(1 / scalar);
}

void Vector3::min(const Vector3 &v)
{
    this->m_x = std::min(this->m_x, v.x());
    this->m_y = std::min(this->m_y, v.y());
    this->m_z = std::min(this->m_z, v.z());
}

void Vector3::max(const Vector3 &v)
{
    this->m_x = std::max(this->m_x, v.x());
    this->m_y = std::max(this->m_y, v.y());
    this->m_z = std::max(this->m_z, v.z());
}

void Vector3::clamp(const Vector3 &min, const Vector3 &max)
{
    // assumes min < max, componentwise
    this->m_x = MathUtils::clamp(this->m_x, min.x(), max.x());
    this->m_y = MathUtils::clamp(this->m_y, min.y(), max.y());
    this->m_z = MathUtils::clamp(this->m_z, min.z(), max.z());
}

void Vector3::clampScalar(HIGH_PRECISION minVal, HIGH_PRECISION maxVal)
{
    this->m_x = MathUtils::clamp(this->m_x, minVal, maxVal);
    this->m_y = MathUtils::clamp(this->m_y, minVal, maxVal);
    this->m_z = MathUtils::clamp(this->m_z, minVal, maxVal);
}

void Vector3::clampLength(HIGH_PRECISION min, HIGH_PRECISION max)
{
    auto length = this->length();
    divideScalar(length ? length : 1.0);
    multiplyScalar(MathUtils::clamp(length, min, max));
}

void Vector3::floor()
{
    this->m_x = std::floor(this->m_x);
    this->m_y = std::floor(this->m_y);
    this->m_z = std::floor(this->m_z);
}

void Vector3::ceil()
{
    this->m_x = std::ceil(this->m_x);
    this->m_y = std::ceil(this->m_y);
    this->m_z = std::ceil(this->m_z);
}

void Vector3::round()
{
    this->m_x = std::round(this->m_x);
    this->m_y = std::round(this->m_y);
    this->m_z = std::round(this->m_z);
}

void Vector3::roundToZero()
{
    this->m_x = std::trunc(this->m_x);
    this->m_y = std::trunc(this->m_y);
    this->m_z = std::trunc(this->m_z);
}

void Vector3::negate()
{
    this->m_x = -this->m_x;
    this->m_y = -this->m_y;
    this->m_z = -this->m_z;
}

HIGH_PRECISION Vector3::dot(const Vector3 &v) const
{
    return this->m_x * v.x() + this->m_y * v.y() + this->m_z * v.z();
}

HIGH_PRECISION Vector3::lengthSq() const
{
    return this->m_x * this->m_x + this->m_y * this->m_y + this->m_z * this->m_z;
}

HIGH_PRECISION Vector3::length() const
{
    return std::sqrt(this->m_x * this->m_x + this->m_y * this->m_y + this->m_z * this->m_z);
}

HIGH_PRECISION Vector3::manhattanLength() const
{
    return std::abs(this->m_x) + std::abs(this->m_y) + std::abs(this->m_z);
}

void Vector3::normalize()
{
    auto length = this->length();
    this->divideScalar(length ? length : 1);
}

Vector3 Vector3::normalized()
{
    Vector3 vec(this->m_x, this->m_y, this->m_z);
    auto length = vec.length();
    vec.divideScalar(length ? length : 1);
    return vec;
}

void Vector3::setLength(HIGH_PRECISION length)
{
    normalize();
    multiplyScalar(length);
}

void Vector3::lerp(const Vector3 &v, HIGH_PRECISION alpha)
{
    this->m_x += (v.x() - this->m_x) * alpha;
    this->m_y += (v.y() - this->m_y) * alpha;
    this->m_z += (v.z() - this->m_z) * alpha;
}

void Vector3::lerpVectors(const Vector3 &v1, const Vector3 &v2, HIGH_PRECISION alpha)
{
    this->m_x = v1.x() + (v2.x() - v1.x()) * alpha;
    this->m_y = v1.y() + (v2.y() - v1.y()) * alpha;
    this->m_z = v1.z() + (v2.z() - v1.z()) * alpha;
}

void Vector3::cross(const Vector3 &v)
{
    crossVectors(*this, v);
}

void Vector3::crossVectors(const Vector3 &a, const Vector3 &b)
{
    auto ax = a.x(), ay = a.y(), az = a.z();
    auto bx = b.x(), by = b.y(), bz = b.z();

    this->m_x = ay * bz - az * by;
    this->m_y = az * bx - ax * bz;
    this->m_z = ax * by - ay * bx;
}

void Vector3::projectOnVector(const Vector3 &v)
{
    auto denominator = v.lengthSq();

    if (denominator == 0)
        return this->set(0, 0, 0);

    auto scalar = v.dot(*this) / denominator;
    copy(v);
    multiplyScalar(scalar);
}

void Vector3::projectOnPlane(const Vector3 &planeNormal)
{
    auto _vector = Vector3(0, 0, 0);
    _vector.copy(*this);
    _vector.projectOnVector(planeNormal);
    this->sub(_vector);
}

void Vector3::reflect(const Vector3 &normal)
{
    auto _vector = Vector3(0, 0, 0);
    _vector.copy(normal);
    _vector.multiplyScalar(2 * this->dot(normal));
    sub(_vector);
}

float Vector3::angleTo(const Vector3 &v)
{
    auto denominator = std::sqrt(lengthSq() * v.lengthSq());

    if (denominator == 0)
        return MATH_PI / 2;

    auto theta = dot(v) / denominator;

    // clamp, to handle numerical problems

    return std::acos(MathUtils::clamp(theta, -1, 1));
}

HIGH_PRECISION Vector3::distanceTo(const Vector3 &v) const
{
    return std::sqrt(distanceToSquared(v));
}

HIGH_PRECISION Vector3::distanceToSquared(const Vector3 &v) const
{
    auto dx = this->m_x - v.x(), dy = this->m_y - v.y(), dz = this->m_z - v.z();

    return dx * dx + dy * dy + dz * dz;
}

HIGH_PRECISION Vector3::manhattanDistanceTo(const Vector3 &v) const
{
    return std::abs(this->m_x - v.x()) + std::abs(this->m_y - v.y()) + std::abs(this->m_z - v.z());
}

void Vector3::setFromSphericalCoords(float radius, float phi, float theta)
{
    auto sinPhiRadius = std::sin(phi) * radius;

    this->m_x = sinPhiRadius * std::sin(theta);
    this->m_y = std::cos(phi) * radius;
    this->m_z = sinPhiRadius * std::cos(theta);
}

void Vector3::setFromCylindricalCoords(float radius, float theta, HIGH_PRECISION y)
{
    this->m_x = radius * std::sin(theta);
    this->m_y = y;
    this->m_z = radius * std::cos(theta);
}

void Vector3::setFromMatrix3Column(const Matrix3 &m, size_t index)
{
    auto ele_list = MathUtils::convertSpanToVector(m.elements());
    fromArray(ele_list, index * 3);
}

bool Vector3::equals(const Vector3 &v, float epsilon) const
{
    const bool x_equal = std::abs(v.x() - m_x) <= epsilon;
    const bool y_equal = std::abs(v.y() - m_y) <= epsilon;
    const bool z_equal = std::abs(v.z() - m_z) <= epsilon;
    return x_equal && y_equal && z_equal;
}

void Vector3::fromArray(const std::vector<HIGH_PRECISION> &array, size_t offset)
{
    m_x = array[offset];
    m_y = array[offset + 1];
    m_z = array[offset + 2];
}

void Vector3::toArray(std::vector<HIGH_PRECISION> &array, size_t offset)
{
    array[offset] = m_x;
    array[offset + 1] = m_y;
    array[offset + 2] = m_z;
}

void Vector3::random()
{
    m_x = DIST_DOUBLE(RANDOM_ENGINE);
    m_y = DIST_DOUBLE(RANDOM_ENGINE);
    m_z = DIST_DOUBLE(RANDOM_ENGINE);
}

void Vector3::randomDirection()
{
    // https://mathworld.wolfram.com/SpherePointPicking.html

    auto theta = DIST_DOUBLE(RANDOM_ENGINE) * MATH_PI * 2;
    auto u = DIST_DOUBLE(RANDOM_ENGINE) * 2 - 1;
    auto c = std::sqrt(1 - u * u);

    m_x = c * std::cos(theta);
    m_y = u;
    m_z = c * std::sin(theta);
}


Vector3 Vector3::operator+(const Vector3 &v)
{
    Vector3 _vector;
    _vector.copy(*this);
    _vector.add(v);
    return _vector;
}

Vector3 Vector3::operator-(const Vector3 &v)
{
    Vector3 _vector;
    _vector.copy(*this);
    _vector.sub(v);
    return _vector;
}

Vector3 Vector3::operator*(const Vector3 &v)
{
    Vector3 _vector;
    _vector.copy(*this);
    _vector.multiply(v);
    return _vector;
}

Vector3 Vector3::operator/(const Vector3 &v)
{
    Vector3 _vector;
    _vector.copy(*this);
    _vector.divide(v);
    return _vector;
}

void Vector3::operator=(const Vector3 &v)
{
    this->copy(v);
}

bool Vector3::operator==(const Vector3 &v) const
{
    return this->equals(v);
}

HIGH_PRECISION Vector3::operator[](size_t index) const
{
    return this->getComponent(index);
}
