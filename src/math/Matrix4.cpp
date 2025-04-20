#include "math/Matrix4.h"
#include "math/Vector3.h"

Matrix4::Matrix4(
    HIGH_PRECISION n11, HIGH_PRECISION n12, HIGH_PRECISION n13, HIGH_PRECISION n14,
    HIGH_PRECISION n21, HIGH_PRECISION n22, HIGH_PRECISION n23, HIGH_PRECISION n24,
    HIGH_PRECISION n31, HIGH_PRECISION n32, HIGH_PRECISION n33, HIGH_PRECISION n34,
    HIGH_PRECISION n41, HIGH_PRECISION n42, HIGH_PRECISION n43, HIGH_PRECISION n44)
{
    m_isMatrix4 = true;
    set(n11, n12, n13, n14, n21, n22, n23, n24, n31, n32, n33, n34, n41, n42, n43, n44);
}
Matrix4::~Matrix4()
{
}

std::span<const HIGH_PRECISION, 16> Matrix4::elements() const
{
    return m_elements;
}

void Matrix4::set(
    HIGH_PRECISION n11, HIGH_PRECISION n12, HIGH_PRECISION n13, HIGH_PRECISION n14,
    HIGH_PRECISION n21, HIGH_PRECISION n22, HIGH_PRECISION n23, HIGH_PRECISION n24,
    HIGH_PRECISION n31, HIGH_PRECISION n32, HIGH_PRECISION n33, HIGH_PRECISION n34,
    HIGH_PRECISION n41, HIGH_PRECISION n42, HIGH_PRECISION n43, HIGH_PRECISION n44)
{
    auto &te = this->m_elements;

    te[0] = n11;
    te[4] = n12;
    te[8] = n13;
    te[12] = n14;

    te[1] = n21;
    te[5] = n22;
    te[9] = n23;
    te[13] = n24;

    te[2] = n31;
    te[6] = n32;
    te[10] = n33;
    te[14] = n34;

    te[3] = n41;
    te[7] = n42;
    te[11] = n43;
    te[15] = n44;
}