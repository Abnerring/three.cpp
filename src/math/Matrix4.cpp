#include "math/Matrix4.h"
#include "math/Matrix3.h"
#include "math/Vector3.h"
#include "math/Euler.h"
#include "math/Quaternion.h"
#include <cmath>
#include <algorithm>
#include <stdexcept>
#include "constants.h"
#include "math/MathUtils.h"
#include "math/JSNumberMath.h"

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
Matrix4::Matrix4(const Matrix4& source) { m_isMatrix4 = true; copy(source); }
Matrix4& Matrix4::operator=(const Matrix4& source) { return copy(source); }
Matrix4& Matrix4::aliasTo(std::shared_ptr<Matrix4> target,std::weak_ptr<Object3D> objectOwner) {
    if (!target) throw std::invalid_argument("Matrix4 alias target is null");
    for (auto current = target; current; current = current->aliasTarget())
        if (current.get() == this) throw std::invalid_argument("Matrix4 alias cycle");
    alias_ = std::move(target);
    aliasObjectOwner_ = std::move(objectOwner);
    return *this;
}

std::span<const HIGH_PRECISION, 16> Matrix4::elements() const
{
    if (alias_) return static_cast<const Matrix4&>(*alias_).elements();
    return m_elements;
}

Matrix4 &Matrix4::set(
    HIGH_PRECISION n11, HIGH_PRECISION n12, HIGH_PRECISION n13, HIGH_PRECISION n14,
    HIGH_PRECISION n21, HIGH_PRECISION n22, HIGH_PRECISION n23, HIGH_PRECISION n24,
    HIGH_PRECISION n31, HIGH_PRECISION n32, HIGH_PRECISION n33, HIGH_PRECISION n34,
    HIGH_PRECISION n41, HIGH_PRECISION n42, HIGH_PRECISION n43, HIGH_PRECISION n44)
{
    auto te = elements();

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
    return *this;
}

Matrix4 &Matrix4::identity()
{
    set(
        1, 0, 0, 0,
        0, 1, 0, 0,
        0, 0, 1, 0,
        0, 0, 0, 1);
    return *this;
}

Matrix4 Matrix4::clone() const
{
    Matrix4 _m4;
    _m4.copy(*this);
    return _m4;
}

Matrix4 &Matrix4::copy(const Matrix4 &m)
{
    auto te = elements();
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
    return *this;
}

Matrix4 &Matrix4::copyPosition(const Matrix4 &m)
{
    auto te = elements();
    auto me = m.elements();

    te[12] = me[12];
    te[13] = me[13];
    te[14] = me[14];
    return *this;
}

Matrix4 &Matrix4::setFromMatrix3(const Matrix3 &m)
{
    auto me = m.elements();

    set(
        me[0], me[3], me[6], 0,
        me[1], me[4], me[7], 0,
        me[2], me[5], me[8], 0,
        0, 0, 0, 1);
    return *this;
}

Matrix4 &Matrix4::extractBasis(Vector3 &xAxis, Vector3 &yAxis, Vector3 &zAxis)
{
    xAxis.setFromMatrixColumn(*this, 0);
    yAxis.setFromMatrixColumn(*this, 1);
    zAxis.setFromMatrixColumn(*this, 2);
    return *this;
}

Matrix4 &Matrix4::makeBasis(const Vector3 &xAxis, const Vector3 &yAxis, const Vector3 &zAxis)
{
    set(
        xAxis.x, yAxis.x, zAxis.x, 0,
        xAxis.y, yAxis.y, zAxis.y, 0,
        xAxis.z, yAxis.z, zAxis.z, 0,
        0, 0, 0, 1);
    return *this;
}

Matrix4 &Matrix4::extractRotation(const Matrix4 &m)
{
    if (m.determinantAffine() == 0) return identity();
    Vector3 _v1;

    auto te = elements();
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
    return *this;
}

