#ifndef MATRIX4_H
#define MATRIX4_H

#include "common/BasicType.h"
#include <span>

class Vector3;
class Matrix3;

class Matrix4
{
public:
    Matrix4(
        HIGH_PRECISION n11 = 1.0, HIGH_PRECISION n12 = 0.0, HIGH_PRECISION n13 = 0.0, HIGH_PRECISION n14 = 0.0,
        HIGH_PRECISION n21 = 0.0, HIGH_PRECISION n22 = 1.0, HIGH_PRECISION n23 = 0.0, HIGH_PRECISION n24 = 0.0,
        HIGH_PRECISION n31 = 0.0, HIGH_PRECISION n32 = 0.0, HIGH_PRECISION n33 = 1.0, HIGH_PRECISION n34 = 0.0,
        HIGH_PRECISION n41 = 0.0, HIGH_PRECISION n42 = 0.0, HIGH_PRECISION n43 = 0.0, HIGH_PRECISION n44 = 1.0);
    ~Matrix4();
    std::span<const HIGH_PRECISION, 16> elements() const;
    void set(
        HIGH_PRECISION n11, HIGH_PRECISION n12, HIGH_PRECISION n13, HIGH_PRECISION n14,
        HIGH_PRECISION n21, HIGH_PRECISION n22, HIGH_PRECISION n23, HIGH_PRECISION n24,
        HIGH_PRECISION n31, HIGH_PRECISION n32, HIGH_PRECISION n33, HIGH_PRECISION n34,
        HIGH_PRECISION n41, HIGH_PRECISION n42, HIGH_PRECISION n43, HIGH_PRECISION n44);
    void identity();
    Matrix4 clone();
    void copy(const Matrix4 &m);
    void copyPosition(const Matrix4 &m);
    void setFromMatrix3(const Matrix3 &m);
    void extractBasis(Vector3 &xAxis, Vector3 &yAxis, Vector3 &zAxis);
    void makeBasis(const Vector3 &xAxis, const Vector3 &yAxis, const Vector3 &zAxis);
    void extractRotation(const Matrix4 &m);
    // makeRotationFromEuler( euler )
    // makeRotationFromQuaternion( q )
    void lookAt(Vector3 &eye, Vector3 &target, Vector3 &up);
    void multiply(const Matrix4 &m);
    void premultiply(const Matrix4 &m);
    void multiplyMatrices(const Matrix4 &a, const Matrix4 &b);
    void multiplyScalar(HIGH_PRECISION s);
    HIGH_PRECISION determinant();
    void transpose();
    void setPosition(HIGH_PRECISION x, HIGH_PRECISION y, HIGH_PRECISION z);
    void setPosition(const Vector3 &v);
    void invert();
    void scale(const Vector3 &v);
    HIGH_PRECISION getMaxScaleOnAxis();
    void makeTranslation(HIGH_PRECISION x, HIGH_PRECISION y, HIGH_PRECISION z);
    void makeTranslation(const Vector3 &v);
    void makeRotationX(float theta);
    void makeRotationY(float theta);
    void makeRotationZ(float theta);
    void makeRotationAxis(const Vector3 &axis, float angle);
    void makeScale(HIGH_PRECISION x, HIGH_PRECISION y, HIGH_PRECISION z);
    void makeShear(HIGH_PRECISION xy, HIGH_PRECISION xz, HIGH_PRECISION yx, HIGH_PRECISION yz, HIGH_PRECISION zx, HIGH_PRECISION zy);
    // void Matrix4::compose( position, quaternion, scale ) 

private:
    bool m_isMatrix4 = false;
    HIGH_PRECISION m_elements[16] = {};
};

#endif