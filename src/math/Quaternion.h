/**
 * @file   Quaternion.h
 * @brief  四元数类
 * @author Abnerring
 * @date   2025-2026
 * @note   参考 three.js Quaternion 实现，用于表示 3D 旋转
 */

#ifndef QUATERNION_H
#define QUATERNION_H

#include "core/BasicType.h"
#include "core/Property.h"
#include <vector>
#include <functional>

class Vector3;
class Euler;
class Matrix3;
class Matrix4;

/**
 * @class Quaternion
 * @brief 四元数，用于表示 3D 旋转
 * @details 由四个分量 (x, y, z, w) 组成，w 为标量部分
 *        单位四元数表示旋转，可避免万向节锁并支持平滑插值
 * @see Vector3 Euler Matrix4
 *
 * @example
 * @code
 * Quaternion q1, q2;
 * q1.setFromAxisAngle(Vector3(1, 0, 0), MathUtils::PI / 4);
 * q2.slerp(q1, 0.5);
 * @endcode
 */
class Quaternion
{
    friend struct NativeObjectAccess;
private:
    std::function<void()> m_onChangeCallback = [] {};
    /** @brief x 分量（向量部分）*/
    HIGH_PRECISION m_x;

    /** @brief y 分量（向量部分）*/
    HIGH_PRECISION m_y;

    /** @brief z 分量（向量部分）*/
    HIGH_PRECISION m_z;

    /** @brief w 分量（标量部分）*/
    HIGH_PRECISION m_w;
public:
    static constexpr bool isQuaternion = true;
    /**
     * @brief 构造四元数
     * @param x x 分量（向量部分），默认 0
     * @param y y 分量（向量部分），默认 0
     * @param z z 分量（向量部分），默认 0
     * @param w w 分量（标量部分），默认 1
     */
    Quaternion(HIGH_PRECISION x = 0.0, HIGH_PRECISION y = 0.0, HIGH_PRECISION z = 0.0, HIGH_PRECISION w = 1.0);

    Quaternion(const Quaternion &quaternion);
    Quaternion(Quaternion&& source);
    /** @brief 析构函数 */
    ~Quaternion();

    /** @brief 获取 x 分量 @return x 值 */
    Property<HIGH_PRECISION> x{m_x, m_onChangeCallback};

    /** @brief 获取 y 分量 @return y 值 */
    Property<HIGH_PRECISION> y{m_y, m_onChangeCallback};

    /** @brief 获取 z 分量 @return z 值 */
    Property<HIGH_PRECISION> z{m_z, m_onChangeCallback};

    /** @brief 获取 w 分量 @return w 值 */
    Property<HIGH_PRECISION> w{m_w, m_onChangeCallback};

    /**
     * @brief 设置四元数分量
     * @param x x 分量
     * @param y y 分量
     * @param z z 分量
     * @param w w 分量
     * @return Quaternion& 自身引用
     */
    Quaternion &set(HIGH_PRECISION x, HIGH_PRECISION y, HIGH_PRECISION z, HIGH_PRECISION w);

    /**
     * @brief 设置所有分量为相同值
     * @param s 标量值
     * @return Quaternion& 自身引用
     */
    Quaternion &setScalar(HIGH_PRECISION s);

    /** @brief 设置 x 分量 @param x x 值 @return Quaternion& 自身引用 */
    Quaternion &setX(HIGH_PRECISION x);

    /** @brief 设置 y 分量 @param y y 值 @return Quaternion& 自身引用 */
    Quaternion &setY(HIGH_PRECISION y);

    /** @brief 设置 z 分量 @param z z 值 @return Quaternion& 自身引用 */
    Quaternion &setZ(HIGH_PRECISION z);

    /** @brief 设置 w 分量 @param w w 值 @return Quaternion& 自身引用 */
    Quaternion &setW(HIGH_PRECISION w);