Matrix4 &Matrix4::lookAt(const Vector3 &eye, const Vector3 &target, const Vector3 &up)
{
    auto te = elements();
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
        if (std::abs(up.z) == 1)
        {
            _z.setX(_z.x + 0.0001);
        }
        else
        {
            _z.setZ(_z.z + 0.0001);
        }

        _z.normalize();
        _x.crossVectors(up, _z);
    }

    _x.normalize();
    _y.crossVectors(_z, _x);

    te[0] = _x.x;
    te[4] = _y.x;
    te[8] = _z.x;
    te[1] = _x.y;
    te[5] = _y.y;
    te[9] = _z.y;
    te[2] = _x.z;
    te[6] = _y.z;
    te[10] = _z.z;
    return *this;
}

Matrix4 &Matrix4::multiply(const Matrix4 &m)
{
    multiplyMatrices(*this, m);
    return *this;
}

Matrix4 &Matrix4::premultiply(const Matrix4 &m)
{
    multiplyMatrices(m, *this);
    return *this;
}

Matrix4 &Matrix4::multiplyMatrices(const Matrix4 &a, const Matrix4 &b)
{
    auto ae = a.elements();
    auto be = b.elements();
    auto te = elements();

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
    return *this;
}

Matrix4 &Matrix4::multiplyScalar(HIGH_PRECISION s)
{
    auto te = elements();

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
    return *this;
}

HIGH_PRECISION Matrix4::determinant() const
{
    const auto te = elements();
    const auto n11 = te[0], n12 = te[4], n13 = te[8], n14 = te[12];
    const auto n21 = te[1], n22 = te[5], n23 = te[9], n24 = te[13];
    const auto n31 = te[2], n32 = te[6], n33 = te[10], n34 = te[14];
    const auto n41 = te[3], n42 = te[7], n43 = te[11], n44 = te[15];
    const auto t11 = n23 * n34 - n24 * n33;
    const auto t12 = n22 * n34 - n24 * n32;
    const auto t13 = n22 * n33 - n23 * n32;
    const auto t21 = n21 * n34 - n24 * n31;
    const auto t22 = n21 * n33 - n23 * n31;
    const auto t23 = n21 * n32 - n22 * n31;
    return n11 * (n42 * t11 - n43 * t12 + n44 * t13) -
           n12 * (n41 * t11 - n43 * t21 + n44 * t22) +
           n13 * (n41 * t12 - n42 * t21 + n44 * t23) -
           n14 * (n41 * t13 - n42 * t22 + n43 * t23);
}

Matrix4 &Matrix4::transpose()
{
    auto te = elements();
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
    return *this;
}

Matrix4 &Matrix4::setPosition(HIGH_PRECISION x, HIGH_PRECISION y, HIGH_PRECISION z)
{
    auto te = elements();

    te[12] = x;
    te[13] = y;
    te[14] = z;
    return *this;
}

Matrix4 &Matrix4::setPosition(const Vector3 &v)
{
    auto te = elements();

    te[12] = v.x;
    te[13] = v.y;
    te[14] = v.z;
    return *this;
}

