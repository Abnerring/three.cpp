#ifndef VECTOR3_H
#define VECTOR3_H

#include "common/BasicType.h"

class Matrix3;
class Matrix4;

class Vector3
{
public:
    Vector3(HIGH_PRECISION x = 0.0, HIGH_PRECISION y = 0.0, HIGH_PRECISION z = 0.0);
    ~Vector3();

    HIGH_PRECISION x() const;
    HIGH_PRECISION y() const;
    HIGH_PRECISION z() const;

    void set(HIGH_PRECISION x, HIGH_PRECISION y, HIGH_PRECISION z);
    void setScalar(HIGH_PRECISION scalar);

    void setX(HIGH_PRECISION x);
    void setY(HIGH_PRECISION y);
    void setZ(HIGH_PRECISION z);

    void setComponent(size_t index, HIGH_PRECISION value);
    HIGH_PRECISION getComponent(size_t index) const;

    Vector3 clone();
    void copy(const Vector3 &v);
    void add(const Vector3 &v);
    void addScalar(HIGH_PRECISION s);
    void addVectors(const Vector3 &a, const Vector3 &b);
    void addScaledVector(const Vector3 &v, HIGH_PRECISION s);
    void sub(const Vector3 &v);
    void subScalar(HIGH_PRECISION s);
    void subVectors(const Vector3 &a, const Vector3 &b);
    void multiply(const Vector3 &v);
    void multiplyScalar(HIGH_PRECISION scalar);
    void multiplyVectors(const Vector3 &a, const Vector3 &b);
    // applyEuler( euler )
    // applyAxisAngle( axis, angle )
    void applyMatrix3(Matrix3 &m);
    void applyNormalMatrix(Matrix3 &m);
    // applyMatrix4( m )
    // applyQuaternion( q )
    // project( camera )
    // unproject( camera )
    // transformDirection( m )
    void divide(const Vector3 &v);
    void divideScalar(HIGH_PRECISION scalar);
    void min(const Vector3 &v);
    void max(const Vector3 &v);
    void clamp(const Vector3 &min, const Vector3 &max);
    void clampScalar(HIGH_PRECISION minVal, HIGH_PRECISION maxVal);
    void clampLength(HIGH_PRECISION min, HIGH_PRECISION max);
    void floor();
    void ceil();
    void round();
    void roundToZero();
    void negate();
    HIGH_PRECISION dot(const Vector3 &v) const;
    HIGH_PRECISION lengthSq() const;
    HIGH_PRECISION length() const;
    HIGH_PRECISION manhattanLength() const;
    void normalize();
    Vector3 normalized();
    void setLength(HIGH_PRECISION length);
    void lerp(const Vector3 &v, HIGH_PRECISION alpha);
    void lerpVectors(const Vector3 &v1, const Vector3 &v2, HIGH_PRECISION alpha);
    void cross(const Vector3 &v);
    void crossVectors(const Vector3 &a, const Vector3 &b);
    void projectOnVector(const Vector3 &v);
    void projectOnPlane(const Vector3 &planeNormal);
    void reflect(const Vector3 &normal);
    float angleTo(const Vector3 &v);
    HIGH_PRECISION distanceTo(const Vector3 &v) const;
    HIGH_PRECISION distanceToSquared(const Vector3 &v) const;
    HIGH_PRECISION manhattanDistanceTo(const Vector3 &v) const;
    // setFromSpherical(Spherical s )
    void setFromSphericalCoords(float radius, float phi, float theta);
    // setFromCylindrical( c )
    void setFromCylindricalCoords(float radius, float theta, HIGH_PRECISION y);
    // setFromMatrixPosition( m )
    // setFromMatrixScale( m )
    void setFromMatrixColumn(const Matrix4 &m, size_t index);
    void setFromMatrix3Column(const Matrix3 &m, size_t index);
    // setFromEuler( e )
    // setFromColor( c )
    bool equals(const Vector3 &v, float epsilon = 1e-6) const;
    void fromArray(const std::vector<HIGH_PRECISION> &array, size_t offset = 0);
    void toArray(std::vector<HIGH_PRECISION> &array, size_t offset = 0);
    // fromBufferAttribute( attribute, index )
    void random();
    void randomDirection();

public:
    Vector3 operator+(const Vector3 &v);
    Vector3 operator-(const Vector3 &v);
    Vector3 operator*(const Vector3 &v);
    Vector3 operator/(const Vector3 &v);
    void operator=(const Vector3 &v);
    bool operator==(const Vector3 &v) const;
    HIGH_PRECISION operator[](size_t index) const;

private:
    HIGH_PRECISION m_x;
    HIGH_PRECISION m_y;
    HIGH_PRECISION m_z;
};

#endif