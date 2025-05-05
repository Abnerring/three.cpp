#include "math/Matrix4.h"
#include "math/Matrix3.h"
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

void Matrix4::identity()
{
    set(
        1, 0, 0, 0,
        0, 1, 0, 0,
        0, 0, 1, 0,
        0, 0, 0, 1);
}

Matrix4 Matrix4::clone()
{
    Matrix4 _m4;
    _m4.copy(*this);
    return _m4;
}

void Matrix4::copy(const Matrix4 &m)
{
    auto &te = m_elements;
    auto me = m.elements();

    te[0] = me[0];
    te[1] = me[1];
    te[2] = me[2];
    te[3] = me[3];
    te[4] = me[4];
    te[5] = me[5];
    te[6] = me[6];
    te[7] = me[7];
    te[8] = me[8];
    te[9] = me[9];
    te[10] = me[10];
    te[11] = me[11];
    te[12] = me[12];
    te[13] = me[13];
    te[14] = me[14];
    te[15] = me[15];
}

void Matrix4::copyPosition(const Matrix4 &m)
{
    auto &te = m_elements;
    auto me = m.elements();

    te[12] = me[12];
    te[13] = me[13];
    te[14] = me[14];
}

void Matrix4::setFromMatrix3(const Matrix3 &m)
{
    auto &me = m_elements;

    set(
        me[0], me[3], me[6], 0,
        me[1], me[4], me[7], 0,
        me[2], me[5], me[8], 0,
        0, 0, 0, 1);
}

void Matrix4::extractBasis(Vector3 &xAxis, Vector3 &yAxis, Vector3 &zAxis)
{
    xAxis.setFromMatrixColumn(*this, 0);
    yAxis.setFromMatrixColumn(*this, 1);
    zAxis.setFromMatrixColumn(*this, 2);
}

void Matrix4::makeBasis(const Vector3 &xAxis, const Vector3 &yAxis, const Vector3 &zAxis)
{
    set(
        xAxis.x(), yAxis.x(), zAxis.x(), 0,
        xAxis.y(), yAxis.y(), zAxis.y(), 0,
        xAxis.z(), yAxis.z(), zAxis.z(), 0,
        0, 0, 0, 1);
}

void Matrix4::extractRotation(const Matrix4 &m)
{
    Vector3 _v1;

    auto &te = m_elements;
    auto me = m.elements();
    _v1.setFromMatrixColumn(m, 0);
    auto scaleX = 1 / _v1.length();
    _v1.setFromMatrixColumn(m, 1);
    auto scaleY = 1 / _v1.length();
    _v1.setFromMatrixColumn(m, 2);
    auto scaleZ = 1 / _v1.length();

    te[0] = me[0] * scaleX;
    te[1] = me[1] * scaleX;
    te[2] = me[2] * scaleX;
    te[3] = 0;

    te[4] = me[4] * scaleY;
    te[5] = me[5] * scaleY;
    te[6] = me[6] * scaleY;
    te[7] = 0;

    te[8] = me[8] * scaleZ;
    te[9] = me[9] * scaleZ;
    te[10] = me[10] * scaleZ;
    te[11] = 0;

    te[12] = 0;
    te[13] = 0;
    te[14] = 0;
    te[15] = 1;
}

void Matrix4::lookAt(Vector3 &eye, Vector3 &target, Vector3 &up)
{
    auto &te = m_elements;
    Vector3 _z;
    Vector3 _x;
    Vector3 _y;
    _z.subVectors(eye, target);

    if (_z.lengthSq() == 0)
    {
        // eye and target are in the same position
        _z.setZ(1);
    }

    _z.normalize();
    _x.crossVectors(up, _z);

    if (_x.lengthSq() == 0)
    {
        // up and z are parallel
        if (std::abs(up.z()) == 1)
        {
            _z.setX(_z.x() + 0.0001);
        }
        else
        {
            _z.setZ(_z.z() + 0.0001);
        }

        _z.normalize();
        _x.crossVectors(up, _z);
    }

    _x.normalize();
    _y.crossVectors(_z, _x);

    te[0] = _x.x();
    te[4] = _y.x();
    te[8] = _z.x();
    te[1] = _x.y();
    te[5] = _y.y();
    te[9] = _z.y();
    te[2] = _x.z();
    te[6] = _y.z();
    te[10] = _z.z();
}

void Matrix4::multiply(const Matrix4 &m)
{
    multiplyMatrices(*this, m);
}