Matrix4 &Matrix4::invert()
{

		// based on https://github.com/toji/gl-matrix
		auto te = elements();
        const double

			n11 = te[ 0 ], n21 = te[ 1 ], n31 = te[ 2 ], n41 = te[ 3 ],
			n12 = te[ 4 ], n22 = te[ 5 ], n32 = te[ 6 ], n42 = te[ 7 ],
			n13 = te[ 8 ], n23 = te[ 9 ], n33 = te[ 10 ], n43 = te[ 11 ],
			n14 = te[ 12 ], n24 = te[ 13 ], n34 = te[ 14 ], n44 = te[ 15 ],

			t1 = n11 * n22 - n21 * n12,
			t2 = n11 * n32 - n31 * n12,
			t3 = n11 * n42 - n41 * n12,
			t4 = n21 * n32 - n31 * n22,
			t5 = n21 * n42 - n41 * n22,
			t6 = n31 * n42 - n41 * n32,
			t7 = n13 * n24 - n23 * n14,
			t8 = n13 * n34 - n33 * n14,
			t9 = n13 * n44 - n43 * n14,
			t10 = n23 * n34 - n33 * n24,
			t11 = n23 * n44 - n43 * n24,
			t12 = n33 * n44 - n43 * n34;

		const auto det = t1 * t12 - t2 * t11 + t3 * t10 + t4 * t9 - t5 * t8 + t6 * t7;

		if ( det == 0 ) return set( 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 );

		const auto detInv = 1 / det;

		te[ 0 ] = ( n22 * t12 - n32 * t11 + n42 * t10 ) * detInv;
		te[ 1 ] = ( n31 * t11 - n21 * t12 - n41 * t10 ) * detInv;
		te[ 2 ] = ( n24 * t6 - n34 * t5 + n44 * t4 ) * detInv;
		te[ 3 ] = ( n33 * t5 - n23 * t6 - n43 * t4 ) * detInv;

		te[ 4 ] = ( n32 * t9 - n12 * t12 - n42 * t8 ) * detInv;
		te[ 5 ] = ( n11 * t12 - n31 * t9 + n41 * t8 ) * detInv;
		te[ 6 ] = ( n34 * t3 - n14 * t6 - n44 * t2 ) * detInv;
		te[ 7 ] = ( n13 * t6 - n33 * t3 + n43 * t2 ) * detInv;

		te[ 8 ] = ( n12 * t11 - n22 * t9 + n42 * t7 ) * detInv;
		te[ 9 ] = ( n21 * t9 - n11 * t11 - n41 * t7 ) * detInv;
		te[ 10 ] = ( n14 * t5 - n24 * t3 + n44 * t1 ) * detInv;
		te[ 11 ] = ( n23 * t3 - n13 * t5 - n43 * t1 ) * detInv;

		te[ 12 ] = ( n22 * t8 - n12 * t10 - n32 * t7 ) * detInv;
		te[ 13 ] = ( n11 * t10 - n21 * t8 + n31 * t7 ) * detInv;
		te[ 14 ] = ( n24 * t2 - n14 * t4 - n34 * t1 ) * detInv;
		te[ 15 ] = ( n13 * t4 - n23 * t2 + n33 * t1 ) * detInv;

		return *this;

}

Matrix4 &Matrix4::scale(const Vector3 &v)
{
    auto te = elements();
    auto x = v.x, y = v.y, z = v.z;

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
    return *this;
}

HIGH_PRECISION Matrix4::getMaxScaleOnAxis() const
{
    auto te = elements();

    HIGH_PRECISION scaleXSq = te[0] * te[0] + te[1] * te[1] + te[2] * te[2];
    HIGH_PRECISION scaleYSq = te[4] * te[4] + te[5] * te[5] + te[6] * te[6];
    HIGH_PRECISION scaleZSq = te[8] * te[8] + te[9] * te[9] + te[10] * te[10];

    return std::sqrt(MathUtils::max(MathUtils::max(scaleXSq, scaleYSq), scaleZSq));
}

Matrix4 &Matrix4::makeTranslation(HIGH_PRECISION x, HIGH_PRECISION y, HIGH_PRECISION z)
{
    set(

        1, 0, 0, x,
        0, 1, 0, y,
        0, 0, 1, z,
        0, 0, 0, 1

    );
    return *this;
}

Matrix4 &Matrix4::makeTranslation(const Vector3 &v)
{
    set(

        1, 0, 0, v.x,
        0, 1, 0, v.y,
        0, 0, 1, v.z,
        0, 0, 0, 1

    );
    return *this;
}

Matrix4 &Matrix4::makeRotationX(HIGH_PRECISION theta)
{
    auto c = JSNumberMath::cos(theta), s = JSNumberMath::sin(theta);

    set(

        1, 0, 0, 0,
        0, c, -s, 0,
        0, s, c, 0,
        0, 0, 0, 1

    );
    return *this;
}

Matrix4 &Matrix4::makeRotationY(HIGH_PRECISION theta)
{
    auto c = JSNumberMath::cos(theta), s = JSNumberMath::sin(theta);

    set(

        c, 0, s, 0,
        0, 1, 0, 0,
        -s, 0, c, 0,
        0, 0, 0, 1

    );
    return *this;
}

Matrix4 &Matrix4::makeRotationZ(HIGH_PRECISION theta)
{
    auto c = JSNumberMath::cos(theta), s = JSNumberMath::sin(theta);

    set(

        c, -s, 0, 0,
        s, c, 0, 0,
        0, 0, 1, 0,
        0, 0, 0, 1

    );
    return *this;
}

