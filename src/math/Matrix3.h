/**
 * @file   Matrix3.h
 * @brief  3x3 矩阵类
 * @author Abnerring
 * @date   2025-2026
 * @note   参考 three.js Matrix3 实现，用于 2D 变换和法线变换
 */

#ifndef MATRIX3_H
#define MATRIX3_H

#include "core/BasicType.h"
#include <vector>
#include <span>
#include <array>

class Vector2;
class Vector3;
class Matrix4;

/**
 * @class Matrix3
 * @brief 3x3 矩阵，用于 2D 变换、UV 变换、法线变换等
 * @details 存储为列主序数组，共 9 个元素
 * @see Matrix4 Vector2
 */
class Matrix3
{
public:
    static constexpr bool isMatrix3 = true;
    /**
     * @brief 构造 3x3 单位矩阵
     * @param n11-n33 矩阵元素，按列主序排列
     */
    Matrix3(
        HIGH_PRECISION n11 = 1.0, HIGH_PRECISION n12 = 0.0, HIGH_PRECISION n13 = 0.0,
        HIGH_PRECISION n21 = 0.0, HIGH_PRECISION n22 = 1.0, HIGH_PRECISION n23 = 0.0,
        HIGH_PRECISION n31 = 0.0, HIGH_PRECISION n32 = 0.0, HIGH_PRECISION n33 = 1.0);

    /** @brief 析构函数 */
    ~Matrix3();

    /**
     * @brief 获取矩阵元素
     * @return std::span<const HIGH_PRECISION, 9> 16 元素只读视图
     */
    std::span<const HIGH_PRECISION, 9> elements() const;
    std::span<HIGH_PRECISION, 9> elements();

    /**
     * @brief 设置矩阵元素
     * @param n11-n33 矩阵元素
     */
    Matrix3 &set(
        HIGH_PRECISION n11, HIGH_PRECISION n12, HIGH_PRECISION n13,
        HIGH_PRECISION n21, HIGH_PRECISION n22, HIGH_PRECISION n23,
        HIGH_PRECISION n31, HIGH_PRECISION n32, HIGH_PRECISION n33);

    /** @brief 设置为单位矩阵 */
    Matrix3 &identity();

    /**
     * @brief 复制矩阵
     * @param m 源矩阵
     */
    Matrix3 &copy(const Matrix3 &m);

    /**
     * @brief 右乘矩阵
     * @param m 右乘矩阵 this = this * m
     */
    Matrix3 &multiply(const Matrix3 &m);

    /**
     * @brief 左乘矩阵
     * @param m 左乘矩阵 this = m * this
     */
    Matrix3 &premultiply(const Matrix3 &m);

    /**
     * @brief 矩阵乘法
     * @param a 左矩阵
     * @param b 右矩阵 this = a * b
     */
    Matrix3 &multiplyMatrices(const Matrix3 &a, const Matrix3 &b);

    /**
     * @brief 所有元素乘以标量
     * @param s 标量
     */
    Matrix3 &multiplyScalar(HIGH_PRECISION s);

    /**
     * @brief 计算行列式
     * @return HIGH_PRECISION 行列式值
     */
    HIGH_PRECISION determinant() const;

    /** @brief 求逆矩阵 @warning 行列式为 0 时结果无效 */
    Matrix3 &invert();

    /** @brief 转置矩阵 */
    Matrix3 &transpose();

    /**
     * @brief 转置到数组
     * @param r 目标数组
     */
    Matrix3 &transposeIntoArray(std::array<HIGH_PRECISION, 9> &r);

    /**
     * @brief 设置 UV 变换
     * @param tx X 轴平移
     * @param ty Y 轴平移
     * @param sx X 轴缩放
     * @param sy Y 轴缩放
     * @param rotation 旋转角度（弧度）
     * @param cx 旋转中心 X
     * @param cy 旋转中心 Y
     */
    Matrix3 &setUvTransform(HIGH_PRECISION tx, HIGH_PRECISION ty, HIGH_PRECISION sx, HIGH_PRECISION sy, HIGH_PRECISION rotation, HIGH_PRECISION cx, HIGH_PRECISION cy);

    /**
     * @brief 缩放变换
     * @param sx X 轴缩放
     * @param sy Y 轴缩放
     */
    Matrix3 &scale(HIGH_PRECISION sx, HIGH_PRECISION sy);

    /**
     * @brief 旋转变换
     * @param theta 旋转角度（弧度）
     */
    Matrix3 &rotate(HIGH_PRECISION theta);

    /**
     * @brief 平移变换
     * @param tx X 轴平移
     * @param ty Y 轴平移
     */
    Matrix3 &translate(HIGH_PRECISION tx, HIGH_PRECISION ty);

    /**
     * @brief 设置为平移矩阵
     * @param v 平移向量
     */
    Matrix3 &makeTranslation(const Vector2 &v);

    /**
     * @brief 设置为平移矩阵
     * @param x X 轴平移
     * @param y Y 轴平移
     */
    Matrix3 &makeTranslation(HIGH_PRECISION x, HIGH_PRECISION y);

    /**
     * @brief 设置为旋转矩阵
     * @param theta 旋转角度（弧度）
     */
    Matrix3 &makeRotation(HIGH_PRECISION theta);

    /**
     * @brief 设置为缩放矩阵
     * @param x X 轴缩放
     * @param y Y 轴缩放
     */
    Matrix3 &makeScale(HIGH_PRECISION x, HIGH_PRECISION y);

    /**
     * @brief 判断是否相等
     * @param matrix 比较矩阵
     * @param epsilon 容差
     * @return bool 是否相等
     */
    bool equals(const Matrix3 &matrix, HIGH_PRECISION epsilon = 0) const;

    /**
     * @brief 从数组设置
     * @param array 数据数组
     * @param offset 偏移量
     */
    Matrix3 &fromArray(const std::vector<HIGH_PRECISION> &array, size_t offset = 0);

    /**
     * @brief 写入数组
     * @param array 目标数组
     * @param offset 偏移量
     */
    std::vector<HIGH_PRECISION> &toArray(std::vector<HIGH_PRECISION> &array, size_t offset = 0) const;

    /** @brief 克隆矩阵 @return 矩阵副本 */
    Matrix3 clone() const;

public:
    /** @brief 矩阵乘法运算符 @return 新矩阵 */
    Matrix3 operator*(const Matrix3 &m) const;

    /** @brief 赋值运算符 */
    Matrix3 &operator=(const Matrix3 &m);

    /** @brief 相等判断 @return 是否相等 */
    bool operator==(const Matrix3 &m) const;

    /** @brief 下标访问 @param index 元素索引 @return 元素值 */
    HIGH_PRECISION operator[](size_t index) const;

    Matrix3 &setFromMatrix4(const Matrix4 &matrix);
    Matrix3 &getNormalMatrix(const Matrix4 &matrix);
    Matrix3 &extractBasis(Vector3 &xAxis, Vector3 &yAxis, Vector3 &zAxis);
    std::vector<HIGH_PRECISION> toArray() const;

private:
    /** @brief 标记，用于区分 Matrix3 和 Matrix4 */
    bool m_isMatrix3 = false;

    /** @brief 矩阵元素数组（列主序） */
    HIGH_PRECISION m_elements[9] = {};
};

#endif