void Matrix4::premultiply(const Matrix4 &m)
{
    multiplyMatrices(m, *this);
}

void Matrix4::multiplyMatrices(const Matrix4 &a, const Matrix4 &b)
{
    auto ae = a.elements();
    auto be = b.elements();
    auto &te = m_elements;

    auto a11 = ae[0], a12 = ae[4], a13 = ae[8], a14 = ae[12];
    auto a21 = ae[1], a22 = ae[5], a23 = ae[9], a24 = ae[13];
    auto a31 = ae[2], a32 = ae[6], a33 = ae[10], a34 = ae[14];
    auto a41 = ae[3], a42 = ae[7], a43 = ae[11], a44 = ae[15];

    auto b11 = be[0], b12 = be[4], b13 = be[8], b14 = be[12];
    auto b21 = be[1], b22 = be[5], b23 = be[9], b24 = be[13];
    auto b31 = be[2], b32 = be[6], b33 = be[10], b34 = be[14];
    auto b41 = be[3], b42 = be[7], b43 = be[11], b44 = be[15];

    te[0] = a11 * b11 + a12 * b21 + a13 * b31 + a14 * b41;
    te[4] = a11 * b12 + a12 * b22 + a13 * b32 + a14 * b42;
    te[8] = a11 * b13 + a12 * b23 + a13 * b33 + a14 * b43;
    te[12] = a11 * b14 + a12 * b24 + a13 * b34 + a14 * b44;

    te[1] = a21 * b11 + a22 * b21 + a23 * b31 + a24 * b41;
    te[5] = a21 * b12 + a22 * b22 + a23 * b32 + a24 * b42;
    te[9] = a21 * b13 + a22 * b23 + a23 * b33 + a24 * b43;
    te[13] = a21 * b14 + a22 * b24 + a23 * b34 + a24 * b44;

    te[2] = a31 * b11 + a32 * b21 + a33 * b31 + a34 * b41;
    te[6] = a31 * b12 + a32 * b22 + a33 * b32 + a34 * b42;
    te[10] = a31 * b13 + a32 * b23 + a33 * b33 + a34 * b43;
    te[14] = a31 * b14 + a32 * b24 + a33 * b34 + a34 * b44;

    te[3] = a41 * b11 + a42 * b21 + a43 * b31 + a44 * b41;
    te[7] = a41 * b12 + a42 * b22 + a43 * b32 + a44 * b42;
    te[11] = a41 * b13 + a42 * b23 + a43 * b33 + a44 * b43;
    te[15] = a41 * b14 + a42 * b24 + a43 * b34 + a44 * b44;
}

void Matrix4::multiplyScalar(HIGH_PRECISION s)
{
    auto &te = m_elements;

    te[0] *= s;
    te[4] *= s;
    te[8] *= s;
    te[12] *= s;
    te[1] *= s;
    te[5] *= s;
    te[9] *= s;
    te[13] *= s;
    te[2] *= s;
    te[6] *= s;
    te[10] *= s;
    te[14] *= s;
    te[3] *= s;
    te[7] *= s;
    te[11] *= s;
    te[15] *= s;
}

HIGH_PRECISION Matrix4::determinant()
{
    auto &te = m_elements;

    auto n11 = te[0], n12 = te[4], n13 = te[8], n14 = te[12];
    auto n21 = te[1], n22 = te[5], n23 = te[9], n24 = te[13];
    auto n31 = te[2], n32 = te[6], n33 = te[10], n34 = te[14];
    auto n41 = te[3], n42 = te[7], n43 = te[11], n44 = te[15];

    // TODO: make this more efficient

    return (
        n41 * (+n14 * n23 * n32 - n13 * n24 * n32 - n14 * n22 * n33 + n12 * n24 * n33 + n13 * n22 * n34 - n12 * n23 * n34) +
        n42 * (+n11 * n23 * n34 - n11 * n24 * n33 + n14 * n21 * n33 - n13 * n21 * n34 + n13 * n24 * n31 - n14 * n23 * n31) +
        n43 * (+n11 * n24 * n32 - n11 * n22 * n34 - n14 * n21 * n32 + n12 * n21 * n34 + n14 * n22 * n31 - n12 * n24 * n31) +
        n44 * (-n13 * n22 * n31 - n11 * n23 * n32 + n11 * n22 * n33 + n13 * n21 * n32 - n12 * n21 * n33 + n12 * n23 * n31));
}

