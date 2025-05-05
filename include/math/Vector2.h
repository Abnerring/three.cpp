#ifndef VECTOR2_H
#define VECTOR2_H

#include "common/BasicType.h"
#include <vector>

class Matrix3;

/**
 * Class representing a 2D vector. A 2D vector is an ordered pair of numbers
 * (labeled x and y), which can be used to represent a number of things, such as:
 *
 * - A point in 2D space (i.e. a position on a plane).
 * - A direction and length across a plane. In three.js the length will
 * always be the Euclidean distance(straight-line distance) from `(0, 0)` to `(x, y)`
 * and the direction is also measured from `(0, 0)` towards `(x, y)`.
 * - Any arbitrary ordered pair of numbers.
 *
 * There are other things a 2D vector can be used to represent, such as
 * momentum vectors, complex numbers and so on, however these are the most
 * common uses in three.cpp.
 *
 * Iterating through a vector instance will yield its components `(x, y)` in
 * the corresponding order.
 * ```c++
 * Vector2 a(0, 1);
 *
 * //no arguments; will be initialised to (0, 0)
 * Vector2 b;
 *
 * auto d = a.distanceTo( b );
 * ```
 */
class Vector2
{
public:
    /**
     * Constructs a new 2D vector.
     *
     * @param {HIGH_PRECISION} [x=0] - The x value of this vector.
     * @param {HIGH_PRECISION} [y=0] - The y value of this vector.
     */
    Vector2(HIGH_PRECISION x = 0.0, HIGH_PRECISION y = 0.0);
    ~Vector2();
    /**
     * The x value of this vector.
     *
     * @return {HIGH_PRECISION}
     */
    HIGH_PRECISION x() const;
    /**
     * The y value of this vector.
     *
     * @return {HIGH_PRECISION}
     */
    HIGH_PRECISION y() const;

    /**
     * Alias for {@link Vector2#x}.
     *
     * @return {number}
     */
    HIGH_PRECISION width();
    void width(HIGH_PRECISION value);
    /**
     * Alias for {@link Vector2#y}.
     *
     * @return {number}
     */
    HIGH_PRECISION height();
    void height(HIGH_PRECISION value);

    /**
     * Sets the vector components.
     *
     * @param {HIGH_PRECISION} x - The value of the x component.
     * @param {HIGH_PRECISION} y - The value of the y component.
     * @return {Vector2} A reference to this vector.
     */
    Vector2 &set(HIGH_PRECISION x, HIGH_PRECISION y);
    /**
     * Sets the vector components to the same value.
     *
     * @param {HIGH_PRECISION} scalar - The value to set for all vector components.
     * @return {Vector2} A reference to this vector.
     */
    Vector2 &setScalar(HIGH_PRECISION scalar);

    /**
     * Sets the vector's x component to the given value
     *
     * @param {HIGH_PRECISION} x - The value to set.
     * @return {Vector2} A reference to this vector.
     */
    Vector2 &setX(HIGH_PRECISION x);

    /**
     * Sets the vector's y component to the given value
     *
     * @param {HIGH_PRECISION} y - The value to set.
     * @return {Vector2} A reference to this vector.
     */
    Vector2 &setY(HIGH_PRECISION y);

