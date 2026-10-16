// Module-scope state of the pinned examples/jsm/math/ImprovedNoise.js port: the 256 entry
// permutation table and its 256 entry copy, plus the two file local helpers `fade` and `grad`.
#include "math/ImprovedNoise.h"
#include "math/MathUtils.h"
#include <array>
#include <cmath>
#include <cstdint>

namespace {

// const _p = [ ... ] followed by `for ( let i = 0; i < 256; i ++ ) _p[ 256 + i ] = _p[ i ]`.
constexpr std::array<std::uint8_t, 512> buildPermutation() {
    constexpr std::uint8_t base[256] = {
        151, 160, 137, 91, 90, 15, 131, 13, 201, 95, 96, 53, 194, 233, 7, 225, 140, 36, 103, 30,
        69, 142, 8, 99, 37, 240, 21, 10, 23, 190, 6, 148, 247, 120, 234, 75, 0, 26, 197, 62, 94,
        252, 219, 203, 117, 35, 11, 32, 57, 177, 33, 88, 237, 149, 56, 87, 174, 20, 125, 136,
        171, 168, 68, 175, 74, 165, 71, 134, 139, 48, 27, 166, 77, 146, 158, 231, 83, 111, 229,
        122, 60, 211, 133, 230, 220, 105, 92, 41, 55, 46, 245, 40, 244, 102, 143, 54, 65, 25,
        63, 161, 1, 216, 80, 73, 209, 76, 132, 187, 208, 89, 18, 169, 200, 196, 135, 130, 116,
        188, 159, 86, 164, 100, 109, 198, 173, 186, 3, 64, 52, 217, 226, 250, 124, 123, 5, 202,
        38, 147, 118, 126, 255, 82, 85, 212, 207, 206, 59, 227, 47, 16, 58, 17, 182, 189, 28, 42,
        223, 183, 170, 213, 119, 248, 152, 2, 44, 154, 163, 70, 221, 153, 101, 155, 167, 43, 172,
        9, 129, 22, 39, 253, 19, 98, 108, 110, 79, 113, 224, 232, 178, 185, 112, 104, 218, 246,
        97, 228, 251, 34, 242, 193, 238, 210, 144, 12, 191, 179, 162, 241, 81, 51, 145, 235, 249,
        14, 239, 107, 49, 192, 214, 31, 181, 199, 106, 157, 184, 84, 204, 176, 115, 121, 50, 45,
        127, 4, 150, 254, 138, 236, 205, 93, 222, 114, 67, 29, 24, 72, 243, 141, 128, 195, 78,
        66, 215, 61, 156, 180};
    std::array<std::uint8_t, 512> table{};
    for (std::size_t i = 0; i < 256; ++i) {
        table[i] = base[i];
        table[256 + i] = base[i];
    }
    return table;
}
constexpr std::array<std::uint8_t, 512> p = buildPermutation();

double fade(double t) { return t * t * t * (t * (t * 6 - 15) + 10); }

double grad(int hash, double x, double y, double z) {
    const int h = hash & 15;
    const double u = h < 8 ? x : y;
    const double v = h < 4 ? y : (h == 12 || h == 14 ? x : z);
    return ((h & 1) == 0 ? u : -u) + ((h & 2) == 0 ? v : -v);
}

// `floorX & 255` applies the ECMAScript ToInt32 conversion to the floored value first, so
// non-finite values and magnitudes of 2^32 or more reduce to 0 instead of relying on a
// platform conversion. The mask itself keeps every table index in [0, 511] like the source.
int toInt32(double value) {
    if (!std::isfinite(value)) return 0;
    double wrapped = std::fmod(std::trunc(value), 4294967296.);
    if (wrapped < 0) wrapped += 4294967296.;
    if (wrapped >= 2147483648.) wrapped -= 4294967296.;
    return static_cast<int>(wrapped);
}

} // namespace

double ImprovedNoise::noise(double x, double y, double z) const {
    const double floorX = std::floor(x), floorY = std::floor(y), floorZ = std::floor(z);

    const int X = toInt32(floorX) & 255, Y = toInt32(floorY) & 255, Z = toInt32(floorZ) & 255;

    x -= floorX;
    y -= floorY;
    z -= floorZ;

    const double xMinus1 = x - 1, yMinus1 = y - 1, zMinus1 = z - 1;

    const double u = fade(x), v = fade(y), w = fade(z);

    const int A = p[X] + Y, AA = p[A] + Z, AB = p[A + 1] + Z;
    const int B = p[X + 1] + Y, BA = p[B] + Z, BB = p[B + 1] + Z;

    return MathUtils::lerp(
        MathUtils::lerp(MathUtils::lerp(grad(p[AA], x, y, z), grad(p[BA], xMinus1, y, z), u),
                        MathUtils::lerp(grad(p[AB], x, yMinus1, z), grad(p[BB], xMinus1, yMinus1, z), u), v),
        MathUtils::lerp(
            MathUtils::lerp(grad(p[AA + 1], x, y, zMinus1), grad(p[BA + 1], xMinus1, y, zMinus1), u),
            MathUtils::lerp(grad(p[AB + 1], x, yMinus1, zMinus1), grad(p[BB + 1], xMinus1, yMinus1, zMinus1), u), v),
        w);
}
