/**
 * @file   MathUtils.h
 * @brief  数学工具函数库
 * @author Abnerring
 * @date   2025-2026
 * @note   参考 three.js Math 工具集实现
 */

#ifndef MATH_UTILS_H
#define MATH_UTILS_H

#include "core/BasicType.h"
#include <algorithm>
#include <span>
#include <vector>
#include <ranges>
#include <optional>
#include <string>
#include <limits>
#include <type_traits>

/**
 * @brief 数学工具函数命名空间
 * @details 提供常用数学运算、随机数生成、插值等工具函数
 */
class Quaternion;

namespace MathUtils
{
    inline constexpr HIGH_PRECISION PI = MATH_PI;
    HIGH_PRECISION round(HIGH_PRECISION value);
    HIGH_PRECISION min(HIGH_PRECISION a, HIGH_PRECISION b);
    HIGH_PRECISION max(HIGH_PRECISION a, HIGH_PRECISION b);
    void setQuaternionFromProperEuler(Quaternion &q, HIGH_PRECISION a, HIGH_PRECISION b, HIGH_PRECISION c, const std::string &order);
    /**
     * @brief 生成 UUID
     * @return std::string 格式为 xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx 的 UUID 字符串
     */
    std::string generateUUID();

    /**
     * @brief 限制值在范围内
     * @param value 输入值
     * @param min 最小值
     * @param max 最大值
     * @return HIGH_PRECISION 限制后的值
     */
    HIGH_PRECISION clamp(HIGH_PRECISION value, HIGH_PRECISION min, HIGH_PRECISION max);

    /**
     * @brief 欧几里得取模
     * @param n 被除数
     * @param m 除数
     * @return int 结果满足 0 <= result < m
     */
    HIGH_PRECISION euclideanModulo(HIGH_PRECISION n, HIGH_PRECISION m);

    /**
     * @brief 线性映射
     * @param x 输入值
     * @param a1 输入范围起始
     * @param a2 输入范围结束
     * @param b1 输出范围起始
     * @param b2 输出范围结束
     * @return HIGH_PRECISION 映射后的值
     * @retval b1 当 x = a1
     * @retval b2 当 x = a2
     */
    HIGH_PRECISION mapLinear(HIGH_PRECISION x, HIGH_PRECISION a1, HIGH_PRECISION a2, HIGH_PRECISION b1, HIGH_PRECISION b2);

    /**
     * @brief 逆线性插值
     * @param x 查询值
     * @param a 范围起始
     * @param b 范围结束
     * @return HIGH_PRECISION 插值因子 alpha
     * @retval 0 当 x = a
     * @retval 1 当 x = b
     */
    HIGH_PRECISION inverseLerp(HIGH_PRECISION x, HIGH_PRECISION y, HIGH_PRECISION value);

    /**
     * @brief 线性插值
     * @param x 起始值
     * @param y 结束值
     * @param t 插值因子 [0, 1]
     * @return HIGH_PRECISION 插值结果
     */
    HIGH_PRECISION lerp(HIGH_PRECISION x, HIGH_PRECISION y, HIGH_PRECISION t);

    /**
     * @brief 阻尼插值
     * @param x 当前值
     * @param y 目标值
     * @param lambda 阻尼系数
     * @param dt 时间增量
     * @return HIGH_PRECISION 插值后的值
     */
    HIGH_PRECISION damp(HIGH_PRECISION x, HIGH_PRECISION y, HIGH_PRECISION lambda, HIGH_PRECISION dt);

    /**
     * @brief 乒乓往复
     * @param x 输入值
     * @param length 周期长度
     * @return HIGH_PRECISION 往复后的值 [0, length]
     */
    HIGH_PRECISION pingpong(HIGH_PRECISION x, HIGH_PRECISION length);

    /**
     * @brief Smoothstep 平滑插值
     * @param x 输入值
     * @param min 最小值
     * @param max 最大值
     * @return HIGH_PRECISION 平滑插值结果
     * @note 使用 3x^2 - 2x^3 曲线
     */
    HIGH_PRECISION smoothstep(HIGH_PRECISION x, HIGH_PRECISION min, HIGH_PRECISION max);

    /**
     * @brief Smootherstep 增强平滑插值
     * @param x 输入值
     * @param min 最小值
     * @param max 最大值
     * @return HIGH_PRECISION 平滑插值结果
     * @note 使用 6x^5 - 15x^4 + 10x^3 曲线
     */
    HIGH_PRECISION smootherstep(HIGH_PRECISION x, HIGH_PRECISION min, HIGH_PRECISION max);

    /**
     * @brief 生成随机整数
     * @param low 最小值（包含）
     * @param high 最大值（包含）
     * @return int 随机整数
     */
    int randInt(int low, int high);

    /**
     * @brief 生成随机浮点数
     * @param low 最小值
     * @param high 最大值
     * @return HIGH_PRECISION 随机浮点数
     */
    HIGH_PRECISION randFloat(HIGH_PRECISION low, HIGH_PRECISION high);

