/**
 * @file   Matrix4.h
 * @brief  4x4 矩阵类
 * @author Abnerring
 * @date   2025-2026
 * @note   参考 three.js Matrix4 实现，用于 3D 变换
 */

#ifndef MATRIX4_H
#define MATRIX4_H

#include "core/BasicType.h"
#include "core/JSValue.h"
#include <span>
#include <vector>
#include <memory>

class Vector3;
class Matrix3;
class Object3D;
JSValue nativeMatrix4Constructor();

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
    static constexpr bool isMatrix4 = true;
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

    Matrix4(const Matrix4& source);
    Matrix4& operator=(const Matrix4& source);
    // Explicit reference assignment for Object3D helpers. Ordinary C++ copies
    // remain independent value copies, including when their source is aliased.
    Matrix4& aliasTo(std::shared_ptr<Matrix4> target,std::weak_ptr<Object3D> objectOwner = {});
    const std::shared_ptr<Matrix4>& aliasTarget() const { return alias_; }
    const std::weak_ptr<Object3D>& aliasObjectOwner() const { return aliasObjectOwner_; }

    /**
     * @brief 获取矩阵元素
     * @return std::span<const HIGH_PRECISION, 16> 16 元素只读视图
     */
    std::span<const HIGH_PRECISION, 16> elements() const;
    std::span<HIGH_PRECISION, 16> elements();

    /**
     * @brief 设置矩阵元素
     * @param n11-n44 矩阵 16 个元素
     */
    Matrix4 &set(
        HIGH_PRECISION n11, HIGH_PRECISION n12, HIGH_PRECISION n13, HIGH_PRECISION n14,
        HIGH_PRECISION n21, HIGH_PRECISION n22, HIGH_PRECISION n23, HIGH_PRECISION n24,
        HIGH_PRECISION n31, HIGH_PRECISION n32, HIGH_PRECISION n33, HIGH_PRECISION n34,
        HIGH_PRECISION n41, HIGH_PRECISION n42, HIGH_PRECISION n43, HIGH_PRECISION n44);

    /** @brief 设置为单位矩阵 */
    Matrix4 &identity();

    /** @brief 克隆矩阵 @return 矩阵副本 */
    Matrix4 clone() const;

    /**
     * @brief 复制矩阵
     * @param m 源矩阵
     */
    Matrix4 &copy(const Matrix4 &m);

    /**
     * @brief 复制位置分量（仅影响最后一列）
     * @param m 源矩阵
     */
    Matrix4 &copyPosition(const Matrix4 &m);

    /**
     * @brief 从 3x3 矩阵设置
     * @param m 3x3 矩阵
     */
    Matrix4 &setFromMatrix3(const Matrix3 &m);

    /**
     * @brief 提取基向量
     * @param xAxis X 轴输出
     * @param yAxis Y 轴输出
     * @param zAxis Z 轴输出
     */
    Matrix4 &extractBasis(Vector3 &xAxis, Vector3 &yAxis, Vector3 &zAxis);

    /**
     * @brief 从基向量设置
     * @param xAxis X 轴
     * @param yAxis Y 轴
     * @param zAxis Z 轴
     */
    Matrix4 &makeBasis(const Vector3 &xAxis, const Vector3 &yAxis, const Vector3 &zAxis);

    /**
     * @brief 从矩阵提取旋转部分
     * @param m 源矩阵
     */
    Matrix4 &extractRotation(const Matrix4 &m);

    /**
     * @brief 从欧拉角设置旋转
     * @param euler 欧拉角
     */
    Matrix4 &makeRotationFromEuler(const class Euler &euler);

    /**
     * @brief 从四元数设置旋转
     * @param q 四元数
     */
    Matrix4 &makeRotationFromQuaternion(const class Quaternion &q);

    /**
     * @brief 设置为 LookAt 视图矩阵
     * @param eye 相机位置
     * @param target 观察目标
     * @param up 上方向向量
     */
    Matrix4 &lookAt(const Vector3 &eye, const Vector3 &target, const Vector3 &up);

    /**
     * @brief 右乘矩阵 this = this * m
     * @param m 右乘矩阵
     */
    Matrix4 &multiply(const Matrix4 &m);

    /**
     * @brief 左乘矩阵 this = m * this
     * @param m 左乘矩阵
     */
    Matrix4 &premultiply(const Matrix4 &m);

    /**
     * @brief 矩阵乘法 this = a * b
     * @param a 左矩阵
     * @param b 右矩阵
     */
    Matrix4 &multiplyMatrices(const Matrix4 &a, const Matrix4 &b);

    /**
     * @brief 所有元素乘以标量
     * @param s 标量值
     */
    Matrix4 &multiplyScalar(HIGH_PRECISION s);

    /**
     * @brief 计算行列式
     * @return HIGH_PRECISION 行列式值
     */
    HIGH_PRECISION determinant() const;
    HIGH_PRECISION determinantAffine() const;

    /** @brief 转置矩阵 */
    Matrix4 &transpose();

    /**
     * @brief 设置位置（仅影响最后一列）
     * @param x X 坐标
     * @param y Y 坐标
     * @param z Z 坐标
     */
    Matrix4 &setPosition(HIGH_PRECISION x, HIGH_PRECISION y, HIGH_PRECISION z);

    /**
     * @brief 设置位置
     * @param v 位置向量
     */
    Matrix4 &setPosition(const Vector3 &v);

    /** @brief 求逆矩阵 @warning 行列式为 0 时结果无效 */
    Matrix4 &invert();

    /**
     * @brief 缩放变换
     * @param v 缩放向量
     */
    Matrix4 &scale(const Vector3 &v);

    /**
     * @brief 获取最大缩放因子
     * @return HIGH_PRECISION 最大缩放值
     */
    HIGH_PRECISION getMaxScaleOnAxis() const;

    /**
     * @brief 设置为平移矩阵
     * @param x X 轴平移
     * @param y Y 轴平移
     * @param z Z 轴平移
     */
    Matrix4 &makeTranslation(HIGH_PRECISION x, HIGH_PRECISION y, HIGH_PRECISION z);

    /**
     * @brief 设置为平移矩阵
     * @param v 平移向量
     */
    Matrix4 &makeTranslation(const Vector3 &v);

    /** @brief 设置为绕 X 轴旋转矩阵 @param theta 角度（弧度） */
    Matrix4 &makeRotationX(HIGH_PRECISION theta);

    /** @brief 设置为绕 Y 轴旋转矩阵 @param theta 角度（弧度） */
    Matrix4 &makeRotationY(HIGH_PRECISION theta);

    /** @brief 设置为绕 Z 轴旋转矩阵 @param theta 角度（弧度） */
    Matrix4 &makeRotationZ(HIGH_PRECISION theta);

    /**
     * @brief 设置为绕任意轴旋转矩阵
     * @param axis 旋转轴（自动归一化）
     * @param angle 角度（弧度）
     */
    Matrix4 &makeRotationAxis(const Vector3 &axis, HIGH_PRECISION angle);

    /**
     * @brief 设置为缩放矩阵
     * @param x X 轴缩放
     * @param y Y 轴缩放
     * @param z Z 轴缩放
     */
    Matrix4 &makeScale(HIGH_PRECISION x, HIGH_PRECISION y, HIGH_PRECISION z);

    /**
     * @brief 设置为错切矩阵
     * @param xy-xzy 错切参数
     */
    Matrix4 &makeShear(HIGH_PRECISION xy, HIGH_PRECISION xz, HIGH_PRECISION yx, HIGH_PRECISION yz, HIGH_PRECISION zx, HIGH_PRECISION zy);

    /**
     * @brief 从位置、旋转、缩放组合矩阵
     * @param position 位置
     * @param quaternion 旋转（四元数）
     * @param scale 缩放
     */
    Matrix4 &compose(const Vector3 &position, const class Quaternion &quaternion, const Vector3 &scale);

    Matrix4 &decompose(Vector3 &position, class Quaternion &quaternion, Vector3 &scale);
    Matrix4 &makePerspective(HIGH_PRECISION left, HIGH_PRECISION right, HIGH_PRECISION top, HIGH_PRECISION bottom, HIGH_PRECISION near, HIGH_PRECISION far, int coordinateSystem = 2000, bool reversedDepth = false);
    Matrix4 &makeOrthographic(HIGH_PRECISION left, HIGH_PRECISION right, HIGH_PRECISION top, HIGH_PRECISION bottom, HIGH_PRECISION near, HIGH_PRECISION far, int coordinateSystem = 2000, bool reversedDepth = false);
    bool equals(const Matrix4 &matrix) const;
    Matrix4 &fromArray(const std::vector<HIGH_PRECISION> &array, size_t offset = 0);
    std::vector<HIGH_PRECISION> &toArray(std::vector<HIGH_PRECISION> &array, size_t offset = 0) const;
    std::vector<HIGH_PRECISION> toArray() const;
    HIGH_PRECISION operator[](size_t index) const;

private:
    /** @brief 标记，用于区分 Matrix3 和 Matrix4 */
    bool m_isMatrix4 = false;

    /** @brief 矩阵元素数组（列主序） */
    HIGH_PRECISION m_elements[16] = {};
    std::shared_ptr<Matrix4> alias_;
    std::weak_ptr<Object3D> aliasObjectOwner_;
};

#endif
