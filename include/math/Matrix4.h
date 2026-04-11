/**
 * @file   Matrix4.h
 * @brief  4x4 矩阵类
 * @author Abnerring
 * @date   2025-2026
 * @note   参考 three.js Matrix4 实现，用于 3D 变换
 */

#ifndef MATRIX4_H
#define MATRIX4_H

#include "common/BasicType.h"
#include <span>

class Vector3;
class Matrix3;

/**
 * @class Matrix4
 * @brief 4x4 矩阵，用于 3D 变换（平移、旋转、缩放、投影）
 * @details 存储为列主序数组，共 16 个元素，支持链式调用
 * @see Matrix3 Vector3 Quaternion
 *
 * @example
 * @code
 * Matrix4 m;
 * m.makeRotationX(MathUtils::PI / 4);
 * m.multiply(m2);
 * @endcode
 */
class Matrix4
{
public:
    /**
     * @brief 构造 4x4 单位矩阵
     * @param n11-n44 矩阵元素，按列主序排列
     */
    Matrix4(
        HIGH_PRECISION n11 = 1.0, HIGH_PRECISION n12 = 0.0, HIGH_PRECISION n13 = 0.0, HIGH_PRECISION n14 = 0.0,
        HIGH_PRECISION n21 = 0.0, HIGH_PRECISION n22 = 1.0, HIGH_PRECISION n23 = 0.0, HIGH_PRECISION n24 = 0.0,
        HIGH_PRECISION n31 = 0.0, HIGH_PRECISION n32 = 0.0, HIGH_PRECISION n33 = 1.0, HIGH_PRECISION n34 = 0.0,
        HIGH_PRECISION n41 = 0.0, HIGH_PRECISION n42 = 0.0, HIGH_PRECISION n43 = 0.0, HIGH_PRECISION n44 = 1.0);

    /** @brief 析构函数 */
    ~Matrix4();

    /**
     * @brief 获取矩阵元素
     * @return std::span<const HIGH_PRECISION, 16> 16 元素只读视图
     */
    std::span<const HIGH_PRECISION, 16> elements() const;

    /**
     * @brief 设置矩阵元素
     * @param n11-n44 矩阵 16 个元素
     */
    void set(
        HIGH_PRECISION n11, HIGH_PRECISION n12, HIGH_PRECISION n13, HIGH_PRECISION n14,
        HIGH_PRECISION n21, HIGH_PRECISION n22, HIGH_PRECISION n23, HIGH_PRECISION n24,
        HIGH_PRECISION n31, HIGH_PRECISION n32, HIGH_PRECISION n33, HIGH_PRECISION n34,
        HIGH_PRECISION n41, HIGH_PRECISION n42, HIGH_PRECISION n43, HIGH_PRECISION n44);

    /** @brief 设置为单位矩阵 */
    void identity();

    /** @brief 克隆矩阵 @return 矩阵副本 */
    Matrix4 clone();

    /**
     * @brief 复制矩阵
     * @param m 源矩阵
     */
    void copy(const Matrix4 &m);

    /**
     * @brief 复制位置分量（仅影响最后一列）
     * @param m 源矩阵
     */
    void copyPosition(const Matrix4 &m);

    /**
     * @brief 从 3x3 矩阵设置
     * @param m 3x3 矩阵
     */
    void setFromMatrix3(const Matrix3 &m);

    /**
     * @brief 提取基向量
     * @param xAxis X 轴输出
     * @param yAxis Y 轴输出
     * @param zAxis Z 轴输出
     */
    void extractBasis(Vector3 &xAxis, Vector3 &yAxis, Vector3 &zAxis);

    /**
     * @brief 从基向量设置
     * @param xAxis X 轴
     * @param yAxis Y 轴
     * @param zAxis Z 轴
     */
    void makeBasis(const Vector3 &xAxis, const Vector3 &yAxis, const Vector3 &zAxis);

    /**
     * @brief 从矩阵提取旋转部分
     * @param m 源矩阵
     */
    void extractRotation(const Matrix4 &m);

    /**
     * @brief 从欧拉角设置旋转
     * @param euler 欧拉角
     */
    void makeRotationFromEuler(const class Euler &euler);