    /**
     * @brief 生成对称随机浮点数
     * @param range 范围 [-range, range]
     * @return HIGH_PRECISION 随机浮点数
     */
    HIGH_PRECISION randFloatSpread(HIGH_PRECISION range);

    /**
     * @brief 生成种子随机数 [0, 1)
     * @return HIGH_PRECISION 随机数
     */
    HIGH_PRECISION seededRandom(std::optional<uint32_t> seed = std::nullopt);

    /**
     * @brief 度数转弧度
     * @param degrees 角度
     * @return HIGH_PRECISION 弧度
     */
    HIGH_PRECISION degToRad(HIGH_PRECISION degrees);

    /**
     * @brief 弧度转度数
     * @param radians 弧度
     * @return HIGH_PRECISION 角度
     */
    HIGH_PRECISION radToDeg(HIGH_PRECISION radians);

    /**
     * @brief 判断是否为 2 的幂
     * @param value 待检测值
     * @return bool 是否为 2 的幂
     */
    bool isPowerOfTwo(HIGH_PRECISION value);

    /**
     * @brief 向上取整到 2 的幂
     * @param value 输入值
     * @return HIGH_PRECISION 最近的 2^n >= value
     */
    HIGH_PRECISION ceilPowerOfTwo(HIGH_PRECISION value);

    /**
     * @brief 向下取整到 2 的幂
     * @param value 输入值
     * @return HIGH_PRECISION 最近的 2^n <= value
     */
    HIGH_PRECISION floorPowerOfTwo(HIGH_PRECISION value);

    /**
     * @brief 将固定大小 span 转换为向量
     * @param src_span 9 元素 span
     * @return std::vector<HIGH_PRECISION> 转换后的向量
     */
    std::vector<HIGH_PRECISION> convertSpanToVector(const std::span<const HIGH_PRECISION, 9> &src_span);

    /**
     * @brief 将固定大小 span 转换为向量
     * @param src_span 16 元素 span
     * @return std::vector<HIGH_PRECISION> 转换后的向量
     */
    std::vector<HIGH_PRECISION> convertSpanToVector(const std::span<const HIGH_PRECISION, 16> &src_span);

    /**
     * @brief 反归一化值到目标类型
     * @tparam T 目标类型（uint8/16/32, int8/16/32, HIGH_PRECISION/double）
     * @param value 归一化值
     * @return HIGH_PRECISION 反归一化后的值
     * @retval [0, 1] 无符号类型
     * @retval [-1, 1] 有符号类型
     */
    template <typename T>
    HIGH_PRECISION denormalize(HIGH_PRECISION value)
    {
        // [0.0, 1.0]
        if constexpr (std::is_floating_point_v<T>)
        {
            return value;
        }
        else if constexpr (std::is_unsigned_v<T>)
        {
            if constexpr (std::is_same_v<T, uint32_t>)
            {
                return value / 4294967295.0;
            }
            else if constexpr (std::is_same_v<T, uint16_t>)
            {
                return value / 65535.0;
            }
            else if constexpr (std::is_same_v<T, uint8_t>)
            {
                return value / 255.0;
            }
        }
        // [-1.0, 1.0]
        else if constexpr (std::is_signed_v<T>)
        {
            if constexpr (std::is_same_v<T, int32_t>)
            {
                return std::max(value / 2147483647.0, (HIGH_PRECISION)-1.0);
            }
            else if constexpr (std::is_same_v<T, int16_t>)
            {
                return std::max(value / 32767.0, (HIGH_PRECISION)-1.0);
            }
            else if constexpr (std::is_same_v<T, int8_t>)
            {
                return std::max(value / 127.0, (HIGH_PRECISION)-1.0);
            }
        }
        //
        else if constexpr (std::is_floating_point_v<T>)
        {
            return value;
        }
        //
        else
        {
            static_assert(!std::is_same_v<T, T>, "Unsupported component type");
        }
    }

    /**
     * @brief 归一化值到目标类型
     * @tparam T 目标类型
     * @param value 输入值
     * @return T 归一化后的值
     * @retval [0, MAX] 无符号类型
     * @retval [-MAX, MAX] 有符号类型
     */
    template <typename T>
    T normalize(HIGH_PRECISION value)
    {
        if constexpr (std::is_floating_point_v<T>)
        {
            return static_cast<T>(value);
        }
        else if constexpr (std::is_unsigned_v<T>)
        {
            // [0.0, 1.0]
            value = std::clamp(value, (HIGH_PRECISION)0.0, (HIGH_PRECISION)1.0);
            constexpr double max = static_cast<double>(std::numeric_limits<T>::max());
            return static_cast<T>(MathUtils::round(value * max));
        }
        else if constexpr (std::is_signed_v<T>)
        {
            // [-1.0, 1.0]
            value = std::clamp(value, (HIGH_PRECISION)-1.0, (HIGH_PRECISION)1.0);
            constexpr double max = static_cast<double>(std::numeric_limits<T>::max());
            return static_cast<T>(MathUtils::round(value * max));
        }
        else
        {
            //
            static_assert(!std::is_same_v<T, T>, "Unsupported target type");
        }
    }
}

#endif
