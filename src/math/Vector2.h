/**
 * @file   Vector2.h
 * @brief  二维向量类
 * @author Abnerring
 * @date   2025-2026
 * @note   参考 three.js Vector2 实现
 */

#ifndef VECTOR2_H
#define VECTOR2_H

#include "core/BasicType.h"
#include "core/Property.h"
#include <vector>

class Matrix3;

/**
 * @class Vector2
 * @brief 二维向量，用于表示平面上的点或方向
 * @details 支持向量运算、插值、距离计算等功能
 * @see Vector3 Vector4
 *
 * @example
 * @code
 * Vector2 a(1, 2);
 * Vector2 b(3, 4);
 * a.add(b); // a = (4, 6)
 * @endcode
 */
class Vector2
{
public:
    static constexpr bool isVector2 = true;
    /**
     * @brief 构造二维向量
     * @param x X 分量，默认 0
     * @param y Y 分量，默认 0
     */
    Vector2(HIGH_PRECISION x = 0.0, HIGH_PRECISION y = 0.0);
    Vector2(const Vector2& source);
    Vector2(Vector2&& source);

    /** @brief 析构函数 */
    ~Vector2();

    /** @brief 获取 X 分量 @return X 值 */
    HIGH_PRECISION x;

    /** @brief 获取 Y 分量 @return Y 值 */
    HIGH_PRECISION y;

    /** @brief 获取 X 分量（width 别名）@return X 值 */
    Property<HIGH_PRECISION> width{x};

    /** @brief 获取 Y 分量（height 别名）@return Y 值 */
    Property<HIGH_PRECISION> height{y};

    /**
     * @brief 设置分量
     * @param x X 值
     * @param y Y 值
     * @return Vector2& 自身引用
     */
    Vector2 &set(HIGH_PRECISION x, HIGH_PRECISION y);

    /**
     * @brief 设置所有分量为相同值
     * @param scalar 标量值
     * @return Vector2& 自身引用
     */
    Vector2 &setScalar(HIGH_PRECISION scalar);

    /** @brief 设置 X 分量 @param x X 值 @return Vector2& 自身引用 */
    Vector2 &setX(HIGH_PRECISION x);

    /** @brief 设置 Y 分量 @param y Y 值 @return Vector2& 自身引用 */
    Vector2 &setY(HIGH_PRECISION y);

    /**
     * @brief 通过索引设置分量
     * @param index 0=x, 1=y
     * @param value 值
     * @return Vector2& 自身引用
     */
    Vector2 &setComponent(size_t index, HIGH_PRECISION value);

    /**
     * @brief 通过索引获取分量
     * @param index 0=x, 1=y
     * @return HIGH_PRECISION 分量值
     */
    HIGH_PRECISION getComponent(size_t index) const;

    /** @brief 克隆向量 @return 向量副本 */
    Vector2 clone() const;

    /**
     * @brief 复制向量
     * @param v 源向量
     * @return Vector2& 自身引用
     */
    Vector2 &copy(const Vector2 &v);

    /**
     * @brief 向量加法
     * @param v 加的向量
     * @return Vector2& 自身引用
     */
    Vector2 &add(const Vector2 &v);

    /**
     * @brief 所有分量加上标量
     * @param s 标量
     * @return Vector2& 自身引用
     */
    Vector2 &addScalar(HIGH_PRECISION s);

    /**
     * @brief 两向量相加
     * @param a 向量 1
     * @param b 向量 2
     * @return Vector2& 自身引用
     */
    Vector2 &addVectors(const Vector2 &a, const Vector2 &b);

    /**
     * @brief 加上缩放后的向量
     * @param v 向量
     * @param s 缩放因子
     * @return Vector2& 自身引用
     */
    Vector2 &addScaledVector(const Vector2 &v, HIGH_PRECISION s);

    /**
     * @brief 向量减法
     * @param v 减的向量
     * @return Vector2& 自身引用
     */
    Vector2 &sub(const Vector2 &v);

