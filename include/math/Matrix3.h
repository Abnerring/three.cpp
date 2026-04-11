/**
 * @file   Matrix3.h
 * @brief  3x3 矩阵类
 * @author Abnerring
 * @date   2025-2026
 * @note   参考 three.js Matrix3 实现，用于 2D 变换和法线变换
 */

#ifndef MATRIX3_H
#define MATRIX3_H

#include "common/BasicType.h"
#include <span>

class Vector2;

/**
 * @class Matrix3
 * @brief 3x3 矩阵，用于 2D 变换、UV 变换、法线变换等
 * @details 存储为列主序数组，共 9 个元素
 * @see Matrix4 Vector2
 */
class Matrix3
{
public:
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

    /**
     * @brief 设置矩阵元素
     * @param n11-n33 矩阵元素
     */
    void set(
        HIGH_PRECISION n11, HIGH_PRECISION n12, HIGH_PRECISION n13, 
        HIGH_PRECISION n21, HIGH_PRECISION n22, HIGH_PRECISION n23, 
        HIGH_PRECISION n31, HIGH_PRECISION n32, HIGH_PRECISION n33);

    /** @brief 设置为单位矩阵 */
    void identity();

    /**
     * @brief 复制矩阵
     * @param m 源矩阵
     */
    void copy(const Matrix3 &m);

    /**
     * @brief 右乘矩阵
     * @param m 右乘矩阵 this = this * m
     */
    void multiply(const Matrix3 &m);

    /**
     * @brief 左乘矩阵
     * @param m 左乘矩阵 this = m * this
     */
    void premultiply(const Matrix3 &m);

    /**
     * @brief 矩阵乘法
     * @param a 左矩阵
     * @param b 右矩阵 this = a * b
     */
    void multiplyMatrices(const Matrix3 &a, const Matrix3 &b);

    /**
     * @brief 所有元素乘以标量
     * @param s 标量
     */
    void multiplyScalar(HIGH_PRECISION s);

    /**
     * @brief 计算行列式
     * @return HIGH_PRECISION 行列式值
     */
    HIGH_PRECISION determinant();

    /** @brief 求逆矩阵 @warning 行列式为 0 时结果无效 */
    void invert();

    /** @brief 转置矩阵 */
    void transpose();

    /**
     * @brief 转置到数组
     * @param r 目标数组
     */
    void transposeIntoArray(std::array<HIGH_PRECISION, 9> &r);

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
    void setUvTransform(HIGH_PRECISION tx, HIGH_PRECISION ty, HIGH_PRECISION sx, HIGH_PRECISION sy, float rotation, HIGH_PRECISION cx, HIGH_PRECISION cy);

    /**
     * @brief 缩放变换
     * @param sx X 轴缩放
     * @param sy Y 轴缩放
     */
    void scale(HIGH_PRECISION sx, HIGH_PRECISION sy);

    /**
     * @brief 旋转变换
     * @param theta 旋转角度（弧度）
     */
    void rotate(float theta);

    /**
     * @brief 平移变换
     * @param tx X 轴平移
     * @param ty Y 轴平移
     */
    void translate(HIGH_PRECISION tx, HIGH_PRECISION ty);

    /**
     * @brief 设置为平移矩阵
     * @param v 平移向量
     */
    void makeTranslation(const Vector2 &v);

    /**
     * @brief 设置为平移矩阵
     * @param x X 轴平移
     * @param y Y 轴平移
     */
    void makeTranslation(HIGH_PRECISION x, HIGH_PRECISION y);

    /**
     * @brief 设置为旋转矩阵
     * @param theta 旋转角度（弧度）
     */
    void makeRotation(float theta);

    /**
     * @brief 设置为缩放矩阵
     * @param x X 轴缩放
     * @param y Y 轴缩放
     */
    void makeScale(HIGH_PRECISION x, HIGH_PRECISION y);

    /**
     * @brief 判断是否相等
     * @param matrix 比较矩阵
     * @param epsilon 容差
     * @return bool 是否相等
     */
    bool equals(const Matrix3 &matrix, float epsilon = 1e-6) const;

    /**
     * @brief 从数组设置
     * @param array 数据数组
     * @param offset 偏移量
     */
    void fromArray(const std::vector<HIGH_PRECISION> &array, size_t offset = 0);

    /**
     * @brief 写入数组
     * @param array 目标数组
     * @param offset 偏移量
     */
    void toArray(std::vector<HIGH_PRECISION> &array, size_t offset = 0);

    /** @brief 克隆矩阵 @return 矩阵副本 */
    Matrix3 clone();

public:
    /** @brief 矩阵乘法运算符 @return 新矩阵 */
    Matrix3 operator*(const Matrix3 &m);

    /** @brief 赋值运算符 */
    void operator=(const Matrix3 &m);

    /** @brief 相等判断 @return 是否相等 */
    bool operator==(const Matrix3 &m) const;

    /** @brief 下标访问 @param index 元素索引 @return 元素值 */
    HIGH_PRECISION operator[](size_t index) const;

private:
    /** @brief 标记，用于区分 Matrix3 和 Matrix4 */
    bool m_isMatrix3 = false;

    /** @brief 矩阵元素数组（列主序） */
    HIGH_PRECISION m_elements[9] = {};
};

#endif
