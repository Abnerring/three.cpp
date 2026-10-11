#include "math/Euler.h"
#include "math/Vector3.h"
#include "math/Quaternion.h"
#include "math/Matrix3.h"
#include "math/Matrix4.h"
#include "math/MathUtils.h"
#include <algorithm>
#include <cmath>
#include <sstream>
#include <utility>
#include <stdexcept>

Euler::Euler(HIGH_PRECISION x, HIGH_PRECISION y, HIGH_PRECISION z, EulerOrder order)
    : m_x(x), m_y(y), m_z(z), m_order(order)
{
}

Euler::Euler(const Euler &euler)
    : Euler(euler.x(), euler.y(), euler.z(), euler.order())
{
}

Euler::~Euler()
{
}

HIGH_PRECISION Euler::x() const { return m_x; }
HIGH_PRECISION Euler::y() const { return m_y; }
HIGH_PRECISION Euler::z() const { return m_z; }
EulerOrder Euler::order() const { return m_order; }

Euler &Euler::set(HIGH_PRECISION x, HIGH_PRECISION y, HIGH_PRECISION z, std::optional<EulerOrder> order)
{
    m_x = x; m_y = y; m_z = z;
    if (order) m_order = *order;
    m_onChangeCallback();
    return *this;
}

Euler &Euler::setX(HIGH_PRECISION x) {
    m_x = x;
    m_onChangeCallback();
    return *this;
}
Euler &Euler::setY(HIGH_PRECISION y) {
    m_y = y;
    m_onChangeCallback();
    return *this;
}
Euler &Euler::setZ(HIGH_PRECISION z) {
    m_z = z;
    m_onChangeCallback();
    return *this;
}
Euler &Euler::setOrder(EulerOrder order) {
    m_order = order;
    m_onChangeCallback();
    return *this;
}

Euler Euler::clone() const
{
    return Euler(m_x, m_y, m_z, m_order);
}

Euler &Euler::copy(const Euler &euler)
{
    m_x = euler.x(); m_y = euler.y(); m_z = euler.z(); m_order = euler.order();
    m_onChangeCallback();
    return *this;
}

Euler &Euler::setFromEuler(const Euler &euler)
{
    return copy(euler);
}

Euler &Euler::setFromQuaternion(const Quaternion &q, std::optional<EulerOrder> order, bool update)
{
    Matrix4 m;
    m.makeRotationFromQuaternion(q);
    return setFromRotationMatrix(m, order, update);
}

Euler &Euler::setFromRotationMatrix(const Matrix4 &m, std::optional<EulerOrder> requestedOrder, bool update)
{
    const auto order = requestedOrder.value_or(m_order);
    const auto e = m.elements();
    const auto m11 = e[0], m12 = e[4], m13 = e[8];
    const auto m21 = e[1], m22 = e[5], m23 = e[9];
    const auto m31 = e[2], m32 = e[6], m33 = e[10];
    switch (order)
    {
    case EulerOrder::XYZ:
        m_y = std::asin(MathUtils::clamp(m13, -1, 1));
        if (std::abs(m13) < 0.9999999) { m_x = std::atan2(-m23, m33); m_z = std::atan2(-m12, m11); }
        else { m_x = std::atan2(m32, m22); m_z = 0; }
        break;
    case EulerOrder::YXZ:
        m_x = std::asin(-MathUtils::clamp(m23, -1, 1));
        if (std::abs(m23) < 0.9999999) { m_y = std::atan2(m13, m33); m_z = std::atan2(m21, m22); }
        else { m_y = std::atan2(-m31, m11); m_z = 0; }
        break;
    case EulerOrder::ZXY:
        m_x = std::asin(MathUtils::clamp(m32, -1, 1));
        if (std::abs(m32) < 0.9999999) { m_y = std::atan2(-m31, m33); m_z = std::atan2(-m12, m22); }
        else { m_y = 0; m_z = std::atan2(m21, m11); }
        break;
    case EulerOrder::ZYX:
        m_y = std::asin(-MathUtils::clamp(m31, -1, 1));
        if (std::abs(m31) < 0.9999999) { m_x = std::atan2(m32, m33); m_z = std::atan2(m21, m11); }
        else { m_x = 0; m_z = std::atan2(-m12, m22); }
        break;
    case EulerOrder::YZX:
        m_z = std::asin(MathUtils::clamp(m21, -1, 1));
        if (std::abs(m21) < 0.9999999) { m_x = std::atan2(-m23, m22); m_y = std::atan2(-m31, m11); }
        else { m_x = 0; m_y = std::atan2(m13, m33); }
        break;
    case EulerOrder::XZY:
        m_z = std::asin(-MathUtils::clamp(m12, -1, 1));
        if (std::abs(m12) < 0.9999999) { m_x = std::atan2(m32, m22); m_y = std::atan2(m13, m11); }
        else { m_x = std::atan2(-m23, m33); m_y = 0; }
        break;
    }
    m_order = order;
    if (update) m_onChangeCallback();
    return *this;
}

