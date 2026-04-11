#include "math/Euler.h"
#include "math/Vector3.h"
#include "math/Quaternion.h"
#include "math/Matrix3.h"
#include "math/Matrix4.h"
#include "math/MathUtils.h"
#include <algorithm>
#include <cmath>
#include <sstream>

Euler::Euler(HIGH_PRECISION x, HIGH_PRECISION y, HIGH_PRECISION z, EulerOrder order)
    : m_x(x), m_y(y), m_z(z), m_order(order)
{
}

Euler::~Euler()
{
}

HIGH_PRECISION Euler::x() const { return m_x; }
HIGH_PRECISION Euler::y() const { return m_y; }
HIGH_PRECISION Euler::z() const { return m_z; }
EulerOrder Euler::order() const { return m_order; }

Euler &Euler::set(HIGH_PRECISION x, HIGH_PRECISION y, HIGH_PRECISION z, EulerOrder order)
{
    m_x = x;
    m_y = y;
    m_z = z;
    m_order = order;
    return *this;
}

Euler &Euler::setX(HIGH_PRECISION x) { m_x = x; return *this; }
Euler &Euler::setY(HIGH_PRECISION y) { m_y = y; return *this; }
Euler &Euler::setZ(HIGH_PRECISION z) { m_z = z; return *this; }
Euler &Euler::setOrder(EulerOrder order) { m_order = order; return *this; }

Euler Euler::clone() const
{
    return Euler(m_x, m_y, m_z, m_order);
}

void Euler::copy(const Euler &euler)
{
    m_x = euler.x();
    m_y = euler.y();
    m_z = euler.z();
    m_order = euler.order();
}

Euler &Euler::setFromEuler(const Euler &euler)
{
    m_x = euler.x();
    m_y = euler.y();
    m_z = euler.z();
    m_order = euler.order();
    return *this;
}

Euler &Euler::setFromQuaternion(const Quaternion &q, EulerOrder order)
{
    auto m = Matrix4();
    q.toRotationMatrix(m);
    return setFromRotationMatrix(m, order);
}

Euler &Euler::setFromRotationMatrix(const Matrix4 &m, EulerOrder order)
{
    auto e = m.elements();

    // Extract rotations based on order
    // This implementation follows three.js logic
    switch (order)
    {
    case EulerOrder::XYZ:
        m_y = std::asin(MathUtils::clamp(e[8], -1.0f, 1.0f));
        if (std::abs(e[8]) < 0.9999999f)
        {
            m_x = std::atan2(-e[9], e[10]);
            m_z = std::atan2(-e[4], e[0]);
        }
        else
        {
            m_x = std::atan2(e[6], e[5]);
            m_z = 0.0f;
        }
        break;

    case EulerOrder::YXZ:
        m_x = std::asin(-MathUtils::clamp(e[9], -1.0f, 1.0f));
        if (std::abs(e[9]) < 0.9999999f)
        {
            m_y = std::atan2(e[8], e[10]);
            m_z = std::atan2(e[1], e[5]);
        }
        else
        {
            m_y = std::atan2(-e[2], e[0]);
            m_z = 0.0f;
        }
        break;

    case EulerOrder::ZXY:
        m_x = std::asin(MathUtils::clamp(e[9], -1.0f, 1.0f));
        if (std::abs(e[9]) < 0.9999999f)
        {
            m_y = std::atan2(-e[8], e[10]);
            m_z = std::atan2(-e[1], e[5]);
        }
        else
        {
            m_y = 0.0f;
            m_z = std::atan2(e[4], e[0]);
        }
        break;

    case EulerOrder::ZYX:
        m_y = std::asin(-MathUtils::clamp(e[8], -1.0f, 1.0f));
        if (std::abs(e[8]) < 0.9999999f)
        {
            m_x = std::atan2(e[9], e[10]);
            m_z = std::atan2(e[4], e[0]);
        }
        else
        {
            m_x = 0.0f;
            m_z = std::atan2(-e[1], e[5]);
        }
        break;

    case EulerOrder::YZX:
        m_z = std::asin(MathUtils::clamp(e[4], -1.0f, 1.0f));
        if (std::abs(e[4]) < 0.9999999f)
        {
            m_x = std::atan2(-e[6], e[10]);
            m_y = std::atan2(-e[1], e[0]);
        }
        else
        {
            m_x = 0.0f;
            m_y = std::atan2(e[8], e[9]);
        }
        break;

    case EulerOrder::XZY:
        m_z = std::asin(-MathUtils::clamp(e[4], -1.0f, 1.0f));
        if (std::abs(e[4]) < 0.9999999f)
        {
            m_x = std::atan2(e[6], e[5]);
            m_y = std::atan2(e[8], e[0]);
        }
        else
        {
            m_x = std::atan2(-e[9], e[10]);
            m_y = 0.0f;
        }
        break;
    }

    m_order = order;
    return *this;
}