void Matrix4::transpose()
{
    auto &te = m_elements;
    HIGH_PRECISION tmp;

    tmp = te[1];
    te[1] = te[4];
    te[4] = tmp;
    tmp = te[2];
    te[2] = te[8];
    te[8] = tmp;
    tmp = te[6];
    te[6] = te[9];
    te[9] = tmp;

    tmp = te[3];
    te[3] = te[12];
    te[12] = tmp;
    tmp = te[7];
    te[7] = te[13];
    te[13] = tmp;
    tmp = te[11];
    te[11] = te[14];
    te[14] = tmp;
}

void Matrix4::setPosition(HIGH_PRECISION x, HIGH_PRECISION y, HIGH_PRECISION z)
{
    auto &te = m_elements;

    te[12] = x;
    te[13] = y;
    te[14] = z;
}

void Matrix4::setPosition(const Vector3 &v)
{
    auto &te = m_elements;

    te[12] = v.x();
    te[13] = v.y();
    te[14] = v.z();
}

void Matrix4::invert()
{
    // based on http://www.euclideanspace.com/maths/algebra/matrix/functions/inverse/fourD/index.htm
    auto &te = m_elements;

    HIGH_PRECISION n11 = te[0], n21 = te[1], n31 = te[2], n41 = te[3],
                   n12 = te[4], n22 = te[5], n32 = te[6], n42 = te[7],
                   n13 = te[8], n23 = te[9], n33 = te[10], n43 = te[11],
                   n14 = te[12], n24 = te[13], n34 = te[14], n44 = te[15],

                   t11 = n23 * n34 * n42 - n24 * n33 * n42 + n24 * n32 * n43 - n22 * n34 * n43 - n23 * n32 * n44 + n22 * n33 * n44,
                   t12 = n14 * n33 * n42 - n13 * n34 * n42 - n14 * n32 * n43 + n12 * n34 * n43 + n13 * n32 * n44 - n12 * n33 * n44,
                   t13 = n13 * n24 * n42 - n14 * n23 * n42 + n14 * n22 * n43 - n12 * n24 * n43 - n13 * n22 * n44 + n12 * n23 * n44,
                   t14 = n14 * n23 * n32 - n13 * n24 * n32 - n14 * n22 * n33 + n12 * n24 * n33 + n13 * n22 * n34 - n12 * n23 * n34;

    auto det = n11 * t11 + n21 * t12 + n31 * t13 + n41 * t14;

    if (det == 0)
        return set(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);

    auto detInv = 1 / det;

    te[0] = t11 * detInv;
    te[1] = (n24 * n33 * n41 - n23 * n34 * n41 - n24 * n31 * n43 + n21 * n34 * n43 + n23 * n31 * n44 - n21 * n33 * n44) * detInv;
    te[2] = (n22 * n34 * n41 - n24 * n32 * n41 + n24 * n31 * n42 - n21 * n34 * n42 - n22 * n31 * n44 + n21 * n32 * n44) * detInv;
    te[3] = (n23 * n32 * n41 - n22 * n33 * n41 - n23 * n31 * n42 + n21 * n33 * n42 + n22 * n31 * n43 - n21 * n32 * n43) * detInv;

    te[4] = t12 * detInv;
    te[5] = (n13 * n34 * n41 - n14 * n33 * n41 + n14 * n31 * n43 - n11 * n34 * n43 - n13 * n31 * n44 + n11 * n33 * n44) * detInv;
    te[6] = (n14 * n32 * n41 - n12 * n34 * n41 - n14 * n31 * n42 + n11 * n34 * n42 + n12 * n31 * n44 - n11 * n32 * n44) * detInv;
    te[7] = (n12 * n33 * n41 - n13 * n32 * n41 + n13 * n31 * n42 - n11 * n33 * n42 - n12 * n31 * n43 + n11 * n32 * n43) * detInv;

    te[8] = t13 * detInv;
    te[9] = (n14 * n23 * n41 - n13 * n24 * n41 - n14 * n21 * n43 + n11 * n24 * n43 + n13 * n21 * n44 - n11 * n23 * n44) * detInv;
    te[10] = (n12 * n24 * n41 - n14 * n22 * n41 + n14 * n21 * n42 - n11 * n24 * n42 - n12 * n21 * n44 + n11 * n22 * n44) * detInv;
    te[11] = (n13 * n22 * n41 - n12 * n23 * n41 - n13 * n21 * n42 + n11 * n23 * n42 + n12 * n21 * n43 - n11 * n22 * n43) * detInv;

    te[12] = t14 * detInv;
    te[13] = (n13 * n24 * n31 - n14 * n23 * n31 + n14 * n21 * n33 - n11 * n24 * n33 - n13 * n21 * n34 + n11 * n23 * n34) * detInv;
    te[14] = (n14 * n22 * n31 - n12 * n24 * n31 - n14 * n21 * n32 + n11 * n24 * n32 + n12 * n21 * n34 - n11 * n22 * n34) * detInv;
    te[15] = (n12 * n23 * n31 - n13 * n22 * n31 + n13 * n21 * n32 - n11 * n23 * n32 - n12 * n21 * n33 + n11 * n22 * n33) * detInv;
}

