/**
 * @file   Vector3.h
 * @brief  三维向量类
 * @author Abnerring
 * @date   2025-2026
 * @note   参考 three.js Vector3 实现
 */

#ifndef VECTOR3_H
#define VECTOR3_H

#include "core/BasicType.h"
#include <vector>

class Matrix3;
class Matrix4;

/**
 * @class Vector3
 * @brief 三维向量，用于表示空间中的点、方向或位移
 * @details 支持向量运算、插值、几何计算、矩阵变换等功能
 * @see Vector2 Vector4
 *
 * @example
 * @code
 * Vector3 a(1, 2, 3);
 * Vector3 b(4, 5, 6);
 * a.add(b); // a = (5, 7, 9)
 * @endcode
 */
class Vector3 {
  public:
    static constexpr bool isVector3 = true;
    /**
     * @brief 构造新的三维向量
     * @param x X 分量，默认 0
     * @param y Y 分量，默认 0
     * @param z Z 分量，默认 0
     */
    Vector3(HIGH_PRECISION x = 0.0, HIGH_PRECISION y = 0.0, HIGH_PRECISION z = 0.0);

    /** @brief 析构函数 */
    ~Vector3();

    /** @brief 获取 X 分量 @return X 分量值 */
    HIGH_PRECISION x;

    /** @brief 获取 Y 分量 @return Y 分量值 */
    HIGH_PRECISION y;

    /** @brief 获取 Z 分量 @return Z 分量值 */
    HIGH_PRECISION z;

    /**
     * @brief 设置向量分量
     * @param x X 分量
     * @param y Y 分量
     * @param z Z 分量
     */
    Vector3& set(HIGH_PRECISION x, HIGH_PRECISION y, HIGH_PRECISION z);

    /**
     * @brief 设置所有分量为相同值
     * @param scalar 标量值
     */
    Vector3& set(HIGH_PRECISION x, HIGH_PRECISION y);
    Vector3& setScalar(HIGH_PRECISION scalar);

    /** @brief 设置 X 分量 @param x X 分量值 */
    Vector3& setX(HIGH_PRECISION x);

    /** @brief 设置 Y 分量 @param y Y 分量值 */
    Vector3& setY(HIGH_PRECISION y);

    /** @brief 设置 Z 分量 @param z Z 分量值 */
    Vector3& setZ(HIGH_PRECISION z);

    /**
     * @brief 通过索引设置分量
     * @param index 分量索引，0 为 x，1 为 y，2 为 z
     * @param value 要设置的值
     */
    Vector3& setComponent(size_t index, HIGH_PRECISION value);

    /**
     * @brief 通过索引获取分量
     * @param index 分量索引，0 为 x，1 为 y，2 为 z
     * @return 分量值
     */
    HIGH_PRECISION getComponent(size_t index) const;

    /** @brief 克隆向量 @return 向量副本 */
    Vector3 clone() const;

    /**
     * @brief 复制向量
     * @param v 要复制的源向量
     */
    Vector3& copy(const Vector3& v);

    /**
     * @brief 向量加法
     * @param v 要加的向量
     */
    Vector3& add(const Vector3& v);

    /**
     * @brief 所有分量加上标量
     * @param s 标量值
     */
    Vector3& addScalar(HIGH_PRECISION s);

    /**
     * @brief 两向量相加，结果存入自身
     * @param a 第一个向量
     * @param b 第二个向量
     */
    Vector3& addVectors(const Vector3& a, const Vector3& b);

    /**
     * @brief 加上缩放后的向量
     * @param v 向量
     * @param s 缩放因子
     */
    Vector3& addScaledVector(const Vector3& v, HIGH_PRECISION s);

    /**
     * @brief 向量减法
     * @param v 要减的向量
     */
    Vector3& sub(const Vector3& v);

    /**
     * @brief 所有分量减去标量
     * @param s 标量值
     */
    Vector3& subScalar(HIGH_PRECISION s);

    /**
     * @brief 两向量相减，结果存入自身
     * @param a 第一个向量
     * @param b 第二个向量
     */
    Vector3& subVectors(const Vector3& a, const Vector3& b);

    /**
     * @brief 向量逐分量乘法
     * @param v 要乘的向量
     */
    Vector3& multiply(const Vector3& v);

    /**
     * @brief 所有分量乘以标量
     * @param scalar 标量值
     */
    Vector3& multiplyScalar(HIGH_PRECISION scalar);

    /**
     * @brief 两向量相乘，结果存入自身
     * @param a 第一个向量
     * @param b 第二个向量
     */
    Vector3& multiplyVectors(const Vector3& a, const Vector3& b);

    /**
     * @brief 应用欧拉角变换
     * @param euler 欧拉角
     */
    Vector3& applyEuler(const class Euler& euler);