bool Euler::equals(const Euler &euler, float epsilon) const
{
    return std::abs(m_x - euler.x()) <= epsilon &&
           std::abs(m_y - euler.y()) <= epsilon &&
           std::abs(m_z - euler.z()) <= epsilon &&
           m_order == euler.order();
}

void Euler::toQuaternion(Quaternion &q) const
{
    q.setFromEuler(*this);
}

void Euler::toRotationMatrix(Matrix4 &m) const
{
    auto x = m_x, y = m_y, z = m_z;

    auto c1 = std::cos(x);
    auto c2 = std::cos(y);
    auto c3 = std::cos(z);

    auto s1 = std::sin(x);
    auto s2 = std::sin(y);
    auto s3 = std::sin(z);

    // Use m.set() method to avoid const span issue
    switch (m_order)
    {
    case EulerOrder::XYZ:
        m.set(
            c2 * c3, -c2 * s3, s2, 0,
            c1 * s3 + c3 * s1 * s2, c1 * c3 - s1 * s2 * s3, -c2 * s1, 0,
            s1 * s3 - c1 * c3 * s2, c3 * s1 + c1 * s2 * s3, c1 * c2, 0,
            0, 0, 0, 1
        );
        break;

    case EulerOrder::YXZ:
        m.set(
            c2 * c3 + s1 * s2 * s3, c1 * s3, -c2 * s3 + c3 * s1 * s2, 0,
            -c3 * s2 + c1 * c2 * s3, c1 * c3, c2 * c3 + s1 * c2 * s3, 0,
            c1 * s2, -s1, c1 * c2, 0,
            0, 0, 0, 1
        );
        break;

    case EulerOrder::ZXY:
        m.set(
            c2 * c3, -c1 * s3 + c3 * s1 * s2, c3 * c1 + c2 * s1 * s3, 0,
            c2 * s3, c1 * c3 + c2 * s1 * s3, -c3 * s1 + c1 * c2 * s3, 0,
            -s2, c1 * s2, c1 * c2, 0,
            0, 0, 0, 1
        );
        break;

    case EulerOrder::ZYX:
        m.set(
            c1 * c2, c1 * s2 * s3 - c3 * s1, c2 * s1 * s3 + c1 * c3, 0,
            c2 * s1, c1 * c3 + c2 * s1 * s3, c3 * s1 * s2 - c1 * s3, 0,
            -s2, c2 * s3, c2 * c3, 0,
            0, 0, 0, 1
        );
        break;

    case EulerOrder::YZX:
        m.set(
            c1 * c2, s1 * s3 + c1 * c3 * s2, -c3 * s1 + c1 * c2 * s3, 0,
            s2, c2 * c3, -c2 * s3, 0,
            -c1 * s2, c2 * s1 - c1 * c3 * s2, c1 * c3 + c2 * s1 * s3, 0,
            0, 0, 0, 1
        );
        break;

    case EulerOrder::XZY:
        m.set(
            c1 * c3, -c2 * s3, c1 * s2 * s3 + c3 * s1, 0,
            s1 * s2, c2 * c3, c1 * s2 * c3 - s1 * s3, 0,
            -c1 * s2, s3 * c2, c1 * c2 * c3 + s1 * s3, 0,
            0, 0, 0, 1
        );
        break;
    }
}

std::vector<HIGH_PRECISION> &Euler::toArray(std::vector<HIGH_PRECISION> &array, size_t offset) const
{
    array[offset] = m_x;
    array[offset + 1] = m_y;
    array[offset + 2] = m_z;
    return array;
}

Euler &Euler::fromArray(const std::vector<HIGH_PRECISION> &array, size_t offset)
{
    m_x = array[offset];
    m_y = array[offset + 1];
    m_z = array[offset + 2];
    return *this;
}

std::string Euler::getOrderString() const
{
    switch (m_order)
    {
    case EulerOrder::XYZ: return "XYZ";
    case EulerOrder::XZY: return "XZY";
    case EulerOrder::YXZ: return "YXZ";
    case EulerOrder::YZX: return "YZX";
    case EulerOrder::ZXY: return "ZXY";
    case EulerOrder::ZYX: return "ZYX";
    default: return "XYZ";
    }
}

void Euler::operator=(const Euler &euler) { copy(euler); }
bool Euler::operator==(const Euler &euler) const { return equals(euler); }
