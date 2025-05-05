#ifndef MATH_UTILS_H
#define MATH_UTILS_H

#include "common/BasicType.h"
#include <algorithm>
#include <span>
#include <vector>
#include <ranges>

namespace MathUtils
{
    std::string generateUUID();
    HIGH_PRECISION clamp(HIGH_PRECISION value, HIGH_PRECISION min, HIGH_PRECISION max);
    int euclideanModulo(int n, int m);
    HIGH_PRECISION mapLinear(HIGH_PRECISION x, HIGH_PRECISION a1, HIGH_PRECISION a2, HIGH_PRECISION b1, HIGH_PRECISION b2);
    HIGH_PRECISION inverseLerp(HIGH_PRECISION x, HIGH_PRECISION a, HIGH_PRECISION b);
    HIGH_PRECISION lerp(HIGH_PRECISION x, HIGH_PRECISION y, HIGH_PRECISION t);
    HIGH_PRECISION damp(HIGH_PRECISION x, HIGH_PRECISION y, HIGH_PRECISION lambda, HIGH_PRECISION dt);
    HIGH_PRECISION pingpong(HIGH_PRECISION x, HIGH_PRECISION length);
    HIGH_PRECISION smoothstep(HIGH_PRECISION x, HIGH_PRECISION min, HIGH_PRECISION max);
    HIGH_PRECISION smootherstep(HIGH_PRECISION x, HIGH_PRECISION min, HIGH_PRECISION max);
    int randInt(int low, int high);
    float randFloat(float low, float high);
    float randFloatSpread(float range);
    float seededRandom();
    float degToRad(float degrees);
    float radToDeg(float radians);
    bool isPowerOfTwo(int value);
    HIGH_PRECISION ceilPowerOfTwo(HIGH_PRECISION value);
    HIGH_PRECISION floorPowerOfTwo(HIGH_PRECISION value);
    //TODO:
    // function setQuaternionFromProperEuler( q, a, b, c, order )
    std::vector<HIGH_PRECISION> convertSpanToVector(const std::span<const HIGH_PRECISION, 9> &src_span);
    std::vector<HIGH_PRECISION> convertSpanToVector(const std::span<const HIGH_PRECISION, 16> &src_span);
    
    template <typename T>
    HIGH_PRECISION denormalize(HIGH_PRECISION value)
    {
        // [0.0, 1.0]
        if constexpr (std::is_unsigned_v<T>)
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
            return static_cast<T>(std::round(value * max));
        }
        else if constexpr (std::is_signed_v<T>)
        {
            // [-1.0, 1.0]
            value = std::clamp(value, (HIGH_PRECISION)-1.0, (HIGH_PRECISION)1.0);
            constexpr double max = static_cast<double>(std::numeric_limits<T>::max());
            return static_cast<T>(std::round(value * max));
        }
        else
        {
            //
            static_assert(!std::is_same_v<T, T>, "Unsupported target type");
        }
    }
}

#endif