void Matrix4::scale(const Vector3 &v)
{
    auto &te = m_elements;
    auto x = v.x(), y = v.y(), z = v.z();

    te[0] *= x;
    te[4] *= y;
    te[8] *= z;
    te[1] *= x;
    te[5] *= y;
    te[9] *= z;
    te[2] *= x;
    te[6] *= y;
    te[10] *= z;
    te[3] *= x;
    te[7] *= y;
    te[11] *= z;
}

HIGH_PRECISION Matrix4::getMaxScaleOnAxis()
{
    auto &te = m_elements;

    HIGH_PRECISION scaleXSq = te[0] * te[0] + te[1] * te[1] + te[2] * te[2];
    HIGH_PRECISION scaleYSq = te[4] * te[4] + te[5] * te[5] + te[6] * te[6];
    HIGH_PRECISION scaleZSq = te[8] * te[8] + te[9] * te[9] + te[10] * te[10];

    return std::sqrt(std::max(std::max(scaleXSq, scaleYSq), scaleZSq));
}

void Matrix4::makeTranslation(HIGH_PRECISION x, HIGH_PRECISION y, HIGH_PRECISION z)
{
    set(

        1, 0, 0, x,
        0, 1, 0, y,
        0, 0, 1, z,
        0, 0, 0, 1

    );
}

void Matrix4::makeTranslation(const Vector3 &v)
{
    set(

        1, 0, 0, v.x(),
        0, 1, 0, v.y(),
        0, 0, 1, v.z(),
        0, 0, 0, 1

    );
}

void Matrix4::makeRotationX(float theta)
{
    auto c = std::cos(theta), s = std::sin(theta);

    set(

        1, 0, 0, 0,
        0, c, -s, 0,
        0, s, c, 0,
        0, 0, 0, 1

    );
}

void Matrix4::makeRotationY(float theta)
{
    auto c = std::cos(theta), s = std::sin(theta);

    set(

        c, 0, s, 0,
        0, 1, 0, 0,
        -s, 0, c, 0,
        0, 0, 0, 1

    );
}

void Matrix4::makeRotationZ(float theta)
{
    auto c = std::cos(theta), s = std::sin(theta);

    set(

        c, -s, 0, 0,
        s, c, 0, 0,
        0, 0, 1, 0,
        0, 0, 0, 1

    );
}

void Matrix4::makeRotationAxis(const Vector3 &axis, float angle)
{
    // Based on http://www.gamedev.net/reference/articles/article1199.asp

    auto c = std::cos(angle);
    auto s = std::sin(angle);
    auto t = 1 - c;
    auto x = axis.x(), y = axis.y(), z = axis.z();
    auto tx = t * x, ty = t * y;

    set(
        tx * x + c, tx * y - s * z, tx * z + s * y, 0,
        tx * y + s * z, ty * y + c, ty * z - s * x, 0,
        tx * z - s * y, ty * z + s * x, t * z * z + c, 0,
        0, 0, 0, 1);
}

void Matrix4::makeScale(HIGH_PRECISION x, HIGH_PRECISION y, HIGH_PRECISION z)
{
    set(

        x, 0, 0, 0,
        0, y, 0, 0,
        0, 0, z, 0,
        0, 0, 0, 1

    );
}

void Matrix4::makeShear(HIGH_PRECISION xy, HIGH_PRECISION xz, HIGH_PRECISION yx, HIGH_PRECISION yz, HIGH_PRECISION zx, HIGH_PRECISION zy)
{
    set(

        1, yx, zx, 0,
        xy, 1, zy, 0,
        xz, yz, 1, 0,
        0, 0, 0, 1

    );
}