    /**
     * @brief 所有分量减去标量
     * @param s 标量
     * @return Vector2& 自身引用
     */
    Vector2 &subScalar(HIGH_PRECISION s);

    /**
     * @brief 两向量相减
     * @param a 向量 1
     * @param b 向量 2
     * @return Vector2& 自身引用
     */
    Vector2 &subVectors(const Vector2 &a, const Vector2 &b);

    /**
     * @brief 向量逐分量乘法
     * @param v 乘的向量
     * @return Vector2& 自身引用
     */
    Vector2 &multiply(const Vector2 &v);

    /**
     * @brief 所有分量乘以标量
     * @param scalar 标量
     * @return Vector2& 自身引用
     */
    Vector2 &multiplyScalar(HIGH_PRECISION scalar);

    /**
     * @brief 向量逐分量除法
     * @param v 除的向量
     * @return Vector2& 自身引用
     */
    Vector2 &divide(const Vector2 &v);

    /**
     * @brief 所有分量除以标量
     * @param scalar 标量
     * @return Vector2& 自身引用
     */
    Vector2 &divideScalar(HIGH_PRECISION scalar);

    /**
     * @brief 应用 3x3 矩阵变换（z 默认为 1）
     * @param m 矩阵
     * @return Vector2& 自身引用
     */
    Vector2 &applyMatrix3(const Matrix3 &m);

    /**
     * @brief 分量取最小值
     * @param v 比较向量
     * @return Vector2& 自身引用
     */
    Vector2 &min(const Vector2 &v);

    /**
     * @brief 分量取最大值
     * @param v 比较向量
     * @return Vector2& 自身引用
     */
    Vector2 &max(const Vector2 &v);

    /**
     * @brief 限制分量在范围内
     * @param min 最小值
     * @param max 最大值
     * @return Vector2& 自身引用
     */
    Vector2 &clamp(const Vector2 &min, const Vector2 &max);

    /**
     * @brief 限制分量在标量范围内
     * @param minVal 最小值
     * @param maxVal 最大值
     * @return Vector2& 自身引用
     */
    Vector2 &clampScalar(HIGH_PRECISION minVal, HIGH_PRECISION maxVal);

    /**
     * @brief 限制向量长度
     * @param min 最小长度
     * @param max 最大长度
     * @return Vector2& 自身引用
     */
    Vector2 &clampLength(HIGH_PRECISION min, HIGH_PRECISION max);

    /** @brief 向下取整 @return Vector2& 自身引用 */
    Vector2 &floor();

    /** @brief 向上取整 @return Vector2& 自身引用 */
    Vector2 &ceil();

    /** @brief 四舍五入取整 @return Vector2& 自身引用 */
    Vector2 &round();

    /** @brief 向零取整 @return Vector2& 自身引用 */
    Vector2 &roundToZero();

    /** @brief 取反 @return Vector2& 自身引用 */
    Vector2 &negate();

    /**
     * @brief 计算点积
     * @param v 另一个向量
     * @return HIGH_PRECISION 点积结果
     */
    HIGH_PRECISION dot(const Vector2 &v) const;

    /**
     * @brief 计算叉积（返回标量）
     * @param v 另一个向量
     * @return HIGH_PRECISION 叉积结果
     */
    HIGH_PRECISION cross(const Vector2 &v) const;

    /**
     * @brief 计算长度平方
     * @return HIGH_PRECISION 长度平方
     * @note 比较距离时更高效
     */
    HIGH_PRECISION lengthSq() const;

    /**
     * @brief 计算欧几里得长度
     * @return HIGH_PRECISION 长度
     */
    HIGH_PRECISION length() const;

    /**
     * @brief 计算曼哈顿长度
     * @return HIGH_PRECISION |x| + |y|
     */
    HIGH_PRECISION manhattanLength() const;

    /** @brief 归一化为单位向量 @return Vector2& 自身引用 */
    Vector2 &normalize();

    /**
     * @brief 获取单位向量副本
     * @return Vector2 单位向量
     */
    Vector2 getNormal() const;