    /**
     * Allows to set a vector component with an index.
     *
     * @param {size_t} index - The component index. `0` equals to x, `1` equals to y.
     * @param {HIGH_PRECISION} value - The value to set.
     * @return {Vector2} A reference to this vector.
     */
    Vector2 &setComponent(size_t index, HIGH_PRECISION value);
    /**
     * Returns the value of the vector component which matches the given index.
     *
     * @param {size_t} index - The component index. `0` equals to x, `1` equals to y.
     * @return {HIGH_PRECISION} A vector component value.
     */
    HIGH_PRECISION getComponent(size_t index) const;
    /**
     * Returns a new vector with copied values from this instance.
     *
     * @return {Vector2} A clone of this instance.
     */
    Vector2 clone();
    /**
     * Copies the values of the given vector to this instance.
     *
     * @param {Vector2} v - The vector to copy.
     * @return {Vector2} A reference to this vector.
     */
    Vector2 &copy(const Vector2 &v);
    /**
     * Adds the given vector to this instance.
     *
     * @param {Vector2} v - The vector to add.
     * @return {Vector2} A reference to this vector.
     */
    Vector2 &add(const Vector2 &v);
    /**
     * Adds the given scalar value to all components of this instance.
     *
     * @param {HIGH_PRECISION} s - The scalar to add.
     * @return {Vector2} A reference to this vector.
     */
    Vector2 &addScalar(HIGH_PRECISION s);
    /**
     * Adds the given vectors and stores the result in this instance.
     *
     * @param {Vector2} a - The first vector.
     * @param {Vector2} b - The second vector.
     * @return {Vector2} A reference to this vector.
     */
    Vector2 &addVectors(const Vector2 &a, const Vector2 &b);
    /**
     * Adds the given vector scaled by the given factor to this instance.
     *
     * @param {Vector2} v - The vector.
     * @param {HIGH_PRECISION} s - The factor that scales `v`.
     * @return {Vector2} A reference to this vector.
     */
    Vector2 &addScaledVector(const Vector2 &v, HIGH_PRECISION s);
    /**
     * Subtracts the given vector from this instance.
     *
     * @param {Vector2} v - The vector to subtract.
     * @return {Vector2} A reference to this vector.
     */
    Vector2 &sub(const Vector2 &v);
    /**
     * Subtracts the given scalar value from all components of this instance.
     *
     * @param {HIGH_PRECISION} s - The scalar to subtract.
     * @return {Vector2} A reference to this vector.
     */
    Vector2 &subScalar(HIGH_PRECISION s);
    /**
     * Subtracts the given vectors and stores the result in this instance.
     *
     * @param {Vector2} a - The first vector.
     * @param {Vector2} b - The second vector.
     * @return {Vector2} A reference to this vector.
     */
    Vector2 &subVectors(const Vector2 &a, const Vector2 &b);
    /**
     * Multiplies the given vector with this instance.
     *
     * @param {Vector2} v - The vector to multiply.
     * @return {Vector2} A reference to this vector.
     */
    Vector2 &multiply(const Vector2 &v);
    /**
     * Multiplies the given scalar value with all components of this instance.
     *
     * @param {HIGH_PRECISION} scalar - The scalar to multiply.
     * @return {Vector2} A reference to this vector.
     */
    Vector2 &multiplyScalar(HIGH_PRECISION scalar);
    /**
     * Divides this instance by the given vector.
     *
     * @param {Vector2} v - The vector to divide.
     * @return {Vector2} A reference to this vector.
     */
    Vector2 &divide(const Vector2 &v);
    /**
     * Divides this vector by the given scalar.
     *
     * @param {HIGH_PRECISION} scalar - The scalar to divide.
     * @return {Vector2} A reference to this vector.
     */
    Vector2 &divideScalar(HIGH_PRECISION scalar);
    /**
     * Multiplies this vector (with an implicit 1 as the 3rd component) by
     * the given 3x3 matrix.
     *
     * @param {Matrix3} m - The matrix to apply.
     * @return {Vector2} A reference to this vector.
     */
    Vector2 &applyMatrix3(const Matrix3 &m);
    /**
     * If this vector's x or y value is greater than the given vector's x or y
     * value, replace that value with the corresponding min value.
     *
     * @param {Vector2} v - The vector.
     * @return {Vector2} A reference to this vector.
     */
    Vector2 &min(const Vector2 &v);
    /**
     * If this vector's x or y value is less than the given vector's x or y
     * value, replace that value with the corresponding max value.
     *
     * @param {Vector2} v - The vector.
     * @return {Vector2} A reference to this vector.
     */
    Vector2 &max(const Vector2 &v);
    /**
     * If this vector's x or y value is greater than the max vector's x or y
     * value, it is replaced by the corresponding value.
     * If this vector's x or y value is less than the min vector's x or y value,
     * it is replaced by the corresponding value.
     *
     * @param {Vector2} min - The minimum x and y values.
     * @param {Vector2} max - The maximum x and y values in the desired range.
     * @return {Vector2} A reference to this vector.
     */
    Vector2 &clamp(const Vector2 &min, const Vector2 &max);
    /**
     * If this vector's x or y values are greater than the max value, they are
     * replaced by the max value.
     * If this vector's x or y values are less than the min value, they are
     * replaced by the min value.
     *
     * @param {HIGH_PRECISION} minVal - The minimum value the components will be clamped to.
     * @param {HIGH_PRECISION} maxVal - The maximum value the components will be clamped to.
     * @return {Vector2} A reference to this vector.
     */
    Vector2 &clampScalar(HIGH_PRECISION minVal, HIGH_PRECISION maxVal);
    /**
     * If this vector's length is greater than the max value, it is replaced by
     * the max value.
     * If this vector's length is less than the min value, it is replaced by the
     * min value.
     *
     * @param {HIGH_PRECISION} min - The minimum value the vector length will be clamped to.
     * @param {HIGH_PRECISION} max - The maximum value the vector length will be clamped to.
     * @return {Vector2} A reference to this vector.
     */
    Vector2 &clampLength(HIGH_PRECISION min, HIGH_PRECISION max);

