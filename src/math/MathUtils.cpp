#include "math/MathUtils.h"
#include <vector>
#include <cstdint>

namespace MathUtils
{
    static const std::vector<std::string> _lut = {
        "00", "01", "02", "03", "04", "05", "06", "07", "08", "09", "0a", "0b", "0c", "0d", "0e", "0f",
        "10", "11", "12", "13", "14", "15", "16", "17", "18", "19", "1a", "1b", "1c", "1d", "1e", "1f",
        "20", "21", "22", "23", "24", "25", "26", "27", "28", "29", "2a", "2b", "2c", "2d", "2e", "2f",
        "30", "31", "32", "33", "34", "35", "36", "37", "38", "39", "3a", "3b", "3c", "3d", "3e", "3f",
        "40", "41", "42", "43", "44", "45", "46", "47", "48", "49", "4a", "4b", "4c", "4d", "4e", "4f",
        "50", "51", "52", "53", "54", "55", "56", "57", "58", "59", "5a", "5b", "5c", "5d", "5e", "5f",
        "60", "61", "62", "63", "64", "65", "66", "67", "68", "69", "6a", "6b", "6c", "6d", "6e", "6f",
        "70", "71", "72", "73", "74", "75", "76", "77", "78", "79", "7a", "7b", "7c", "7d", "7e", "7f",
        "80", "81", "82", "83", "84", "85", "86", "87", "88", "89", "8a", "8b", "8c", "8d", "8e", "8f",
        "90", "91", "92", "93", "94", "95", "96", "97", "98", "99", "9a", "9b", "9c", "9d", "9e", "9f",
        "a0", "a1", "a2", "a3", "a4", "a5", "a6", "a7", "a8", "a9", "aa", "ab", "ac", "ad", "ae", "af",
        "b0", "b1", "b2", "b3", "b4", "b5", "b6", "b7", "b8", "b9", "ba", "bb", "bc", "bd", "be", "bf",
        "c0", "c1", "c2", "c3", "c4", "c5", "c6", "c7", "c8", "c9", "ca", "cb", "cc", "cd", "ce", "cf",
        "d0", "d1", "d2", "d3", "d4", "d5", "d6", "d7", "d8", "d9", "da", "db", "dc", "dd", "de", "df",
        "e0", "e1", "e2", "e3", "e4", "e5", "e6", "e7", "e8", "e9", "ea", "eb", "ec", "ed", "ee", "ef",
        "f0", "f1", "f2", "f3", "f4", "f5", "f6", "f7", "f8", "f9", "fa", "fb", "fc", "fd", "fe", "ff"};

    int _seed = 1234567;

    static const float DEG2RAD = MATH_PI / 180.0f;
    static const float RAD2DEG = 180.0f / MATH_PI;

    // http://stackoverflow.com/questions/105034/how-to-create-a-guid-uuid-in-javascript/21963136#21963136
    std::string generateUUID()
    {
        const uint32_t d0 = DIST_UINT(RANDOM_ENGINE);
        const uint32_t d1 = DIST_UINT(RANDOM_ENGINE);
        const uint32_t d2 = DIST_UINT(RANDOM_ENGINE);
        const uint32_t d3 = DIST_UINT(RANDOM_ENGINE);

        std::string uuid =
            _lut[d0 & 0xFF] +
            _lut[(d0 >> 8) & 0xFF] +
            _lut[(d0 >> 16) & 0xFF] +
            _lut[(d0 >> 24) & 0xFF] + '-' +

            _lut[d1 & 0xFF] +
            _lut[(d1 >> 8) & 0xFF] + '-' +
            _lut[((d1 >> 16) & 0x0F) | 0x40] +
            _lut[(d1 >> 24) & 0xFF] + '-' +

            _lut[(d2 & 0x3F) | 0x80] +
            _lut[(d2 >> 8) & 0xFF] + '-' +

            _lut[(d2 >> 16) & 0xFF] +
            _lut[(d2 >> 24) & 0xFF] +
            _lut[d3 & 0xFF] +
            _lut[(d3 >> 8) & 0xFF] +
            _lut[(d3 >> 16) & 0xFF] +
            _lut[(d3 >> 24) & 0xFF];

        // .toLowerCase() here flattens concatenated strings to save heap memory space.
        std::transform(uuid.begin(), uuid.end(), uuid.begin(),
                       [](unsigned char c)
                       { return std::tolower(c); });

        return uuid;
    }

    HIGH_PRECISION clamp(HIGH_PRECISION value, HIGH_PRECISION min, HIGH_PRECISION max)
    {
        return std::max(min, std::min(max, value));
    }