    /**
     * @brief 绕轴旋转
     * @param axis 旋转轴
     * @param angle 旋转角度（弧度）
     */
    Vector3& applyAxisAngle(const Vector3& axis, HIGH_PRECISION angle);

    /**
     * @brief 应用 3x3 矩阵变换
     * @param m 3x3 矩阵
     */
    Vector3& applyMatrix3(const Matrix3& m);

    /**
     * @brief 应用法线矩阵变换
     * @param m 法线矩阵
     */
    Vector3& applyNormalMatrix(const Matrix3& m);

    /**
     * @brief 应用 4x4 矩阵变换
     * @param m 4x4 矩阵
     */
    Vector3& applyMatrix4(const Matrix4& m);

    /**
     * @brief 应用四元数变换
     * @param q 四元数
     */
    Vector3& applyQuaternion(const class Quaternion& q);

    /**
     * @brief 变换方向向量
     * @param m 4x4 矩阵
     */
    Vector3& transformDirection(const Matrix4& m);

    /**
     * @brief 向量逐分量除法
     * @param v 要除的向量
     */
    Vector3& divide(const Vector3& v);

    /**
     * @brief 所有分量除以标量
     * @param scalar 标量值
     */
    Vector3& divideScalar(HIGH_PRECISION scalar);

    /**
     * @brief 分量取最小值
     * @param v 比较向量，当前分量大于 v 时替换
     */
    Vector3& min(const Vector3& v);

    /**
     * @brief 分量取最大值
     * @param v 比较向量，当前分量小于 v 时替换
     */
    Vector3& max(const Vector3& v);

    /**
     * @brief 限制分量在范围内
     * @param min 最小值向量
     * @param max 最大值向量
     */
    Vector3& clamp(const Vector3& min, const Vector3& max);

    /**
     * @brief 限制分量在标量范围内
     * @param minVal 最小值
     * @param maxVal 最大值
     */
    Vector3& clampScalar(HIGH_PRECISION minVal, HIGH_PRECISION maxVal);

    /**
     * @brief 限制向量长度在范围内
     * @param min 最小长度
     * @param max 最大长度
     */
    Vector3& clampLength(HIGH_PRECISION min, HIGH_PRECISION max);

    /** @brief 向下取整 */
    Vector3& floor();

    /** @brief 向上取整 */
    Vector3& ceil();

    /** @brief 四舍五入取整 */
    Vector3& round();

    /** @brief 向零取整（向内取整） */
    Vector3& roundToZero();

    /** @brief 取反，向量变为 -x, -y, -z */
    Vector3& negate();

    /**
     * @brief 计算点积
     * @param v 另一个向量
     * @return HIGH_PRECISION 点积结果
     */
    HIGH_PRECISION dot(const Vector3& v) const;

    /**
     * @brief 计算长度的平方
     * @return HIGH_PRECISION 长度的平方
     * @note 比较距离时使用此方法更高效
     */
    HIGH_PRECISION lengthSq() const;

    /**
     * @brief 计算欧几里得长度
     * @return HIGH_PRECISION 向量长度
     */
    HIGH_PRECISION length() const;

    /**
     * @brief 计算曼哈顿长度
     * @return HIGH_PRECISION |x| + |y| + |z|
     */
    HIGH_PRECISION manhattanLength() const;

    /** @brief 归一化为单位向量 @warning 如果长度为 0，结果不确定 */
    Vector3& normalize();

    /**
     * @brief 获取归一化副本
     * @return Vector3 归一化后的新向量
     * @note 不修改原向量
     */
    Vector3 normalized();

    /**
     * @brief 设置向量长度
     * @param length 目标长度
     */
    Vector3& setLength(HIGH_PRECISION length);

    /**
     * @brief 线性插值
     * @param v 目标向量
     * @param alpha 插值因子 [0, 1]
     * @note alpha=0 返回自身，alpha=1 返回 v
     */
    Vector3& lerp(const Vector3& v, HIGH_PRECISION alpha);

    /**
     * @brief 两向量间线性插值
     * @param v1 起始向量
     * @param v2 结束向量
     * @param alpha 插值因子 [0, 1]
     */
    Vector3& lerpVectors(const Vector3& v1, const Vector3& v2, HIGH_PRECISION alpha);

    /**
     * @brief 计算叉积
     * @param v 另一个向量
     */
    Vector3& cross(const Vector3& v);

    /**
     * @brief 两向量叉积，结果存入自身
     * @param a 第一个向量
     * @param b 第二个向量
     */
    Vector3& crossVectors(const Vector3& a, const Vector3& b);

    /**
     * @brief 投影到向量上
     * @param v 投影目标向量
     */
    Vector3& projectOnVector(const Vector3& v);

    /**
     * @brief 投影到平面上
     * @param planeNormal 平面法向量
     */
    Vector3& projectOnPlane(const Vector3& planeNormal);