    /**
     * @brief 从四元数设置旋转
     * @param q 四元数
     */
    void makeRotationFromQuaternion(const class Quaternion &q);

    /**
     * @brief 设置为 LookAt 视图矩阵
     * @param eye 相机位置
     * @param target 观察目标
     * @param up 上方向向量
     */
    void lookAt(Vector3 &eye, Vector3 &target, Vector3 &up);

    /**
     * @brief 右乘矩阵 this = this * m
     * @param m 右乘矩阵
     */
    void multiply(const Matrix4 &m);

    /**
     * @brief 左乘矩阵 this = m * this
     * @param m 左乘矩阵
     */
    void premultiply(const Matrix4 &m);

    /**
     * @brief 矩阵乘法 this = a * b
     * @param a 左矩阵
     * @param b 右矩阵
     */
    void multiplyMatrices(const Matrix4 &a, const Matrix4 &b);

    /**
     * @brief 所有元素乘以标量
     * @param s 标量值
     */
    void multiplyScalar(HIGH_PRECISION s);

    /**
     * @brief 计算行列式
     * @return HIGH_PRECISION 行列式值
     */
    HIGH_PRECISION determinant();

    /** @brief 转置矩阵 */
    void transpose();

    /**
     * @brief 设置位置（仅影响最后一列）
     * @param x X 坐标
     * @param y Y 坐标
     * @param z Z 坐标
     */
    void setPosition(HIGH_PRECISION x, HIGH_PRECISION y, HIGH_PRECISION z);

    /**
     * @brief 设置位置
     * @param v 位置向量
     */
    void setPosition(const Vector3 &v);

    /** @brief 求逆矩阵 @warning 行列式为 0 时结果无效 */
    void invert();

    /**
     * @brief 缩放变换
     * @param v 缩放向量
     */
    void scale(const Vector3 &v);

    /**
     * @brief 获取最大缩放因子
     * @return HIGH_PRECISION 最大缩放值
     */
    HIGH_PRECISION getMaxScaleOnAxis();

    /**
     * @brief 设置为平移矩阵
     * @param x X 轴平移
     * @param y Y 轴平移
     * @param z Z 轴平移
     */
    void makeTranslation(HIGH_PRECISION x, HIGH_PRECISION y, HIGH_PRECISION z);

    /**
     * @brief 设置为平移矩阵
     * @param v 平移向量
     */
    void makeTranslation(const Vector3 &v);

    /** @brief 设置为绕 X 轴旋转矩阵 @param theta 角度（弧度） */
    void makeRotationX(float theta);

    /** @brief 设置为绕 Y 轴旋转矩阵 @param theta 角度（弧度） */
    void makeRotationY(float theta);

    /** @brief 设置为绕 Z 轴旋转矩阵 @param theta 角度（弧度） */
    void makeRotationZ(float theta);

    /**
     * @brief 设置为绕任意轴旋转矩阵
     * @param axis 旋转轴（自动归一化）
     * @param angle 角度（弧度）
     */
    void makeRotationAxis(const Vector3 &axis, float angle);

    /**
     * @brief 设置为缩放矩阵
     * @param x X 轴缩放
     * @param y Y 轴缩放
     * @param z Z 轴缩放
     */
    void makeScale(HIGH_PRECISION x, HIGH_PRECISION y, HIGH_PRECISION z);

    /**
     * @brief 设置为错切矩阵
     * @param xy-xzy 错切参数
     */
    void makeShear(HIGH_PRECISION xy, HIGH_PRECISION xz, HIGH_PRECISION yx, HIGH_PRECISION yz, HIGH_PRECISION zx, HIGH_PRECISION zy);

    /**
     * @brief 从位置、旋转、缩放组合矩阵
     * @param position 位置
     * @param quaternion 旋转（四元数）
     * @param scale 缩放
     */
    void compose(const Vector3 &position, const class Quaternion &quaternion, const Vector3 &scale); 

private:
    /** @brief 标记，用于区分 Matrix3 和 Matrix4 */
    bool m_isMatrix4 = false;

    /** @brief 矩阵元素数组（列主序） */
    HIGH_PRECISION m_elements[16] = {};
};

#endif