    /**
     * The components of this vector are rounded down to the nearest integer value.
     *
     * @return {Vector2} A reference to this vector.
     */
    Vector2 &floor();
    /**
     * The components of this vector are rounded up to the nearest integer value.
     *
     * @return {Vector2} A reference to this vector.
     */
    Vector2 &ceil();
    /**
     * The components of this vector are rounded to the nearest integer value
     *
     * @return {Vector2} A reference to this vector.
     */
    Vector2 &round();
    /**
     * The components of this vector are rounded towards zero (up if negative,
     * down if positive) to an integer value.
     *
     * @return {Vector2} A reference to this vector.
     */
    Vector2 &roundToZero();
    /**
     * Inverts this vector - i.e. sets x = -x and y = -y.
     *
     * @return {Vector2} A reference to this vector.
     */
    Vector2 &negate();

    /**
     * Calculates the dot product of the given vector with this instance.
     *
     * @param {Vector2} v - The vector to compute the dot product with.
     * @return {HIGH_PRECISION} The result of the dot product.
     */
    HIGH_PRECISION dot(const Vector2 &v);
    /**
     * Calculates the cross product of the given vector with this instance.
     *
     * @param {Vector2} v - The vector to compute the cross product with.
     * @return {HIGH_PRECISION} The result of the cross product.
     */
    HIGH_PRECISION cross(const Vector2 &v);
    /**
     * Computes the square of the Euclidean length (straight-line length) from
     * (0, 0) to (x, y). If you are comparing the lengths of vectors, you should
     * compare the length squared instead as it is slightly more efficient to calculate.
     *
     * @return {HIGH_PRECISION} The square length of this vector.
     */
    HIGH_PRECISION lengthSq() const;