    /** @brief 克隆四元数 @return 四元数副本 */
    Quaternion clone() const;

    /**
     * @brief 复制四元数
     * @param q 源四元数
     */
    Quaternion &copy(const Quaternion &q);

    /**
     * @brief 从欧拉角设置
     * @param euler 欧拉角
     * @return Quaternion& 自身引用
     */
    Quaternion &setFromEuler(const Euler &euler, bool update = true);

    /**
     * @brief 从轴角设置
     * @param axis 旋转轴（自动归一化）
     * @param angle 角度（弧度）
     * @return Quaternion& 自身引用
     */
    Quaternion &setFromAxisAngle(const Vector3 &axis, HIGH_PRECISION angle);

    /**
     * @brief 从旋转矩阵设置
     * @param m 旋转矩阵
     * @return Quaternion& 自身引用
     */
    Quaternion &setFromRotationMatrix(const Matrix4 &m);

    /**
     * @brief 从两个单位向量设置
     * @param v1 起始单位向量
     * @param v2 结束单位向量
     * @return Quaternion& 自身引用
     */
    Quaternion &setFromUnitVectors(const Vector3 &v1, const Vector3 &v2);

    /**
     * @brief 判断是否相等
     * @param q 比较四元数
     * @param epsilon 容差
     * @return bool 是否相等
     * @note 考虑四元数 q 和 -q 表示相同旋转
     */
    bool isQuaternionEqual(const Quaternion &q, HIGH_PRECISION epsilon = 0) const;

    /** @brief 归一化为单位四元数 @return Quaternion& 自身引用 */
    Quaternion &normalize();

    /** @brief 共轭四元数（取反向量部分）@return Quaternion& 自身引用 */
    Quaternion &conjugate();

    /** @brief 求逆四元数 @return Quaternion& 自身引用 */
    Quaternion &invert();

    /**
     * @brief 分量加法
     * @param q 四元数
     * @return Quaternion& 自身引用
     */
    Quaternion &add(const Quaternion &q);

    /**
     * @brief 分量减法
     * @param q 四元数
     * @return Quaternion& 自身引用
     */
    Quaternion &sub(const Quaternion &q);

    /**
     * @brief 计算点积
     * @param q 另一个四元数
     * @return HIGH_PRECISION 点积结果
     */
    HIGH_PRECISION dot(const Quaternion &q) const;

    /**
     * @brief 计算长度的平方
     * @return HIGH_PRECISION 长度平方
     */
    HIGH_PRECISION lengthSq() const;

    /**
     * @brief 计算长度
     * @return HIGH_PRECISION 长度
     */
    HIGH_PRECISION length() const;

    /**
     * @brief 右乘四元数 this = this * q
     * @param q 四元数
     * @return Quaternion& 自身引用
     */
    Quaternion &multiply(const Quaternion &q);

    /**
     * @brief 左乘四元数 this = q * this
     * @param q 四元数
     * @return Quaternion& 自身引用
     */
    Quaternion &premultiply(const Quaternion &q);

    /**
     * @brief 四元数乘法 this = a * b
     * @param a 四元数
     * @param b 四元数
     * @return Quaternion& 自身引用
     */
    Quaternion &multiplyQuaternions(const Quaternion &a, const Quaternion &b);

    /**
     * @brief 球面线性插值
     * @param qb 目标四元数
     * @param t 插值因子 [0, 1]
     * @return Quaternion& 自身引用
     * @note 提供平滑旋转插值
     */
    Quaternion &slerp(const Quaternion &qb, HIGH_PRECISION t);

    /**
     * @brief 两四元数球面线性插值
     * @param qa 起始四元数
     * @param qb 结束四元数
     * @param t 插值因子 [0, 1]
     * @return Quaternion& 自身引用
     */
    Quaternion &slerpQuaternions(const Quaternion &qa, const Quaternion &qb, HIGH_PRECISION t);

