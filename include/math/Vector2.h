#ifndef VECTOR2
#define VECTOR2

#include "common/BasicType.h"

class Vector2
{
public:
    Vector2(HIGH_PRECISION x = 0, HIGH_PRECISION y = 0);
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
    // void applyMatrix3(m);
    void min(const Vector2 &v);
    void max(const Vector2 &v);
    void clamp(const Vector2& min, const Vector2& max);
    
public:
    Vector2 operator+(const Vector2 &v) const;
    Vector2 operator-(const Vector2 &v) const;
    Vector2 operator*(const Vector2 &v) const;
    Vector2 operator/(const Vector2 &v) const;

private:
    HIGH_PRECISION m_x;
    HIGH_PRECISION m_y;
};

#endif