    // compute euclidean modulo of m % n
    // https://en.wikipedia.org/wiki/Modulo_operation
    int euclideanModulo(int n, int m)
    {
        if (m == 0)
        {
            return std::numeric_limits<int>::quiet_NaN();
        }
        return ((n % m) + m) % m;
    }

    // Linear mapping from range <a1, a2> to range <b1, b2>
    HIGH_PRECISION mapLinear(HIGH_PRECISION x, HIGH_PRECISION a1, HIGH_PRECISION a2, HIGH_PRECISION b1, HIGH_PRECISION b2)
    {
        return b1 + (x - a1) * (b2 - b1) / (a2 - a1);
    }

    // https://www.gamedev.net/tutorials/programming/general-and-gameplay-programming/inverse-lerp-a-super-useful-yet-often-overlooked-function-r5230/
    HIGH_PRECISION inverseLerp(HIGH_PRECISION x, HIGH_PRECISION y, HIGH_PRECISION value)
    {
        if (x != y)
        {

            return (value - x) / (y - x);
        }
        else
        {

            return 0.0;
        }
    }

    // https://en.wikipedia.org/wiki/Linear_interpolation
    HIGH_PRECISION lerp(HIGH_PRECISION x, HIGH_PRECISION y, HIGH_PRECISION t)
    {
        return (1 - t) * x + t * y;
    }

    // http://www.rorydriscoll.com/2016/03/07/frame-rate-independent-damping-using-lerp/
    HIGH_PRECISION damp(HIGH_PRECISION x, HIGH_PRECISION y, HIGH_PRECISION lambda, HIGH_PRECISION dt)
    {
        return lerp(x, y, 1 - std::exp(-lambda * dt));
    }

    // https://www.desmos.com/calculator/vcsjnyz7x4
    HIGH_PRECISION pingpong(HIGH_PRECISION x, HIGH_PRECISION length)
    {
        return length - std::abs(euclideanModulo(x, length * 2) - length);
    }

    // http://en.wikipedia.org/wiki/Smoothstep
    HIGH_PRECISION smoothstep(HIGH_PRECISION x, HIGH_PRECISION min, HIGH_PRECISION max)
    {
        if (x <= min)
            return 0;
        if (x >= max)
            return 1;

        x = (x - min) / (max - min);

        return x * x * (3 - 2 * x);
    }

    HIGH_PRECISION smootherstep(HIGH_PRECISION x, HIGH_PRECISION min, HIGH_PRECISION max)
    {
        if (x <= min)
            return 0;
        if (x >= max)
            return 1;

        x = (x - min) / (max - min);

        return x * x * x * (x * (x * 6 - 15) + 10);
    }

    // Random integer from <low, high> interval
    int randInt(int low, int high)
    {
        return low + std::floor(DIST_DOUBLE(RANDOM_ENGINE) * (high - low + 1));
    }

    // Random float from <low, high> interval
    float randFloat(float low, float high)
    {
        return low + (float)DIST_DOUBLE(RANDOM_ENGINE) * (high - low);
    }

    // Random float from <-range/2, range/2> interval
    float randFloatSpread(float range)
    {
        return range * (0.5 - (float)DIST_DOUBLE(RANDOM_ENGINE));
    }

    // Deterministic pseudo-random float in the interval [ 0, 1 ]
    float seededRandom()
    {
        return (float)DIST_DOUBLE(RANDOM_ENGINE);
    }

    float degToRad(float degrees)
    {
        return degrees * DEG2RAD;
    }

    float radToDeg(float radians)
    {
        return radians * RAD2DEG;
    }

    bool isPowerOfTwo(int value)
    {
        return (value & (value - 1)) == 0 && value != 0;
    }

    HIGH_PRECISION ceilPowerOfTwo(HIGH_PRECISION value)
    {
        return std::pow(2, std::ceil(std::log(value) / M_LN2));
    }

    HIGH_PRECISION floorPowerOfTwo(HIGH_PRECISION value)
    {
        return std::pow(2, std::floor(std::log(value) / M_LN2));
    }

    // TODO:
    //  function setQuaternionFromProperEuler( q, a, b, c, order )

    // Matrix3 span to vector Matrix3
    std::vector<HIGH_PRECISION> convertSpanToVector(const std::span<const HIGH_PRECISION, 9> &src_span)
    {
        return std::vector<HIGH_PRECISION>(src_span.begin(), src_span.end());
    }

    // Matrix4 span to vector Matrix3
    std::vector<HIGH_PRECISION> convertSpanToVector(const std::span<const HIGH_PRECISION, 16> &src_span)
    {
        return std::vector<HIGH_PRECISION>(src_span.begin(), src_span.end());
    }
}