Matrix4 &Matrix4::makeRotationAxis(const Vector3 &axis, HIGH_PRECISION angle)
{
    // Based on http://www.gamedev.net/reference/articles/article1199.asp

    auto c = JSNumberMath::cos(angle);
    auto s = JSNumberMath::sin(angle);
    auto t = 1 - c;
    auto x = axis.x, y = axis.y, z = axis.z;
    auto tx = t * x, ty = t * y;

    set(
        tx * x + c, tx * y - s * z, tx * z + s * y, 0,
        tx * y + s * z, ty * y + c, ty * z - s * x, 0,
        tx * z - s * y, ty * z + s * x, t * z * z + c, 0,
        0, 0, 0, 1);
    return *this;
}

Matrix4 &Matrix4::makeScale(HIGH_PRECISION x, HIGH_PRECISION y, HIGH_PRECISION z)
{
    set(

        x, 0, 0, 0,
        0, y, 0, 0,
        0, 0, z, 0,
        0, 0, 0, 1

    );
    return *this;
}

Matrix4 &Matrix4::makeShear(HIGH_PRECISION xy, HIGH_PRECISION xz, HIGH_PRECISION yx, HIGH_PRECISION yz, HIGH_PRECISION zx, HIGH_PRECISION zy)
{
    set(

        1, yx, zx, 0,
        xy, 1, zy, 0,
        xz, yz, 1, 0,
        0, 0, 0, 1

    );
    return *this;
}

Matrix4 &Matrix4::makeRotationFromEuler(const Euler &euler)
{
auto te = elements();

		const auto x = euler.x, y = euler.y, z = euler.z;
		const auto a = JSNumberMath::cos( x ), b = JSNumberMath::sin( x );
		const auto c = JSNumberMath::cos( y ), d = JSNumberMath::sin( y );
		const auto e = JSNumberMath::cos( z ), f = JSNumberMath::sin( z );

		if ( euler.order == EulerOrder::XYZ ) {

			const auto ae = a * e, af = a * f, be = b * e, bf = b * f;

			te[ 0 ] = c * e;
			te[ 4 ] = - c * f;
			te[ 8 ] = d;

			te[ 1 ] = af + be * d;
			te[ 5 ] = ae - bf * d;
			te[ 9 ] = - b * c;

			te[ 2 ] = bf - ae * d;
			te[ 6 ] = be + af * d;
			te[ 10 ] = a * c;

		} else if ( euler.order == EulerOrder::YXZ ) {

			const auto ce = c * e, cf = c * f, de = d * e, df = d * f;

			te[ 0 ] = ce + df * b;
			te[ 4 ] = de * b - cf;
			te[ 8 ] = a * d;

			te[ 1 ] = a * f;
			te[ 5 ] = a * e;
			te[ 9 ] = - b;

			te[ 2 ] = cf * b - de;
			te[ 6 ] = df + ce * b;
			te[ 10 ] = a * c;

		} else if ( euler.order == EulerOrder::ZXY ) {

			const auto ce = c * e, cf = c * f, de = d * e, df = d * f;

			te[ 0 ] = ce - df * b;
			te[ 4 ] = - a * f;
			te[ 8 ] = de + cf * b;

			te[ 1 ] = cf + de * b;
			te[ 5 ] = a * e;
			te[ 9 ] = df - ce * b;

			te[ 2 ] = - a * d;
			te[ 6 ] = b;
			te[ 10 ] = a * c;

		} else if ( euler.order == EulerOrder::ZYX ) {

			const auto ae = a * e, af = a * f, be = b * e, bf = b * f;

			te[ 0 ] = c * e;
			te[ 4 ] = be * d - af;
			te[ 8 ] = ae * d + bf;

			te[ 1 ] = c * f;
			te[ 5 ] = bf * d + ae;
			te[ 9 ] = af * d - be;

			te[ 2 ] = - d;
			te[ 6 ] = b * c;
			te[ 10 ] = a * c;

		} else if ( euler.order == EulerOrder::YZX ) {

			const auto ac = a * c, ad = a * d, bc = b * c, bd = b * d;

			te[ 0 ] = c * e;
			te[ 4 ] = bd - ac * f;
			te[ 8 ] = bc * f + ad;

			te[ 1 ] = f;
			te[ 5 ] = a * e;
			te[ 9 ] = - b * e;

			te[ 2 ] = - d * e;
			te[ 6 ] = ad * f + bc;
			te[ 10 ] = ac - bd * f;

		} else if ( euler.order == EulerOrder::XZY ) {

			const auto ac = a * c, ad = a * d, bc = b * c, bd = b * d;

			te[ 0 ] = c * e;
			te[ 4 ] = - f;
			te[ 8 ] = d * e;

			te[ 1 ] = ac * f + bd;
			te[ 5 ] = a * e;
			te[ 9 ] = ad * f - bc;

			te[ 2 ] = bc * f - ad;
			te[ 6 ] = b * e;
			te[ 10 ] = bd * f + ac;

		}

		// bottom row
		te[ 3 ] = 0;
		te[ 7 ] = 0;
		te[ 11 ] = 0;

		// last column
		te[ 12 ] = 0;
		te[ 13 ] = 0;
		te[ 14 ] = 0;
		te[ 15 ] = 1;

		return *this;
}

