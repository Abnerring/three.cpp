#ifndef MATRIX3_H
#define MATRIX3_H

#include "common/BasicType.h"
#include <span>

class Vector2;

class Matrix3
{
public:
    Matrix3(
        HIGH_PRECISION n11 = 1.0, HIGH_PRECISION n12 = 0.0, HIGH_PRECISION n13 = 0.0, 
        HIGH_PRECISION n21 = 0.0, HIGH_PRECISION n22 = 1.0, HIGH_PRECISION n23 = 0.0, 
        HIGH_PRECISION n31 = 0.0, HIGH_PRECISION n32 = 0.0, HIGH_PRECISION n33 = 1.0);
    ~Matrix3();
    std::span<const HIGH_PRECISION, 9> elements() const;
    void set(
        HIGH_PRECISION n11, HIGH_PRECISION n12, HIGH_PRECISION n13, 
        HIGH_PRECISION n21, HIGH_PRECISION n22, HIGH_PRECISION n23, 
        HIGH_PRECISION n31, HIGH_PRECISION n32, HIGH_PRECISION n33);
    void identity();
    void copy(const Matrix3 &m);
    // TODO:
    //  extractBasis( xAxis, yAxis, zAxis )
    //  void setFromMatrix4(const Matrix4 &m)
    void multiply(const Matrix3 &m);
    void premultiply(const Matrix3 &m);
    void multiplyMatrices(const Matrix3 &a, const Matrix3 &b);
    void multiplyScalar(HIGH_PRECISION s);
    HIGH_PRECISION determinant();
    void invert();
    void transpose();
    // getNormalMatrix( matrix4 )
    void transposeIntoArray(std::array<HIGH_PRECISION, 9> &r);
    void setUvTransform(HIGH_PRECISION tx, HIGH_PRECISION ty, HIGH_PRECISION sx, HIGH_PRECISION sy, float rotation, HIGH_PRECISION cx, HIGH_PRECISION cy);
    void scale(HIGH_PRECISION sx, HIGH_PRECISION sy);
    void rotate(float theta);
    void translate(HIGH_PRECISION tx, HIGH_PRECISION ty);
    void makeTranslation(const Vector2 &v);
    void makeTranslation(HIGH_PRECISION x, HIGH_PRECISION y);
    void makeRotation(float theta);
    void makeScale(HIGH_PRECISION x, HIGH_PRECISION y);
    bool equals(const Matrix3 &matrix, float epsilon = 1e-6) const;
    void fromArray(const std::vector<HIGH_PRECISION> &array, size_t offset = 0);
    void toArray(std::vector<HIGH_PRECISION> &array, size_t offset = 0);
    Matrix3 clone();

public:
    Matrix3 operator*(const Matrix3 &m);
    void operator=(const Matrix3 &m);
    bool operator==(const Matrix3 &m) const;
    HIGH_PRECISION operator[](size_t index) const;
private:
    bool m_isMatrix3 = false;
    HIGH_PRECISION m_elements[9] = {};
};

#endif
