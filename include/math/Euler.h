/**
 * @file   Euler.h
 * @brief  欧拉角类
 * @author Abnerring
 * @date   2025-2026
 * @note   参考 three.js Euler 实现，用于表示 3D 旋转
 */

#ifndef EULER_H
#define EULER_H

#include "common/BasicType.h"
#include <vector>
#include <string>
#include <functional>
#include <optional>
#include <variant>

class Vector3;
class Quaternion;
class Matrix3;
class Matrix4;

/**
 * @brief 欧拉角旋转顺序枚举
 * @details 定义三个旋转轴的应用顺序，共 6 种组合
 */
enum class EulerOrder
{
    XYZ,    ///< 先 X 再 Y 最后 Z
    XZY,    ///< 先 X 再 Z 最后 Y
    YXZ,    ///< 先 Y 再 X 最后 Z
    YZX,    ///< 先 Y 再 Z 最后 X
    ZXY,    ///< 先 Z 再 X 最后 Y
    ZYX     ///< 先 Z 再 Y 最后 X
};

/**
 * @class Euler
 * @brief 欧拉角，用于表示 3D 旋转
 * @details 由三个角度 (x, y, z) 组成，按指定顺序依次旋转
 *        角度单位为弧度，与 three.js 保持一致
 * @see Quaternion Matrix4 Vector3
 *
 * @example
 * @code
 * Euler euler(MathUtils::PI / 4, 0, 0, EulerOrder::XYZ);
 * Quaternion q;
 * euler.toQuaternion(q);
 * @endcode
 */
class Euler
{
public:
    static constexpr bool isEuler = true;
    /**
     * @brief 构造欧拉角（弧度）
     * @param x X 轴旋转角度，默认 0
     * @param y Y 轴旋转角度，默认 0
     * @param z Z 轴旋转角度，默认 0
     * @param order 旋转顺序，默认 EulerOrder::XYZ
     */
    Euler(HIGH_PRECISION x = 0.0, HIGH_PRECISION y = 0.0, HIGH_PRECISION z = 0.0, EulerOrder order = EulerOrder::XYZ);

    Euler(const Euler &euler);
    /** @brief 析构函数 */
    ~Euler();

    /** @brief 获取 X 轴角度 @return 弧度值 */
    HIGH_PRECISION x() const;

    /** @brief 获取 Y 轴角度 @return 弧度值 */
    HIGH_PRECISION y() const;

    /** @brief 获取 Z 轴角度 @return 弧度值 */
    HIGH_PRECISION z() const;

    /** @brief 获取旋转顺序 @return EulerOrder */
    EulerOrder order() const;

    /**
     * @brief 设置角度和旋转顺序
     * @param x X 轴角度（弧度）
     * @param y Y 轴角度（弧度）
     * @param z Z 轴角度（弧度）
     * @param order 旋转顺序，默认 XYZ
     * @return Euler& 自身引用
     */
    Euler &set(HIGH_PRECISION x, HIGH_PRECISION y, HIGH_PRECISION z, std::optional<EulerOrder> order = std::nullopt);

    /** @brief 设置 X 轴角度 @param x 弧度值 @return Euler& 自身引用 */
    Euler &setX(HIGH_PRECISION x);

    /** @brief 设置 Y 轴角度 @param y 弧度值 @return Euler& 自身引用 */
    Euler &setY(HIGH_PRECISION y);

    /** @brief 设置 Z 轴角度 @param z 弧度值 @return Euler& 自身引用 */
    Euler &setZ(HIGH_PRECISION z);

    /** @brief 设置旋转顺序 @param order 旋转顺序 @return Euler& 自身引用 */
    Euler &setOrder(EulerOrder order);

    /** @brief 克隆欧拉角 @return 欧拉角副本 */
    Euler clone() const;

    /**
     * @brief 复制欧拉角
     * @param euler 源欧拉角
     */
    Euler &copy(const Euler &euler);

    /**
     * @brief 从欧拉角设置
     * @param euler 源欧拉角
     * @return Euler& 自身引用
     */
    Euler &setFromEuler(const Euler &euler);

    /**
     * @brief 从四元数设置
     * @param q 四元数
     * @param order 旋转顺序，默认 XYZ
     * @return Euler& 自身引用
     */
    Euler &setFromQuaternion(const Quaternion &q, std::optional<EulerOrder> order = std::nullopt, bool update = true);

    /**
     * @brief 从旋转矩阵设置
     * @param m 旋转矩阵
     * @param order 旋转顺序，默认 XYZ
     * @return Euler& 自身引用
     */
    Euler &setFromRotationMatrix(const Matrix4 &m, std::optional<EulerOrder> order = std::nullopt, bool update = true);

    /**
     * @brief 判断是否相等
     * @param euler 比较欧拉角
     * @param epsilon 容差
     * @return bool 是否相等
     */
    bool equals(const Euler &euler, HIGH_PRECISION epsilon = 0) const;

    /**
     * @brief 转换为四元数
     * @param q 输出四元数
     */
    void toQuaternion(Quaternion &q) const;

    /**
     * @brief 转换为旋转矩阵
     * @param m 输出 4x4 矩阵
     */
    void toRotationMatrix(Matrix4 &m) const;

    /**
     * @brief 写入数组
     * @param array 目标数组
     * @param offset 偏移量
     * @return std::vector<HIGH_PRECISION>& 数组引用
     */
    std::vector<HIGH_PRECISION> &toArray(std::vector<HIGH_PRECISION> &array, size_t offset = 0) const;

    /**
     * @brief 从数组设置
     * @param array 数据数组
     * @param offset 偏移量
     * @return Euler& 自身引用
     */
    Euler &fromArray(const std::vector<HIGH_PRECISION> &array, size_t offset = 0);

    /**
     * @brief 获取旋转顺序字符串
     * @return std::string 旋转顺序，如 "XYZ"
     */
    std::string getOrderString() const;

public:
    /** @brief 赋值运算符 */
    Euler &operator=(const Euler &euler);

    /** @brief 相等判断 @return 是否相等 */
    bool operator==(const Euler &euler) const;

    Euler &_onChange(std::function<void()> callback);
    Euler &setFromVector3(const Vector3 &v, std::optional<EulerOrder> order = std::nullopt);
    Euler &reorder(EulerOrder order);
    using ArrayValue = std::variant<HIGH_PRECISION, std::string>;
    std::vector<ArrayValue> toArray() const;
    std::vector<ArrayValue> &toArray(std::vector<ArrayValue> &array, size_t offset = 0) const;
    Euler &fromArray(const std::vector<ArrayValue> &array, size_t offset = 0);

private:
    std::function<void()> m_onChangeCallback = [] {};
    /** @brief X 轴角度（弧度）*/
    HIGH_PRECISION m_x;

    /** @brief Y 轴角度（弧度）*/
    HIGH_PRECISION m_y;

    /** @brief Z 轴角度（弧度）*/
    HIGH_PRECISION m_z;

    /** @brief 旋转顺序 */
    EulerOrder m_order;
};

#endif