Matrix4 &Matrix4::makeRotationFromQuaternion(const Quaternion &q)
{
    auto x = q.x, y = q.y, z = q.z, w = q.w;

    auto x2 = x + x, y2 = y + y, z2 = z + z;
    auto xx = x * x2, xy = x * y2, xz = x * z2;
    auto yy = y * y2, yz = y * z2, zz = z * z2;
    auto wx = w * x2, wy = w * y2, wz = w * z2;

    auto te = elements();

    te[0] = 1.0f - (yy + zz);
    te[4] = xy - wz;
    te[8] = xz + wy;

    te[1] = xy + wz;
    te[5] = 1.0f - (xx + zz);
    te[9] = yz - wx;

    te[2] = xz - wy;
    te[6] = yz + wx;
    te[10] = 1.0f - (xx + yy);

    te[3] = te[7] = te[11] = te[12] = te[13] = te[14] = 0;
    te[15] = 1;
    return *this;
}

Matrix4 &Matrix4::compose(const Vector3 &position, const Quaternion &quaternion, const Vector3 &scale)
{
    auto x = quaternion.x;
    auto y = quaternion.y;
    auto z = quaternion.z;
    auto w = quaternion.w;

    auto x2 = x + x;
    auto y2 = y + y;
    auto z2 = z + z;
    auto xx = x * x2;
    auto xy = x * y2;
    auto xz = x * z2;
    auto yy = y * y2;
    auto yz = y * z2;
    auto zz = z * z2;
    auto wx = w * x2;
    auto wy = w * y2;
    auto wz = w * z2;
    auto sx = scale.x;
    auto sy = scale.y;
    auto sz = scale.z;

    auto te = elements();

    te[0] = (1.0f - (yy + zz)) * sx;
    te[1] = (xy + wz) * sx;
    te[2] = (xz - wy) * sx;
    te[3] = 0;

    te[4] = (xy - wz) * sy;
    te[5] = (1.0f - (xx + zz)) * sy;
    te[6] = (yz + wx) * sy;
    te[7] = 0;

    te[8] = (xz + wy) * sz;
    te[9] = (yz - wx) * sz;
    te[10] = (1.0f - (xx + yy)) * sz;
    te[11] = 0;

    te[12] = position.x;
    te[13] = position.y;
    te[14] = position.z;
    te[15] = 1;
    return *this;
}