    /**
     * @brief 获取与 X 轴夹角
     * @return HIGH_PRECISION 弧度
     */
    HIGH_PRECISION angle() const;

    /**
     * @brief 计算与另一向量夹角
     * @param v 另一个向量
     * @return HIGH_PRECISION 弧度
     */
    HIGH_PRECISION angleTo(const Vector2 &v) const;

    /**
     * @brief 计算到另一点距离
     * @param v 目标点
     * @return HIGH_PRECISION 距离
     */
    HIGH_PRECISION distanceTo(const Vector2 &v) const;

    /**
     * @brief 计算距离平方
     * @param v 目标点
     * @return HIGH_PRECISION 距离平方
     * @note 比较距离时更高效
     */
    HIGH_PRECISION distanceToSquared(const Vector2 &v) const;

    /**
     * @brief 计算曼哈顿距离
     * @param v 目标点
     * @return HIGH_PRECISION 曼哈顿距离
     */
    HIGH_PRECISION manhattanDistanceTo(const Vector2 &v) const;

    /**
     * @brief 设置向量长度
     * @param length 目标长度
     * @return Vector2& 自身引用
     */
    Vector2 &setLength(HIGH_PRECISION length);

    /**
     * @brief 线性插值
     * @param v 目标向量
     * @param alpha 因子 [0, 1]
     * @return Vector2& 自身引用
     */
    Vector2 &lerp(const Vector2 &v, HIGH_PRECISION alpha);

    /**
     * @brief 两向量间插值
     * @param v1 起始向量
     * @param v2 结束向量
     * @param alpha 因子 [0, 1]
     * @return Vector2& 自身引用
     */
    Vector2 &lerpVectors(const Vector2 &v1, const Vector2 &v2, HIGH_PRECISION alpha);

    /**
     * @brief 判断是否相等
     * @param v 比较向量
     * @param epsilon 容差，默认 1e-6
     * @return bool 是否相等
     */
    bool equals(const Vector2 &v, HIGH_PRECISION epsilon = 0) const;

    /**
     * @brief 从数组设置
     * @param array 数据数组
     * @param offset 偏移量，默认 0
     * @return Vector2& 自身引用
     */
    Vector2 &fromArray(const std::vector<HIGH_PRECISION> &array, size_t offset = 0);

    /**
     * @brief 写入数组
     * @param array 目标数组
     * @param offset 偏移量，默认 0
     * @return std::vector<HIGH_PRECISION>& 数组引用
     */
    std::vector<HIGH_PRECISION> &toArray(std::vector<HIGH_PRECISION> &array, size_t offset = 0) const;

    /**
     * @brief 绕中心点旋转
     * @param center 旋转中心
     * @param angle 角度（弧度）
     * @return Vector2& 自身引用
     */
    Vector2 &rotateAround(const Vector2 &center, HIGH_PRECISION angle);

    /** @brief 设为随机值 [0, 1) @return Vector2& 自身引用 */
    Vector2 &random();

public:
    /** @brief 加法运算符 @return 新向量 */
    Vector2 operator+(const Vector2 &v) const;

    /** @brief 减法运算符 @return 新向量 */
    Vector2 operator-(const Vector2 &v) const;

    /** @brief 乘法运算符 @return 新向量 */
    Vector2 operator*(const Vector2 &v) const;

    /** @brief 除法运算符 @return 新向量 */
    Vector2 operator/(const Vector2 &v) const;

    /** @brief 赋值运算符 */
    Vector2 &operator=(const Vector2 &v);

    /** @brief 相等判断 @return 是否相等 */
    bool operator==(const Vector2 &v) const;

    /** @brief 下标访问 @param index 0=x, 1=y @return 分量值 */
    HIGH_PRECISION operator[](size_t index) const;

    std::vector<HIGH_PRECISION> toArray() const;

    Vector2 &fromBufferAttribute(const class VertexAttribute &attribute, double index);

};

#endif