bool Euler::equals(const Euler &euler, HIGH_PRECISION epsilon) const
{
    if (epsilon == 0) return m_x == euler.x() && m_y == euler.y() && m_z == euler.z() && m_order == euler.order();
    return std::abs(m_x - euler.x()) <= epsilon && std::abs(m_y - euler.y()) <= epsilon && std::abs(m_z - euler.z()) <= epsilon && m_order == euler.order();
}

void Euler::toQuaternion(Quaternion &q) const
{
    q.setFromEuler(*this);
}

void Euler::toRotationMatrix(Matrix4 &m) const
{
    m.makeRotationFromEuler(*this);
}

std::vector<HIGH_PRECISION> &Euler::toArray(std::vector<HIGH_PRECISION> &array, size_t offset) const
{
    if (array.size() < offset + 3) array.resize(offset + 3);
    array[offset] = m_x;
    array[offset + 1] = m_y;
    array[offset + 2] = m_z;
    return array;
}

Euler &Euler::fromArray(const std::vector<HIGH_PRECISION> &array, size_t offset)
{
    m_x = array.at(offset); m_y = array.at(offset + 1); m_z = array.at(offset + 2);
    m_onChangeCallback();
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

Euler &Euler::operator=(const Euler &euler) {
    return copy(euler);
}
bool Euler::operator==(const Euler &euler) const { return equals(euler); }

Euler &Euler::_onChange(std::function<void()> callback)
{
    m_onChangeCallback = callback ? std::move(callback) : std::function<void()>([] {});
    return *this;
}
Euler &Euler::setFromVector3(const Vector3 &v, std::optional<EulerOrder> order)
{
    return set(v.x(), v.y(), v.z(), order);
}
Euler &Euler::reorder(EulerOrder order)
{
    Quaternion q;
    q.setFromEuler(*this);
    return setFromQuaternion(q, order);
}
std::vector<Euler::ArrayValue> Euler::toArray() const
{
    std::vector<ArrayValue> array;
    toArray(array);
    return array;
}
std::vector<Euler::ArrayValue> &Euler::toArray(std::vector<ArrayValue> &array, size_t offset) const
{
    if (array.size() < offset + 4) array.resize(offset + 4);
    array[offset] = m_x; array[offset + 1] = m_y; array[offset + 2] = m_z; array[offset + 3] = getOrderString();
    return array;
}
Euler &Euler::fromArray(const std::vector<ArrayValue> &array, size_t offset)
{
    auto order = m_order;
    if (array.size() > offset + 3) {
        const auto &name = std::get<std::string>(array[offset + 3]);
        if (name == "XYZ") order = EulerOrder::XYZ;
        else if (name == "YXZ") order = EulerOrder::YXZ;
        else if (name == "ZXY") order = EulerOrder::ZXY;
        else if (name == "ZYX") order = EulerOrder::ZYX;
        else if (name == "YZX") order = EulerOrder::YZX;
        else if (name == "XZY") order = EulerOrder::XZY;
        else throw std::invalid_argument("Unknown Euler order: " + name);
    }
    return set(std::get<HIGH_PRECISION>(array.at(offset)), std::get<HIGH_PRECISION>(array.at(offset + 1)), std::get<HIGH_PRECISION>(array.at(offset + 2)), order);
}
