#ifndef VECTOR4_H
#define VECTOR4_H

#include "common/BasicType.h"
#include <vector>

class Vector3;

/**
 * Class representing a 4D vector. A 4D vector is an ordered quadruplet of numbers
 * (labeled x, y, z, and w), which can represent a position in 4D space (with w as the homogeneous coordinate)
 * or many other things such as colors with alpha channel.
 *
 * @example
 * Vector4 v1(1.0, 2.0, 3.0, 4.0);
 * Vector4 v2(5.0, 6.0, 7.0, 8.0);
 * v1.add(v2); // v1 = (6, 8, 10, 12)
 */
class Vector4
{
public:
    /**
     * Constructs a new 4D vector.
     *
     * @param x The x value of this vector. Default is 0.
     * @param y The y value of this vector. Default is 0.
     * @param z The z value of this vector. Default is 0.
     * @param w The w value of this vector. Default is 1.
     */
    Vector4(HIGH_PRECISION x = 0.0, HIGH_PRECISION y = 0.0, HIGH_PRECISION z = 0.0, HIGH_PRECISION w = 1.0);
    ~Vector4();

    HIGH_PRECISION x() const;
    HIGH_PRECISION y() const;
    HIGH_PRECISION z() const;
    HIGH_PRECISION w() const;

    Vector4 &set(HIGH_PRECISION x, HIGH_PRECISION y, HIGH_PRECISION z, HIGH_PRECISION w);
    Vector4 &setScalar(HIGH_PRECISION scalar);
    Vector4 &setX(HIGH_PRECISION x);
    Vector4 &setY(HIGH_PRECISION y);
    Vector4 &setZ(HIGH_PRECISION z);
    Vector4 &setW(HIGH_PRECISION w);

    Vector4 &setComponent(size_t index, HIGH_PRECISION value);
    HIGH_PRECISION getComponent(size_t index) const;

    Vector4 clone() const;
    void copy(const Vector4 &v);

    Vector4 &add(const Vector4 &v);
    Vector4 &addScalar(HIGH_PRECISION s);
    Vector4 &addVectors(const Vector4 &a, const Vector4 &b);
    Vector4 &addScaledVector(const Vector4 &v, HIGH_PRECISION s);

    Vector4 &sub(const Vector4 &v);
    Vector4 &subScalar(HIGH_PRECISION s);
    Vector4 &subVectors(const Vector4 &a, const Vector4 &b);

    Vector4 &multiply(const Vector4 &v);
    Vector4 &multiplyScalar(HIGH_PRECISION scalar);
    Vector4 &applyMatrix4(const class Matrix4 &m);

    Vector4 &divide(const Vector4 &v);
    Vector4 &divideScalar(HIGH_PRECISION scalar);

    Vector4 &min(const Vector4 &v);
    Vector4 &max(const Vector4 &v);
    Vector4 &clamp(const Vector4 &min, const Vector4 &max);
    Vector4 &clampScalar(HIGH_PRECISION minVal, HIGH_PRECISION maxVal);
    Vector4 &clampLength(HIGH_PRECISION min, HIGH_PRECISION max);

    Vector4 &floor();
    Vector4 &ceil();
    Vector4 &round();
    Vector4 &roundToZero();
    Vector4 &negate();

    HIGH_PRECISION dot(const Vector4 &v) const;
    HIGH_PRECISION lengthSq() const;
    HIGH_PRECISION length() const;
    HIGH_PRECISION manhattanLength() const;

    Vector4 &normalize();
    Vector4 &setLength(HIGH_PRECISION length);

    Vector4 &lerp(const Vector4 &v, HIGH_PRECISION alpha);
    Vector4 &lerpVectors(const Vector4 &v1, const Vector4 &v2, HIGH_PRECISION alpha);

    bool equals(const Vector4 &v, float epsilon = 1e-6) const;

    Vector4 &fromArray(const std::vector<HIGH_PRECISION> &array, size_t offset = 0);
    std::vector<HIGH_PRECISION> &toArray(std::vector<HIGH_PRECISION> &array, size_t offset = 0);

    void fromMatrixColumn(const class Matrix4 &m, size_t index);
    Vector4 &setFromMatrixPosition(const class Matrix4 &m);
    Vector4 &setFromMatrixScale(const class Matrix4 &m);

    Vector4 &random();

public:
    Vector4 operator+(const Vector4 &v) const;
    Vector4 operator-(const Vector4 &v) const;
    Vector4 operator*(const Vector4 &v) const;
    Vector4 operator/(const Vector4 &v) const;
    Vector4 operator*(HIGH_PRECISION scalar) const;
    Vector4 operator/(HIGH_PRECISION scalar) const;
    void operator=(const Vector4 &v);
    bool operator==(const Vector4 &v) const;
    HIGH_PRECISION operator[](size_t index) const;

private:
    HIGH_PRECISION m_x;
    HIGH_PRECISION m_y;
    HIGH_PRECISION m_z;
    HIGH_PRECISION m_w;
};

#endif
