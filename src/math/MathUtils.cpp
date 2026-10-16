#include "math/MathUtils.h"
#include "math/JSNumberMath.h"
#include <vector>
#include <cstdint>
#include "math/Quaternion.h"
#include "math/JSNumberMath.h"

namespace MathUtils
{
    static constexpr const char* _lut[] = {
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

    static const HIGH_PRECISION DEG2RAD = MATH_PI / 180.0;
    static const HIGH_PRECISION RAD2DEG = 180.0 / MATH_PI;
    static const HIGH_PRECISION LN2 = 0.6931471805599453;  // std::log(2.0)

    // http://stackoverflow.com/questions/105034/how-to-create-a-guid-uuid-in-javascript/21963136#21963136
    std::string generateUUID()
    {
        const uint32_t d0 = DIST_UINT(RANDOM_ENGINE);
        const uint32_t d1 = DIST_UINT(RANDOM_ENGINE);
        const uint32_t d2 = DIST_UINT(RANDOM_ENGINE);
        const uint32_t d3 = DIST_UINT(RANDOM_ENGINE);

        std::string uuid =
            std::string(_lut[d0 & 0xFF]) +
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
        return MathUtils::max(min, MathUtils::min(max, value));
    }

    // compute euclidean modulo of m % n
    // https://en.wikipedia.org/wiki/Modulo_operation
    HIGH_PRECISION euclideanModulo(HIGH_PRECISION n, HIGH_PRECISION m)
    {
        return std::fmod(std::fmod(n, m) + m, m);
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

    // Random HIGH_PRECISION from <low, high> interval
    HIGH_PRECISION randFloat(HIGH_PRECISION low, HIGH_PRECISION high)
    {
        return low + (HIGH_PRECISION)DIST_DOUBLE(RANDOM_ENGINE) * (high - low);
    }

    // Random HIGH_PRECISION from <-range/2, range/2> interval
    HIGH_PRECISION randFloatSpread(HIGH_PRECISION range)
    {
        return range * (0.5 - (HIGH_PRECISION)DIST_DOUBLE(RANDOM_ENGINE));
    }

    // Deterministic pseudo-random HIGH_PRECISION in the interval [ 0, 1 ]
    HIGH_PRECISION seededRandom(std::optional<uint32_t> seed)
    {
        static uint32_t state = 1234567;
        if (seed) state = *seed;
        uint32_t t = (state += 0x6D2B79F5u);
        t = (t ^ (t >> 15)) * (t | 1u);
        t ^= t + (t ^ (t >> 7)) * (t | 61u);
        return static_cast<HIGH_PRECISION>(t ^ (t >> 14)) / 4294967296.0;
    }

    HIGH_PRECISION degToRad(HIGH_PRECISION degrees)
    {
        return degrees * DEG2RAD;
    }

    HIGH_PRECISION radToDeg(HIGH_PRECISION radians)
    {
        return radians * RAD2DEG;
    }

    bool isPowerOfTwo(HIGH_PRECISION value)
    {
        return std::isfinite(value) && value > 0 && std::floor(value) == value && std::pow(2.0, MathUtils::round(std::log2(value))) == value;
    }

    HIGH_PRECISION ceilPowerOfTwo(HIGH_PRECISION value)
    {
        return std::pow(2, std::ceil(std::log(value) / LN2));
    }

    HIGH_PRECISION floorPowerOfTwo(HIGH_PRECISION value)
    {
        return std::pow(2, std::floor(std::log(value) / LN2));
    }

    HIGH_PRECISION round(HIGH_PRECISION value)
    {
        if (!std::isfinite(value) || value == 0) return value;
        if (value >= -0.5 && value < 0) return -0.0;
        const auto lower = std::floor(value);
        return (value - lower < 0.5) ? lower : lower + 1;
    }


    HIGH_PRECISION min(HIGH_PRECISION a, HIGH_PRECISION b)
    {
        if (std::isnan(a) || std::isnan(b)) return std::numeric_limits<HIGH_PRECISION>::quiet_NaN();
        if (a == 0 && b == 0) return std::signbit(a) || std::signbit(b) ? -0.0 : 0.0;
        return a < b ? a : b;
    }
    HIGH_PRECISION max(HIGH_PRECISION a, HIGH_PRECISION b)
    {
        if (std::isnan(a) || std::isnan(b)) return std::numeric_limits<HIGH_PRECISION>::quiet_NaN();
        if (a == 0 && b == 0) return !std::signbit(a) || !std::signbit(b) ? 0.0 : -0.0;
        return a > b ? a : b;
    }


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

    void setQuaternionFromProperEuler(Quaternion &q, HIGH_PRECISION a, HIGH_PRECISION b, HIGH_PRECISION c, const std::string &order)
    {





	const auto c2 = JSNumberMath::cos( b / 2 );
	const auto s2 = JSNumberMath::sin( b / 2 );

	const auto c13 = JSNumberMath::cos( ( a + c ) / 2 );
	const auto s13 = JSNumberMath::sin( ( a + c ) / 2 );

	const auto c1_3 = JSNumberMath::cos( ( a - c ) / 2 );
	const auto s1_3 = JSNumberMath::sin( ( a - c ) / 2 );

	const auto c3_1 = JSNumberMath::cos( ( c - a ) / 2 );
	const auto s3_1 = JSNumberMath::sin( ( c - a ) / 2 );

	        if (order == "XYX") { q.set( c2 * s13, s2 * c1_3, s2 * s1_3, c2 * c13 ); }
        else if (order == "YZY") { q.set( s2 * s1_3, c2 * s13, s2 * c1_3, c2 * c13 ); }
        else if (order == "ZXZ") { q.set( s2 * c1_3, s2 * s1_3, c2 * s13, c2 * c13 ); }
        else if (order == "XZX") { q.set( c2 * s13, s2 * s3_1, s2 * c3_1, c2 * c13 ); }
        else if (order == "YXY") { q.set( s2 * c3_1, c2 * s13, s2 * s3_1, c2 * c13 ); }
        else if (order == "ZYZ") { q.set( s2 * s3_1, s2 * c3_1, c2 * s13, c2 * c13 ); }
    }
}
