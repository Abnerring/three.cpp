#ifndef VECTOR2_H
#define VECTOR2_H

#include "common/BasicType.h"
#include <vector>

class Matrix3;

class Vector2
{
public:
    Vector2(HIGH_PRECISION x = 0.0, HIGH_PRECISION y = 0.0);
    ~Vector2();

    HIGH_PRECISION x() const;
    HIGH_PRECISION y() const;

    HIGH_PRECISION width();
    void width(HIGH_PRECISION value);

    HIGH_PRECISION height();
    void height(HIGH_PRECISION value);

    void set(HIGH_PRECISION x, HIGH_PRECISION y);
    void setScalar(HIGH_PRECISION scalar);

    void setX(HIGH_PRECISION x);
    void setY(HIGH_PRECISION y);

    void setComponent(size_t index, HIGH_PRECISION value); 
    HIGH_PRECISION getComponent(size_t index) const;

    Vector2 clone();
    void copy(const Vector2 &v);
    void add(const Vector2 &v);
    void addScalar(HIGH_PRECISION s);
    void addVectors(const Vector2 &a, const Vector2 &b);
    void addScaledVector(const Vector2 &v, HIGH_PRECISION s);
    void sub(const Vector2 &v);
    void subScalar(HIGH_PRECISION s);
    void subVectors(const Vector2 &a, const Vector2 &b);
    void multiply(const Vector2 &v);
    void multiplyScalar(HIGH_PRECISION scalar);
    void divide(const Vector2 &v);
    void divideScalar(HIGH_PRECISION scalar);
    void applyMatrix3(const Matrix3 &m);
    void min(const Vector2 &v);
    void max(const Vector2 &v);
    void clamp(const Vector2 &min, const Vector2 &max);
    void clampScalar(HIGH_PRECISION minVal, HIGH_PRECISION maxVal);
    void clampLength(HIGH_PRECISION min, HIGH_PRECISION max);
    void floor();
    void ceil();
    void round();
    void roundToZero();
    void negate();
    HIGH_PRECISION dot(const Vector2& v);
    HIGH_PRECISION cross(const Vector2 &v);
    HIGH_PRECISION lengthSq() const;
    HIGH_PRECISION length() const;
    HIGH_PRECISION manhattanLength();
    void normalize();
    Vector2 normalized();
    HIGH_PRECISION angle();
    HIGH_PRECISION angleTo(const Vector2 &v);
    HIGH_PRECISION distanceTo(const Vector2 &v);
    HIGH_PRECISION distanceToSquared(const Vector2 &v);
    HIGH_PRECISION manhattanDistanceTo(const Vector2 &v);
    void setLength(HIGH_PRECISION length);
    void lerp(const Vector2 &v, HIGH_PRECISION alpha);
    void lerpVectors(const Vector2 &v1, const Vector2 &v2, HIGH_PRECISION alpha);
    bool equals(const Vector2& v, float epsilon = 1e-6) const;
    void fromArray(const std::vector<HIGH_PRECISION>& array, size_t offset = 0);
    void toArray( std::vector<HIGH_PRECISION>& array, size_t offset = 0 );
    // TODO : Add function
    // fromBufferAttribute( attribute, index ) 
    void rotateAround(const Vector2& center, float angle); 
    void random();
public:
    Vector2 operator+(const Vector2 &v);
    Vector2 operator-(const Vector2 &v);
    Vector2 operator*(const Vector2 &v);
    Vector2 operator/(const Vector2 &v);
    void operator=(const Vector2 &v);
    bool operator==(const Vector2 &v) const;
    HIGH_PRECISION operator[](size_t index) const;
private:
    HIGH_PRECISION m_x;
    HIGH_PRECISION m_y;
};

#endif