    /**
     * Computes the  Euclidean length (straight-line length) from (0, 0) to (x, y).
     *
     * @return {HIGH_PRECISION} The length of this vector.
     */
    HIGH_PRECISION length() const;
    /**
     * Computes the Manhattan length of this vector.
     *
     * @return {HIGH_PRECISION} The length of this vector.
     */
    HIGH_PRECISION manhattanLength();
    /**
     * Converts this vector to a unit vector - that is, sets it equal to a vector
     * with the same direction as this one, but with a vector length of `1`.
     *
     * @return {Vector2} A reference to this vector.
     */
    Vector2 &normalize();
    /**
     * Get the unit vector of the vector - i.e., the vector
     * in the same direction, but with a vector length of '1'.
     *
     * @return {Vector2} is a new unit vector.
     */
    Vector2 getNormal();
    /**
     * Computes the angle in radians of this vector with respect to the positive x-axis.
     *
     * @return {HIGH_PRECISION} The angle in radians.
     */
    HIGH_PRECISION angle();
    /**
     * Returns the angle between the given vector and this instance in radians.
     *
     * @param {Vector2} v - The vector to compute the angle with.
     * @return {HIGH_PRECISION} The angle in radians.
     */
    HIGH_PRECISION angleTo(const Vector2 &v);
    /**
     * Computes the distance from the given vector to this instance.
     *
     * @param {Vector2} v - The vector to compute the distance to.
     * @return {HIGH_PRECISION} The distance.
     */
    HIGH_PRECISION distanceTo(const Vector2 &v);
    /**
     * Computes the squared distance from the given vector to this instance.
     * If you are just comparing the distance with another distance, you should compare
     * the distance squared instead as it is slightly more efficient to calculate.
     *
     * @param {Vector2} v - The vector to compute the squared distance to.
     * @return {HIGH_PRECISION} The squared distance.
     */
    HIGH_PRECISION distanceToSquared(const Vector2 &v);
    /**
     * Computes the Manhattan distance from the given vector to this instance.
     *
     * @param {Vector2} v - The vector to compute the Manhattan distance to.
     * @return {HIGH_PRECISION} The Manhattan distance.
     */
    HIGH_PRECISION manhattanDistanceTo(const Vector2 &v);
    /**
     * Sets this vector to a vector with the same direction as this one, but
     * with the specified length.
     *
     * @param {HIGH_PRECISION} length - The new length of this vector.
     * @return {Vector2} A reference to this vector.
     */
    Vector2 &setLength(HIGH_PRECISION length);
    /**
     * Linearly interpolates between the given vector and this instance, where
     * alpha is the percent distance along the line - alpha = 0 will be this
     * vector, and alpha = 1 will be the given one.
     *
     * @param {Vector2} v - The vector to interpolate towards.
     * @param {HIGH_PRECISION} alpha - The interpolation factor, typically in the closed interval `[0, 1]`.
     * @return {Vector2} A reference to this vector.
     */
    Vector2 &lerp(const Vector2 &v, HIGH_PRECISION alpha);
    /**
     * Linearly interpolates between the given vectors, where alpha is the percent
     * distance along the line - alpha = 0 will be first vector, and alpha = 1 will
     * be the second one. The result is stored in this instance.
     *
     * @param {Vector2} v1 - The first vector.
     * @param {Vector2} v2 - The second vector.
     * @param {HIGH_PRECISION} alpha - The interpolation factor, typically in the closed interval `[0, 1]`.
     * @return {Vector2} A reference to this vector.
     */
    Vector2 &lerpVectors(const Vector2 &v1, const Vector2 &v2, HIGH_PRECISION alpha);
    /**
     * Returns `true` if this vector is equal with the given one.
     *
     * @param {Vector2} v - The vector to test for equality.
     * @param {float} epsilon - The mathematical tolerances
     * @return {boolean} Whether this vector is equal with the given one.
     */
    bool equals(const Vector2 &v, float epsilon = 1e-6) const;
    /**
     * Sets this vector's x value to be `array[ offset ]` and y
     * value to be `array[ offset + 1 ]`.
     *
     * @param {std::vector<HIGH_PRECISION>} array - An array holding the vector component values.
     * @param {size_t} [offset=0] - The offset into the array.
     * @return {Vector2} A reference to this vector.
     */
    Vector2 &fromArray(const std::vector<HIGH_PRECISION> &array, size_t offset = 0);
    /**
     * Writes the components of this vector to the given array. If no array is provided,
     * the method returns a new instance.
     *
     * @param {std::vector<HIGH_PRECISION>} [array=[]] - The target array holding the vector components.
     * @param {size_t} [offset=0] - Index of the first element in the array.
     * @return {std::vector<HIGH_PRECISION>} The vector components.
     */
    std::vector<HIGH_PRECISION> &toArray(std::vector<HIGH_PRECISION> &array, size_t offset = 0);
    // TODO : Add function
    // fromBufferAttribute( attribute, index )
    /**
     * Rotates this vector around the given center by the given angle.
     *
     * @param {Vector2} center - The point around which to rotate.
     * @param {HIGH_PRECISION} angle - The angle to rotate, in radians.
     * @return {Vector2} A reference to this vector.
     */
    Vector2 &rotateAround(const Vector2 &center, float angle);
    /**
     * Sets each component of this vector to a pseudo-random value between `0` and
     * `1`, excluding `1`.
     *
     * @return {Vector2} A reference to this vector.
     */
    Vector2 &random();

public:
    Vector2 operator+(const Vector2 &v);
    Vector2 operator-(const Vector2 &v);
    Vector2 operator*(const Vector2 &v);
    Vector2 operator/(const Vector2 &v);
    void operator=(const Vector2 &v);
    bool operator==(const Vector2 &v) const;
    HIGH_PRECISION operator[](size_t index) const;

private:
    /**
     * The x value of this vector.
     *
     * @type {HIGH_PRECISION}
     */
    HIGH_PRECISION m_x;
    /**
     * The y value of this vector.
     *
     * @type {HIGH_PRECISION}
     */
    HIGH_PRECISION m_y;
};

#endif