    /**
     * @brief 绕法向量反射
     * @param normal 反射法向量
     */
    Vector3& reflect(const Vector3& normal);

    /**
     * @brief 计算与另一向量的夹角
     * @param v 另一个向量
     * @return HIGH_PRECISION 夹角（弧度）
     */
    HIGH_PRECISION angleTo(const Vector3& v) const;

    /**
     * @brief 计算到另一点的距离
     * @param v 目标点
     * @return HIGH_PRECISION 距离
     */
    HIGH_PRECISION distanceTo(const Vector3& v) const;

    /**
     * @brief 计算到另一点距离的平方
     * @param v 目标点
     * @return HIGH_PRECISION 距离的平方
     * @note 比较距离时使用此方法更高效
     */
    HIGH_PRECISION distanceToSquared(const Vector3& v) const;

    /**
     * @brief 计算曼哈顿距离
     * @param v 目标点
     * @return HIGH_PRECISION 曼哈顿距离
     */
    HIGH_PRECISION manhattanDistanceTo(const Vector3& v) const;

    /**
     * @brief 从球坐标设置
     * @param radius 半径
     * @param phi 极角（弧度）
     * @param theta 方位角（弧度）
     */
    Vector3& setFromSphericalCoords(HIGH_PRECISION radius, HIGH_PRECISION phi,
                                    HIGH_PRECISION theta);
    Vector3& setFromSpherical(const class Spherical& spherical);

    /**
     * @brief 从柱坐标设置
     * @param radius 半径
     * @param theta 角度（弧度）
     * @param y Y 分量
     */
    Vector3& setFromCylindricalCoords(HIGH_PRECISION radius, HIGH_PRECISION theta,
                                      HIGH_PRECISION y);
    Vector3& setFromCylindrical(const class Cylindrical& cylindrical);

    /**
     * @brief 从矩阵获取位置
     * @param m 4x4 矩阵
     */
    Vector3& setFromMatrixPosition(const Matrix4& m);

    /**
     * @brief 从矩阵获取缩放
     * @param m 4x4 矩阵
     */
    Vector3& setFromMatrixScale(const Matrix4& m);

    /**
     * @brief 从矩阵列设置
     * @param m 4x4 矩阵
     * @param index 列索引
     */
    Vector3& setFromMatrixColumn(const Matrix4& m, size_t index);

    /**
     * @brief 从 3x3 矩阵列设置
     * @param m 3x3 矩阵
     * @param index 列索引
     */
    Vector3& setFromMatrix3Column(const Matrix3& m, size_t index);

    /**
     * @brief 从欧拉角设置
     * @param e 欧拉角
     */
    Vector3& setFromEuler(const class Euler& e);

    /**
     * @brief 从颜色设置
     * @param c 颜色
     */
    Vector3& setFromColor(const class Color& c);

    /**
     * @brief 判断是否相等
     * @param v 比较向量
     * @param epsilon 容差，默认 1e-6
     * @return bool 是否相等
     */
    bool equals(const Vector3& v, HIGH_PRECISION epsilon = 0) const;

    /**
     * @brief 从数组设置
     * @param array 数据数组
     * @param offset 偏移量，默认 0
     */
    Vector3& fromArray(const std::vector<HIGH_PRECISION>& array, size_t offset = 0);

    /**
     * @brief 写入数组
     * @param array 目标数组
     * @param offset 偏移量，默认 0
     */
    std::vector<HIGH_PRECISION>& toArray(std::vector<HIGH_PRECISION>& array,
                                         size_t offset = 0) const;

    /**
     * @brief 设为随机值 [0, 1)
     */
    Vector3& random();

    /**
     * @brief 设为随机单位向量
     */
    Vector3& randomDirection();

  public:
    /** @brief 向量加法运算符 @return 新向量 */
    Vector3 operator+(const Vector3& v) const;

    /** @brief 向量减法运算符 @return 新向量 */
    Vector3 operator-(const Vector3& v) const;

    /** @brief 向量乘法运算符 @return 新向量 */
    Vector3 operator*(const Vector3& v) const;

    /** @brief 向量除法运算符 @return 新向量 */
    Vector3 operator/(const Vector3& v) const;

    /** @brief 赋值运算符 */
    Vector3& operator=(const Vector3& v);

    /** @brief 相等判断运算符 @return 是否相等 */
    bool operator==(const Vector3& v) const;

    /** @brief 下标访问运算符 @param index 分量索引 @return 分量值 */
    HIGH_PRECISION operator[](size_t index) const;

    std::vector<HIGH_PRECISION> toArray() const;

    Vector3& project(const class Camera& camera);
    Vector3& unproject(const class Camera& camera);
    Vector3& fromBufferAttribute(const class VertexAttribute& attribute, double index);

};

#endif
