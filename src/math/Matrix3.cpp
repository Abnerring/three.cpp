#include "math/Matrix3.h"
#include "math/Vector2.h"
#include <stdexcept>

Matrix3::Matrix3(HIGH_PRECISION n11, HIGH_PRECISION n12, HIGH_PRECISION n13, HIGH_PRECISION n21, HIGH_PRECISION n22, HIGH_PRECISION n23, HIGH_PRECISION n31, HIGH_PRECISION n32, HIGH_PRECISION n33)
{
    m_isMatrix3 = true;
    set(n11, n12, n13, n21, n22, n23, n31, n32, n33);
}
Matrix3::~Matrix3()
{
}

std::span<const HIGH_PRECISION, 9> Matrix3::elements() const
{
    return m_elements;
}

void Matrix3::set(HIGH_PRECISION n11, HIGH_PRECISION n12, HIGH_PRECISION n13, HIGH_PRECISION n21, HIGH_PRECISION n22, HIGH_PRECISION n23, HIGH_PRECISION n31, HIGH_PRECISION n32, HIGH_PRECISION n33)
{
    auto &te = this->m_elements;

    te[0] = n11;
    te[1] = n21;
    te[2] = n31;
    te[3] = n12;
    te[4] = n22;
    te[5] = n32;
    te[6] = n13;
    te[7] = n23;
    te[8] = n33;
}

void Matrix3::identity()
{
    set(1, 0, 0,
        0, 1, 0,
        0, 0, 1);
}

void Matrix3::copy(const Matrix3 &m)
{
    auto &te = this->m_elements;
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
}

void Matrix3::multiply(const Matrix3 &m)
{
    multiplyMatrices(*this, m);
}

void Matrix3::premultiply(const Matrix3 &m)
{
    multiplyMatrices(m, *this);
}

void Matrix3::multiplyMatrices(const Matrix3 &a, const Matrix3 &b)
{
    auto ae = a.elements();
    auto be = b.elements();
    auto &te = this->m_elements;

    auto a11 = ae[0], a12 = ae[3], a13 = ae[6];
    auto a21 = ae[1], a22 = ae[4], a23 = ae[7];
    auto a31 = ae[2], a32 = ae[5], a33 = ae[8];

    auto b11 = be[0], b12 = be[3], b13 = be[6];
    auto b21 = be[1], b22 = be[4], b23 = be[7];
    auto b31 = be[2], b32 = be[5], b33 = be[8];

    te[0] = a11 * b11 + a12 * b21 + a13 * b31;
    te[3] = a11 * b12 + a12 * b22 + a13 * b32;
    te[6] = a11 * b13 + a12 * b23 + a13 * b33;

    te[1] = a21 * b11 + a22 * b21 + a23 * b31;
    te[4] = a21 * b12 + a22 * b22 + a23 * b32;
    te[7] = a21 * b13 + a22 * b23 + a23 * b33;

    te[2] = a31 * b11 + a32 * b21 + a33 * b31;
    te[5] = a31 * b12 + a32 * b22 + a33 * b32;
    te[8] = a31 * b13 + a32 * b23 + a33 * b33;
}

void Matrix3::multiplyScalar(HIGH_PRECISION s)
{
    auto &te = this->m_elements;

    te[0] *= s;
    te[3] *= s;
    te[6] *= s;
    te[1] *= s;
    te[4] *= s;
    te[7] *= s;
    te[2] *= s;
    te[5] *= s;
    te[8] *= s;
}

HIGH_PRECISION Matrix3::determinant()
{
    auto &te = this->m_elements;

    auto a = te[0], b = te[1], c = te[2],
         d = te[3], e = te[4], f = te[5],
         g = te[6], h = te[7], i = te[8];

    return a * e * i - a * f * h - b * d * i + b * f * g + c * d * h - c * e * g;
}

void Matrix3::invert()
{
    auto &te = this->m_elements;

    auto n11 = te[0], n21 = te[1], n31 = te[2],
         n12 = te[3], n22 = te[4], n32 = te[5],
         n13 = te[6], n23 = te[7], n33 = te[8],

         t11 = n33 * n22 - n32 * n23,
         t12 = n32 * n13 - n33 * n12,
         t13 = n23 * n12 - n22 * n13,

         det = n11 * t11 + n21 * t12 + n31 * t13;

    if (det == 0)
        set(0, 0, 0, 0, 0, 0, 0, 0, 0);

    auto detInv = 1 / det;

    te[0] = t11 * detInv;
    te[1] = (n31 * n23 - n33 * n21) * detInv;
    te[2] = (n32 * n21 - n31 * n22) * detInv;

    te[3] = t12 * detInv;
    te[4] = (n33 * n11 - n31 * n13) * detInv;
    te[5] = (n31 * n12 - n32 * n11) * detInv;

    te[6] = t13 * detInv;
    te[7] = (n21 * n13 - n23 * n11) * detInv;
    te[8] = (n22 * n11 - n21 * n12) * detInv;
}

