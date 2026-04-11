/**
 * @file   BasicType.h
 * @brief  基础类型定义
 * @author Abnerring
 * @date   2025-2026
 * @note   定义高精度类型和数学常量，提供跨平台支持
 */

#ifndef BASIC_TYPE_H
#define BASIC_TYPE_H

#include <numbers>
#include <random>

/**
 * @brief 高精度浮点类型
 * @details x86/x64 架构使用 long double，其他使用 double
 */
#if defined(__x86_64__) || defined(__i386__)
    #define HIGH_PRECISION long double
#else
    #define HIGH_PRECISION double
#endif

/**
 * @brief 圆周率常量
 */
#define MATH_PI std::numbers::pi_v<HIGH_PRECISION>

/**
 * @brief 数学常量命名空间
 * @details 提供随机数生成器和常用分布
 */
namespace MathConstants
{
    /**
     * @brief 获取随机设备
     * @return std::random_device& 随机设备引用
     */
    inline std::random_device &getRandomDevice()
    {
        static std::random_device rd;
        return rd;
    }

    /**
     * @brief 获取随机数引擎
     * @return std::mt19937& Mersenne Twister 引擎引用
     * @note 使用随机设备种子
     */
    inline std::mt19937 &getRandomEngine()
    {
        static std::mt19937 engine(getRandomDevice()());
        return engine;
    }

    /**
     * @brief 获取无符号整数均匀分布 [0, 0xFFFFFFFF]
     * @return std::uniform_int_distribution<uint32_t>& 分布引用
     */
    inline std::uniform_int_distribution<uint32_t> &getDistUint()
    {
        static std::uniform_int_distribution<uint32_t> dist(0, 0xFFFFFFFF);
        return dist;
    }

    /**
     * @brief 获取浮点数均匀分布 [0.0, 1.0)
     * @return std::uniform_real_distribution<HIGH_PRECISION>& 分布引用
     */
    inline std::uniform_real_distribution<HIGH_PRECISION> &getDistDouble()
    {
        static std::uniform_real_distribution<HIGH_PRECISION> dist(0.0, 1.0);
        return dist;
    }
}

//! @brief 随机引擎便捷宏
#define RANDOM_ENGINE MathConstants::getRandomEngine()
//! @brief 无符号整数分布便捷宏
#define DIST_UINT MathConstants::getDistUint()
//! @brief 浮点数分布便捷宏
#define DIST_DOUBLE MathConstants::getDistDouble()

#endif