std::span<HIGH_PRECISION, 16> Matrix4::elements()
{
    if (alias_) return alias_->elements();
    return m_elements;
}
HIGH_PRECISION Matrix4::determinantAffine() const
{
    const auto te = elements();
    const auto n11 = te[0], n12 = te[4], n13 = te[8];
    const auto n21 = te[1], n22 = te[5], n23 = te[9];
    const auto n31 = te[2], n32 = te[6], n33 = te[10];
    return n11 * (n22 * n33 - n23 * n32) - n12 * (n21 * n33 - n23 * n31) + n13 * (n21 * n32 - n22 * n31);
}
Matrix4 &Matrix4::decompose(Vector3 &position, Quaternion &quaternion, Vector3 &scale)
{
    const auto te = elements();
    position.set(te[12], te[13], te[14]);
    const auto det = determinantAffine();
    if (det == 0) {
        scale.set(1, 1, 1);
        quaternion.identity();
        return *this;
    }
    auto sx = Vector3(te[0], te[1], te[2]).length();
    const auto sy = Vector3(te[4], te[5], te[6]).length();
    const auto sz = Vector3(te[8], te[9], te[10]).length();
    if (det < 0) sx = -sx;
    auto rotation = clone();
    const auto invSX = 1 / sx, invSY = 1 / sy, invSZ = 1 / sz;
    rotation.elements()[0] *= invSX; rotation.elements()[1] *= invSX; rotation.elements()[2] *= invSX;
    rotation.elements()[4] *= invSY; rotation.elements()[5] *= invSY; rotation.elements()[6] *= invSY;
    rotation.elements()[8] *= invSZ; rotation.elements()[9] *= invSZ; rotation.elements()[10] *= invSZ;
    quaternion.setFromRotationMatrix(rotation);
    scale.set(sx, sy, sz);
    return *this;
}
Matrix4 &Matrix4::makePerspective(HIGH_PRECISION left, HIGH_PRECISION right, HIGH_PRECISION top, HIGH_PRECISION bottom, HIGH_PRECISION near, HIGH_PRECISION far, int coordinateSystem, bool reversedDepth)
{
    const auto x = 2 * near / (right - left), y = 2 * near / (top - bottom);
    const auto a = (right + left) / (right - left), b = (top + bottom) / (top - bottom);
    HIGH_PRECISION c, d;
    if (reversedDepth) { c = near / (far - near); d = far * near / (far - near); }
    else if (coordinateSystem == WebGLCoordinateSystem) { c = -(far + near) / (far - near); d = -2 * far * near / (far - near); }
    else if (coordinateSystem == WebGPUCoordinateSystem) { c = -far / (far - near); d = -far * near / (far - near); }
    else throw std::invalid_argument("Matrix4.makePerspective: invalid coordinate system");
    return set(x, 0, a, 0, 0, y, b, 0, 0, 0, c, d, 0, 0, -1, 0);
}
Matrix4 &Matrix4::makeOrthographic(HIGH_PRECISION left, HIGH_PRECISION right, HIGH_PRECISION top, HIGH_PRECISION bottom, HIGH_PRECISION near, HIGH_PRECISION far, int coordinateSystem, bool reversedDepth)
{
    const auto x = 2 / (right - left), y = 2 / (top - bottom);
    const auto a = -(right + left) / (right - left), b = -(top + bottom) / (top - bottom);
    HIGH_PRECISION c, d;
    if (reversedDepth) { c = 1 / (far - near); d = far / (far - near); }
    else if (coordinateSystem == WebGLCoordinateSystem) { c = -2 / (far - near); d = -(far + near) / (far - near); }
    else if (coordinateSystem == WebGPUCoordinateSystem) { c = -1 / (far - near); d = -near / (far - near); }
    else throw std::invalid_argument("Matrix4.makeOrthographic: invalid coordinate system");
    return set(x, 0, 0, a, 0, y, 0, b, 0, 0, c, d, 0, 0, 0, 1);
}
bool Matrix4::equals(const Matrix4 &matrix) const
{
    for (size_t i = 0; i < 16; ++i) if (elements()[i] != matrix.elements()[i]) return false;
    return true;
}
Matrix4 &Matrix4::fromArray(const std::vector<HIGH_PRECISION> &array, size_t offset)
{
    for (size_t i = 0; i < 16; ++i) elements()[i] = array.at(offset + i);
    return *this;
}
std::vector<HIGH_PRECISION> &Matrix4::toArray(std::vector<HIGH_PRECISION> &array, size_t offset) const
{
    if (array.size() < offset + 16) array.resize(offset + 16);
    std::copy(elements().begin(), elements().end(), array.begin() + offset);
    return array;
}
std::vector<HIGH_PRECISION> Matrix4::toArray() const
{
    return std::vector<HIGH_PRECISION>(elements().begin(), elements().end());
}
HIGH_PRECISION Matrix4::operator[](size_t index) const
{
    if (index >= 16) throw std::out_of_range("Matrix4 element index");
    return elements()[index];
}