void Matrix3::transpose()
{
    HIGH_PRECISION tmp;
    auto &m = this->m_elements;

    tmp = m[1];
    m[1] = m[3];
    m[3] = tmp;
    tmp = m[2];
    m[2] = m[6];
    m[6] = tmp;
    tmp = m[5];
    m[5] = m[7];
    m[7] = tmp;
}

void Matrix3::transposeIntoArray(std::array<HIGH_PRECISION, 9> &r)
{
    auto &m = this->m_elements;
    r[0] = m[0];
    r[1] = m[3];
    r[2] = m[6];
    r[3] = m[1];
    r[4] = m[4];
    r[5] = m[7];
    r[6] = m[2];
    r[7] = m[5];
    r[8] = m[8];
}

void Matrix3::setUvTransform(HIGH_PRECISION tx, HIGH_PRECISION ty, HIGH_PRECISION sx, HIGH_PRECISION sy, float rotation, HIGH_PRECISION cx, HIGH_PRECISION cy)
{
    auto c = std::cos(rotation);
    auto s = std::sin(rotation);

    this->set(sx * c, sx * s,
              -sx * (c * cx + s * cy) + cx + tx,
              -sy * s, sy * c,
              -sy * (-s * cx + c * cy) + cy + ty,
              0, 0, 1);
}

void Matrix3::scale(HIGH_PRECISION sx, HIGH_PRECISION sy)
{
    Matrix3 _m3;
    _m3.makeScale(sx, sy);
    this->premultiply(_m3);
}

void Matrix3::rotate(float theta)
{
    Matrix3 _m3;
    _m3.makeRotation(-theta);
    this->premultiply(_m3);
}

void Matrix3::translate(HIGH_PRECISION tx, HIGH_PRECISION ty)
{
    Matrix3 _m3;
    _m3.makeTranslation(tx, ty);
    this->premultiply(_m3);
}

// for 2D Transforms
void Matrix3::makeTranslation(const Vector2 &v)
{
    this->makeTranslation(v.x(), v.y());
}

void Matrix3::makeTranslation(HIGH_PRECISION x, HIGH_PRECISION y)
{
    this->set(1, 0, x,
              0, 1, y,
              0, 0, 1);
}

void Matrix3::makeRotation(float theta)
{
    // counterclockwise
    auto c = std::cos(theta);
    auto s = std::sin(theta);

    this->set(c, -s, 0,
              s, c, 0,
              0, 0, 1);
}

void Matrix3::makeScale(HIGH_PRECISION x, HIGH_PRECISION y)
{
    this->set(x, 0, 0,
              0, y, 0,
              0, 0, 1);
}

bool Matrix3::equals(const Matrix3 &matrix, float epsilon) const
{
    auto &te = this->m_elements;
    auto me = matrix.elements();

    for (auto i = 0; i < 9; i++)
    {
        const bool equal = std::abs(te[i] - me[i]) > epsilon;
        if (equal)
            return false;
    }

    return true;
}

void Matrix3::fromArray(const std::vector<HIGH_PRECISION> &array, size_t offset)
{
    for (auto i = 0; i < 9; i++)
    {
        this->m_elements[i] = array[i + offset];
    }
}

void Matrix3::toArray(std::vector<HIGH_PRECISION> &array, size_t offset)
{
    auto &te = this->m_elements;

    array[offset] = te[0];
    array[offset + 1] = te[1];
    array[offset + 2] = te[2];

    array[offset + 3] = te[3];
    array[offset + 4] = te[4];
    array[offset + 5] = te[5];

    array[offset + 6] = te[6];
    array[offset + 7] = te[7];
    array[offset + 8] = te[8];
}

Matrix3 Matrix3::clone()
{
    Matrix3 _m3;
    _m3.copy(*this);
    return _m3;
}

Matrix3 Matrix3::operator*(const Matrix3 &m)
{
    Matrix3 _m3 = this->clone();
    _m3.multiply(m);
    return _m3;
}
void Matrix3::operator=(const Matrix3 &m)
{
    this->copy(m);
}
bool Matrix3::operator==(const Matrix3 &m) const
{
    return this->equals(m);
}

HIGH_PRECISION Matrix3::operator[](size_t index) const
{
    if (index >= 9)
        throw std::out_of_range("Index " + std::to_string(index) + " is out of range [0,8]");
    return this->m_elements[index];
}