    /**
     * @brief 线性插值（非球面）
     * @param qb 目标四元数
     * @param t 插值因子 [0, 1]
     * @return Quaternion& 自身引用
     * @note 速度更快但不平滑
     */
    Quaternion &lerp(const Quaternion &qb, HIGH_PRECISION t);

    /**
     * @brief 所有分量乘以标量
     * @param s 标量
     * @return Quaternion& 自身引用
     */
    Quaternion &multiplyScalar(HIGH_PRECISION s);

    /**
     * @brief 所有分量除以标量
     * @param s 标量
     * @return Quaternion& 自身引用
     */
    Quaternion &divideScalar(HIGH_PRECISION s);

    /**
     * @brief 判断是否相等
     * @param q 比较四元数
     * @param epsilon 容差
     * @return bool 是否相等
     */
    bool equals(const Quaternion &q, HIGH_PRECISION epsilon = 0) const;

    /**
     * @brief 从数组设置
     * @param array 数据数组
     * @param offset 偏移量
     * @return Quaternion& 自身引用
     */
    Quaternion &fromArray(const std::vector<HIGH_PRECISION> &array, size_t offset = 0);

    /**
     * @brief 写入数组
     * @param array 目标数组
     * @param offset 偏移量
     * @return std::vector<HIGH_PRECISION>& 数组引用
     */
    std::vector<HIGH_PRECISION> &toArray(std::vector<HIGH_PRECISION> &array, size_t offset = 0) const;

    /**
     * @brief 应用到向量
     * @param v 输入向量
     * @return Vector3& 变换后的向量引用
     */
    Vector3 &applyToVector3(Vector3 &v) const;

    /**
     * @brief 转换为欧拉角
     * @param euler 输出欧拉角
     */
    void toEuler(Euler &euler) const;

    /**
     * @brief 转换为旋转矩阵
     * @param m 输出 4x4 矩阵
     */
    void toRotationMatrix(Matrix4 &m) const;

public:
    /** @brief 加法运算符 @return 新四元数 */
    Quaternion operator+(const Quaternion &q) const;

    /** @brief 减法运算符 @return 新四元数 */
    Quaternion operator-(const Quaternion &q) const;

    /** @brief 乘法运算符 @return 新四元数 */
    Quaternion operator*(const Quaternion &q) const;

    /** @brief 标量乘法运算符 @return 新四元数 */
    Quaternion operator*(HIGH_PRECISION s) const;

    /** @brief 标量除法运算符 @return 新四元数 */
    Quaternion operator/(HIGH_PRECISION s) const;

    /** @brief 赋值运算符 */
    Quaternion &operator=(const Quaternion &q);
    Quaternion& operator=(Quaternion&& source);

    /** @brief 相等判断 @return 是否相等 */
    bool operator==(const Quaternion &q) const;

    Quaternion &_onChange(std::function<void()> callback);
    HIGH_PRECISION angleTo(const Quaternion &q) const;
    Quaternion &rotateTowards(const Quaternion &q, HIGH_PRECISION step);
    Quaternion &identity();
    Quaternion &random();
    std::vector<HIGH_PRECISION> toArray() const;

    static void slerpFlat(std::vector<HIGH_PRECISION> &dst, size_t dstOffset, const std::vector<HIGH_PRECISION> &src0, size_t srcOffset0, const std::vector<HIGH_PRECISION> &src1, size_t srcOffset1, HIGH_PRECISION t);
    static std::vector<HIGH_PRECISION> &multiplyQuaternionsFlat(std::vector<HIGH_PRECISION> &dst, size_t dstOffset, const std::vector<HIGH_PRECISION> &src0, size_t srcOffset0, const std::vector<HIGH_PRECISION> &src1, size_t srcOffset1);
    std::vector<HIGH_PRECISION> toJSON() const;
    Quaternion &fromBufferAttribute(const class BufferAttribute &attribute, size_t index);

};

#endif
