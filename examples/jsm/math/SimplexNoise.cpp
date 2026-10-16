#include "math/SimplexNoise.h"
#include "math/MathUtils.h"
#include <cmath>
#include <cstdint>

namespace {

// `i & 255` / `x0 > y0` and friends operate on the floored coordinate in the source, where
// `&` applies the ECMAScript ToInt32 conversion first. Implementing that conversion keeps
// non-finite and out of int32 range coordinates in the same 0..255 bucket as the JavaScript
// `%`-free table reduction; `static_cast<int>` alone would be undefined behaviour for the
// magnitudes the differential grid reaches (upstream ImprovedNoise.cpp uses the same helper).
int toInt32(double value) {
    if (!std::isfinite(value)) return 0;
    double wrapped = std::fmod(std::trunc(value), 4294967296.);
    if (wrapped < 0) wrapped += 4294967296.;
    if (wrapped >= 2147483648.) wrapped -= 4294967296.;
    return static_cast<int>(wrapped);
}

// `ii = i & 255`: the mask never changes the sign of a value already in [ -255, 255 ], so the
// result stays inside the doubled table for every finite input.
int wrap255(double floored) { return toInt32(floored) & 255; }

// The native stand-in for the default `r = Math` argument: `Math.random()` has no counterpart,
// so the default source is the deterministic generator of src/math/MathUtils.h.
double nativeRandom() { return MathUtils::seededRandom(); }

// Index helpers for the permutation lookups. Every gradient index of the source is
// `perm[ <non negative integer> ]` with an argument at most 510, and every entry of `perm` is a
// floored `random() * 256`, so the conversions below only make the JavaScript number -> table
// index step explicit. There is one helper per chain length:
//
//   2D  gradient2( perm, ii, jj )   == perm[ ii + perm[ jj ] ]
//   3D  gradient3( perm, ii, jj, kk ) == perm[ ii + perm[ jj + perm[ kk ] ] ]
//   4D  gradient4( perm, ii, jj, kk, ll ) == perm[ ii + perm[ jj + perm[ kk + perm[ ll ] ] ] ]
//
// The sums are spelled out level by level so no result depends on the argument evaluation order
// of a single expression; the `% 12` / `% 32` remainders of the source are applied by the
// callers.
std::size_t permIndex(double value) { return static_cast<std::size_t>(value); }

double gradient2(const std::vector<double>& perm, int ii, int jj) {
    const double inner = perm[permIndex(jj)];
    return perm[permIndex(ii + inner)];
}
double gradient3(const std::vector<double>& perm, int ii, int jj, int kk) {
    const double innermost = perm[permIndex(kk)];
    const double inner = perm[permIndex(jj + innermost)];
    return perm[permIndex(ii + inner)];
}
double gradient4(const std::vector<double>& perm, int ii, int jj, int kk, int ll) {
    const double level3 = perm[permIndex(ll)];
    const double level2 = perm[permIndex(kk + level3)];
    const double level1 = perm[permIndex(jj + level2)];
    return perm[permIndex(ii + level1)];
}

} // namespace

SimplexNoise::SimplexNoise(RandomSource random)
    : grad3{{{1, 1, 0},
             {-1, 1, 0},
             {1, -1, 0},
             {-1, -1, 0},
             {1, 0, 1},
             {-1, 0, 1},
             {1, 0, -1},
             {-1, 0, -1},
             {0, 1, 1},
             {0, -1, 1},
             {0, 1, -1},
             {0, -1, -1}}},
      grad4{{{0, 1, 1, 1},
             {0, 1, 1, -1},
             {0, 1, -1, 1},
             {0, 1, -1, -1},
             {0, -1, 1, 1},
             {0, -1, 1, -1},
             {0, -1, -1, 1},
             {0, -1, -1, -1},
             {1, 0, 1, 1},
             {1, 0, 1, -1},
             {1, 0, -1, 1},
             {1, 0, -1, -1},
             {-1, 0, 1, 1},
             {-1, 0, 1, -1},
             {-1, 0, -1, 1},
             {-1, 0, -1, -1},
             {1, 1, 0, 1},
             {1, 1, 0, -1},
             {1, -1, 0, 1},
             {1, -1, 0, -1},
             {-1, 1, 0, 1},
             {-1, 1, 0, -1},
             {-1, -1, 0, 1},
             {-1, -1, 0, -1},
             {1, 1, 1, 0},
             {1, 1, -1, 0},
             {1, -1, 1, 0},
             {1, -1, -1, 0},
             {-1, 1, 1, 0},
             {-1, 1, -1, 0},
             {-1, -1, 1, 0},
             {-1, -1, -1, 0}}},
      p(256),
      perm(512),
      simplex{{{0, 1, 2, 3},
               {0, 1, 3, 2},
               {0, 0, 0, 0},
               {0, 2, 3, 1},
               {0, 0, 0, 0},
               {0, 0, 0, 0},
               {0, 0, 0, 0},
               {1, 2, 3, 0},
               {0, 2, 1, 3},
               {0, 0, 0, 0},
               {0, 3, 1, 2},
               {0, 3, 2, 1},
               {0, 0, 0, 0},
               {0, 0, 0, 0},
               {0, 0, 0, 0},
               {1, 3, 2, 0},
               {0, 0, 0, 0},
               {0, 0, 0, 0},
               {0, 0, 0, 0},
               {0, 0, 0, 0},
               {0, 0, 0, 0},
               {0, 0, 0, 0},
               {0, 0, 0, 0},
               {0, 0, 0, 0},
               {1, 2, 0, 3},
               {0, 0, 0, 0},
               {1, 3, 0, 2},
               {0, 0, 0, 0},
               {0, 0, 0, 0},
               {0, 0, 0, 0},
               {2, 3, 0, 1},
               {2, 3, 1, 0},
               {1, 0, 2, 3},
               {1, 0, 3, 2},
               {0, 0, 0, 0},
               {0, 0, 0, 0},
               {0, 0, 0, 0},
               {2, 0, 3, 1},
               {0, 0, 0, 0},
               {2, 1, 3, 0},
               {0, 0, 0, 0},
               {0, 0, 0, 0},
               {0, 0, 0, 0},
               {0, 0, 0, 0},
               {0, 0, 0, 0},
               {0, 0, 0, 0},
               {0, 0, 0, 0},
               {0, 0, 0, 0},
               {2, 0, 1, 3},
               {0, 0, 0, 0},
               {0, 0, 0, 0},
               {0, 0, 0, 0},
               {3, 0, 1, 2},
               {3, 0, 2, 1},
               {0, 0, 0, 0},
               {3, 1, 2, 0},
               {2, 1, 0, 3},
               {0, 0, 0, 0},
               {0, 0, 0, 0},
               {0, 0, 0, 0},
               {3, 1, 0, 2},
               {0, 0, 0, 0},
               {3, 2, 0, 1},
               {3, 2, 1, 0}}} {
    // const r = random === undefined ? Math : random; for every `r.random()` call the native
    // port uses the injected callable or the native default source. The callable is copied so
    // the default-source object outlives this statement.
    const RandomSource source = random ? random : RandomSource(nativeRandom);

    for (int i = 0; i < 256; i++) {
        this->p[i] = std::floor(source() * 256);
    }

    // To remove the need for index wrapping, double the permutation table length
    for (int i = 0; i < 512; i++) {
        this->perm[i] = this->p[(i & 255)];
    }
}

double SimplexNoise::noise(double xin, double yin) const {
    double n0; // Noise contributions from the three corners
    double n1;
    double n2;
    // Skew the input space to determine which simplex cell we're in
    const double F2 = 0.5 * (std::sqrt(3.0) - 1.0);
    const double s = (xin + yin) * F2; // Hairy factor for 2D
    const double i = std::floor(xin + s);
    const double j = std::floor(yin + s);
    const double G2 = (3.0 - std::sqrt(3.0)) / 6.0;
    const double t = (i + j) * G2;
    const double X0 = i - t; // Unskew the cell origin back to (x,y) space
    const double Y0 = j - t;
    const double x0 = xin - X0; // The x,y distances from the cell origin
    const double y0 = yin - Y0;

    // For the 2D case, the simplex shape is an equilateral triangle.
    // Determine which simplex we are in.
    double i1; // Offsets for second (middle) corner of simplex in (i,j) coords

    double j1;
    if (x0 > y0) {
        i1 = 1;
        j1 = 0;

        // lower triangle, XY order: (0,0)->(1,0)->(1,1)

    } else {
        i1 = 0;
        j1 = 1;

    } // upper triangle, YX order: (0,0)->(0,1)->(1,1)

    // A step of (1,0) in (i,j) means a step of (1-c,-c) in (x,y), and
    // a step of (0,1) in (i,j) means a step of (-c,1-c) in (x,y), where
    // c = (3-sqrt(3))/6
    const double x1 = x0 - i1 + G2; // Offsets for middle corner in (x,y) unskewed coords
    const double y1 = y0 - j1 + G2;
    const double x2 = x0 - 1.0 + 2.0 * G2; // Offsets for last corner in (x,y) unskewed coords
    const double y2 = y0 - 1.0 + 2.0 * G2;
    // Work out the hashed gradient indices of the three simplex corners
    const int ii = wrap255(i);
    const int jj = wrap255(j);
    const int gi0 = static_cast<int>(std::fmod(gradient2(this->perm, ii, jj), 12.0));
    const int gi1 = static_cast<int>(
        std::fmod(gradient2(this->perm, ii + int(i1), jj + int(j1)), 12.0));
    const int gi2 = static_cast<int>(std::fmod(gradient2(this->perm, ii + 1, jj + 1), 12.0));
    // Calculate the contribution from the three corners
    double t0 = 0.5 - x0 * x0 - y0 * y0;
    if (t0 < 0)
        n0 = 0.0;
    else {
        t0 *= t0;
        n0 = t0 * t0 * (this->grad3[gi0][0] * x0 + this->grad3[gi0][1] * y0); // (x,y) of grad3
                                                                             // used for 2D
                                                                             // gradient

    }

    double t1 = 0.5 - x1 * x1 - y1 * y1;
    if (t1 < 0)
        n1 = 0.0;
    else {
        t1 *= t1;
        n1 = t1 * t1 * (this->grad3[gi1][0] * x1 + this->grad3[gi1][1] * y1);

    }

    double t2 = 0.5 - x2 * x2 - y2 * y2;
    if (t2 < 0)
        n2 = 0.0;
    else {
        t2 *= t2;
        n2 = t2 * t2 * (this->grad3[gi2][0] * x2 + this->grad3[gi2][1] * y2);

    }

    // Add contributions from each corner to get the final noise value.
    // The result is scaled to return values in the interval [-1,1].
    return 70.0 * (n0 + n1 + n2);
}

double SimplexNoise::noise3d(double xin, double yin, double zin) const {
    double n0; // Noise contributions from the four corners
    double n1;
    double n2;
    double n3;
    // Skew the input space to determine which simplex cell we're in
    const double F3 = 1.0 / 3.0;
    const double s = (xin + yin + zin) * F3; // Very nice and simple skew factor for 3D
    const double i = std::floor(xin + s);
    const double j = std::floor(yin + s);
    const double k = std::floor(zin + s);
    const double G3 = 1.0 / 6.0; // Very nice and simple unskew factor, too
    const double t = (i + j + k) * G3;
    const double X0 = i - t; // Unskew the cell origin back to (x,y,z) space
    const double Y0 = j - t;
    const double Z0 = k - t;
    const double x0 = xin - X0; // The x,y,z distances from the cell origin
    const double y0 = yin - Y0;
    const double z0 = zin - Z0;

    // For the 3D case, the simplex shape is a slightly irregular tetrahedron.
    // Determine which simplex we are in.
    double i1; // Offsets for second corner of simplex in (i,j,k) coords

    double j1;
    double k1;
    double i2; // Offsets for third corner of simplex in (i,j,k) coords
    double j2;
    double k2;
    if (x0 >= y0) {
        if (y0 >= z0) {
            i1 = 1;
            j1 = 0;
            k1 = 0;
            i2 = 1;
            j2 = 1;
            k2 = 0;

            // X Y Z order

        } else if (x0 >= z0) {
            i1 = 1;
            j1 = 0;
            k1 = 0;
            i2 = 1;
            j2 = 0;
            k2 = 1;

            // X Z Y order

        } else {
            i1 = 0;
            j1 = 0;
            k1 = 1;
            i2 = 1;
            j2 = 0;
            k2 = 1;

        } // Z X Y order

    } else { // x0<y0

        if (y0 < z0) {
            i1 = 0;
            j1 = 0;
            k1 = 1;
            i2 = 0;
            j2 = 1;
            k2 = 1;

            // Z Y X order

        } else if (x0 < z0) {
            i1 = 0;
            j1 = 1;
            k1 = 0;
            i2 = 0;
            j2 = 1;
            k2 = 1;

            // Y Z X order

        } else {
            i1 = 0;
            j1 = 1;
            k1 = 0;
            i2 = 1;
            j2 = 1;
            k2 = 0;

        } // Y X Z order

    }

    // A step of (1,0,0) in (i,j,k) means a step of (1-c,-c,-c) in (x,y,z),
    // a step of (0,1,0) in (i,j,k) means a step of (-c,1-c,-c) in (x,y,z), and
    // a step of (0,0,1) in (i,j,k) means a step of (-c,-c,1-c) in (x,y,z), where
    // c = 1/6.
    const double x1 = x0 - i1 + G3; // Offsets for second corner in (x,y,z) coords
    const double y1 = y0 - j1 + G3;
    const double z1 = z0 - k1 + G3;
    const double x2 = x0 - i2 + 2.0 * G3; // Offsets for third corner in (x,y,z) coords
    const double y2 = y0 - j2 + 2.0 * G3;
    const double z2 = z0 - k2 + 2.0 * G3;
    const double x3 = x0 - 1.0 + 3.0 * G3; // Offsets for last corner in (x,y,z) coords
    const double y3 = y0 - 1.0 + 3.0 * G3;
    const double z3 = z0 - 1.0 + 3.0 * G3;
    // Work out the hashed gradient indices of the four simplex corners
    const int ii = wrap255(i);
    const int jj = wrap255(j);
    const int kk = wrap255(k);
    const int gi0 = static_cast<int>(std::fmod(gradient3(this->perm, ii, jj, kk), 12.0));
    const int gi1 = static_cast<int>(
        std::fmod(gradient3(this->perm, ii + int(i1), jj + int(j1), kk + int(k1)), 12.0));
    const int gi2 = static_cast<int>(
        std::fmod(gradient3(this->perm, ii + int(i2), jj + int(j2), kk + int(k2)), 12.0));
    const int gi3 = static_cast<int>(std::fmod(gradient3(this->perm, ii + 1, jj + 1, kk + 1), 12.0));
    // Calculate the contribution from the four corners
    double t0 = 0.6 - x0 * x0 - y0 * y0 - z0 * z0;
    if (t0 < 0)
        n0 = 0.0;
    else {
        t0 *= t0;
        n0 = t0 * t0 * (this->grad3[gi0][0] * x0 + this->grad3[gi0][1] * y0 + this->grad3[gi0][2] * z0);

    }

    double t1 = 0.6 - x1 * x1 - y1 * y1 - z1 * z1;
    if (t1 < 0)
        n1 = 0.0;
    else {
        t1 *= t1;
        n1 = t1 * t1 * (this->grad3[gi1][0] * x1 + this->grad3[gi1][1] * y1 + this->grad3[gi1][2] * z1);

    }

    double t2 = 0.6 - x2 * x2 - y2 * y2 - z2 * z2;
    if (t2 < 0)
        n2 = 0.0;
    else {
        t2 *= t2;
        n2 = t2 * t2 * (this->grad3[gi2][0] * x2 + this->grad3[gi2][1] * y2 + this->grad3[gi2][2] * z2);

    }

    double t3 = 0.6 - x3 * x3 - y3 * y3 - z3 * z3;
    if (t3 < 0)
        n3 = 0.0;
    else {
        t3 *= t3;
        n3 = t3 * t3 * (this->grad3[gi3][0] * x3 + this->grad3[gi3][1] * y3 + this->grad3[gi3][2] * z3);

    }

    // Add contributions from each corner to get the final noise value.
    // The result is scaled to stay just inside [-1,1]
    return 32.0 * (n0 + n1 + n2 + n3);
}

double SimplexNoise::noise4d(double x, double y, double z, double w) const {
    // For faster and easier lookups the source caches `grad4`, `simplex` and `perm` in local
    // variables; the native port reads the members directly, which keeps the same values
    // without shadowing the members of the same name.

    // The skewing and unskewing factors are hairy again for the 4D case
    const double F4 = (std::sqrt(5.0) - 1.0) / 4.0;
    const double G4 = (5.0 - std::sqrt(5.0)) / 20.0;
    double n0; // Noise contributions from the five corners
    double n1;
    double n2;
    double n3;
    double n4;
    // Skew the (x,y,z,w) space to determine which cell of 24 simplices we're in
    const double s = (x + y + z + w) * F4; // Factor for 4D skewing
    const double i = std::floor(x + s);
    const double j = std::floor(y + s);
    const double k = std::floor(z + s);
    const double l = std::floor(w + s);
    const double t = (i + j + k + l) * G4; // Factor for 4D unskewing
    const double X0 = i - t; // Unskew the cell origin back to (x,y,z,w) space
    const double Y0 = j - t;
    const double Z0 = k - t;
    const double W0 = l - t;
    const double x0 = x - X0; // The x,y,z,w distances from the cell origin
    const double y0 = y - Y0;
    const double z0 = z - Z0;
    const double w0 = w - W0;

    // For the 4D case, the simplex is a 4D shape I won't even try to describe.
    // To find out which of the 24 possible simplices we're in, we need to
    // determine the magnitude ordering of x0, y0, z0 and w0.
    // The method below is a good way of finding the ordering of x,y,z,w and
    // then find the correct traversal order for the simplex we're in.
    // First, six pair-wise comparisons are performed between each possible pair
    // of the four coordinates, and the results are used to add up binary bits
    // for an integer index.
    const int c1 = (x0 > y0) ? 32 : 0;
    const int c2 = (x0 > z0) ? 16 : 0;
    const int c3 = (y0 > z0) ? 8 : 0;
    const int c4 = (x0 > w0) ? 4 : 0;
    const int c5 = (y0 > w0) ? 2 : 0;
    const int c6 = (z0 > w0) ? 1 : 0;
    const int c = c1 + c2 + c3 + c4 + c5 + c6;

    // simplex[c] is a 4-vector with the numbers 0, 1, 2 and 3 in some order.
    // Many values of c will never occur, since e.g. x>y>z>w makes x<z, y<w and x<w
    // impossible. Only the 24 indices which have non-zero entries make any sense.
    // We use a thresholding to set the coordinates in turn from the largest magnitude.
    // The number 3 in the "simplex" array is at the position of the largest coordinate.
    const double i1 = this->simplex[c][0] >= 3 ? 1 : 0;
    const double j1 = this->simplex[c][1] >= 3 ? 1 : 0;
    const double k1 = this->simplex[c][2] >= 3 ? 1 : 0;
    const double l1 = this->simplex[c][3] >= 3 ? 1 : 0;
    // The number 2 in the "simplex" array is at the second largest coordinate.
    const double i2 = this->simplex[c][0] >= 2 ? 1 : 0;
    const double j2 = this->simplex[c][1] >= 2 ? 1 : 0;
    const double k2 = this->simplex[c][2] >= 2 ? 1 : 0;
    const double l2 = this->simplex[c][3] >= 2 ? 1 : 0;
    // The number 1 in the "simplex" array is at the second smallest coordinate.
    const double i3 = this->simplex[c][0] >= 1 ? 1 : 0;
    const double j3 = this->simplex[c][1] >= 1 ? 1 : 0;
    const double k3 = this->simplex[c][2] >= 1 ? 1 : 0;
    const double l3 = this->simplex[c][3] >= 1 ? 1 : 0;
    // The fifth corner has all coordinate offsets = 1, so no need to look that up.
    const double x1 = x0 - i1 + G4; // Offsets for second corner in (x,y,z,w) coords
    const double y1 = y0 - j1 + G4;
    const double z1 = z0 - k1 + G4;
    const double w1 = w0 - l1 + G4;
    const double x2 = x0 - i2 + 2.0 * G4; // Offsets for third corner in (x,y,z,w) coords
    const double y2 = y0 - j2 + 2.0 * G4;
    const double z2 = z0 - k2 + 2.0 * G4;
    const double w2 = w0 - l2 + 2.0 * G4;
    const double x3 = x0 - i3 + 3.0 * G4; // Offsets for fourth corner in (x,y,z,w) coords
    const double y3 = y0 - j3 + 3.0 * G4;
    const double z3 = z0 - k3 + 3.0 * G4;
    const double w3 = w0 - l3 + 3.0 * G4;
    const double x4 = x0 - 1.0 + 4.0 * G4; // Offsets for last corner in (x,y,z,w) coords
    const double y4 = y0 - 1.0 + 4.0 * G4;
    const double z4 = z0 - 1.0 + 4.0 * G4;
    const double w4 = w0 - 1.0 + 4.0 * G4;
    // Work out the hashed gradient indices of the five simplex corners
    const int ii = wrap255(i);
    const int jj = wrap255(j);
    const int kk = wrap255(k);
    const int ll = wrap255(l);
    const int gi0 = static_cast<int>(std::fmod(gradient4(this->perm, ii, jj, kk, ll), 32.0));
    const int gi1 = static_cast<int>(std::fmod(
        gradient4(this->perm, ii + int(i1), jj + int(j1), kk + int(k1), ll + int(l1)), 32.0));
    const int gi2 = static_cast<int>(std::fmod(
        gradient4(this->perm, ii + int(i2), jj + int(j2), kk + int(k2), ll + int(l2)), 32.0));
    const int gi3 = static_cast<int>(std::fmod(
        gradient4(this->perm, ii + int(i3), jj + int(j3), kk + int(k3), ll + int(l3)), 32.0));
    const int gi4 = static_cast<int>(
        std::fmod(gradient4(this->perm, ii + 1, jj + 1, kk + 1, ll + 1), 32.0));
    // Calculate the contribution from the five corners
    double t0 = 0.6 - x0 * x0 - y0 * y0 - z0 * z0 - w0 * w0;
    if (t0 < 0)
        n0 = 0.0;
    else {
        t0 *= t0;
        n0 = t0 * t0 * (this->grad4[gi0][0] * x0 + this->grad4[gi0][1] * y0 +
                        this->grad4[gi0][2] * z0 + this->grad4[gi0][3] * w0);

    }

    double t1 = 0.6 - x1 * x1 - y1 * y1 - z1 * z1 - w1 * w1;
    if (t1 < 0)
        n1 = 0.0;
    else {
        t1 *= t1;
        n1 = t1 * t1 * (this->grad4[gi1][0] * x1 + this->grad4[gi1][1] * y1 +
                        this->grad4[gi1][2] * z1 + this->grad4[gi1][3] * w1);

    }

    double t2 = 0.6 - x2 * x2 - y2 * y2 - z2 * z2 - w2 * w2;
    if (t2 < 0)
        n2 = 0.0;
    else {
        t2 *= t2;
        n2 = t2 * t2 * (this->grad4[gi2][0] * x2 + this->grad4[gi2][1] * y2 +
                        this->grad4[gi2][2] * z2 + this->grad4[gi2][3] * w2);

    }

    double t3 = 0.6 - x3 * x3 - y3 * y3 - z3 * z3 - w3 * w3;
    if (t3 < 0)
        n3 = 0.0;
    else {
        t3 *= t3;
        n3 = t3 * t3 * (this->grad4[gi3][0] * x3 + this->grad4[gi3][1] * y3 +
                        this->grad4[gi3][2] * z3 + this->grad4[gi3][3] * w3);

    }

    double t4 = 0.6 - x4 * x4 - y4 * y4 - z4 * z4 - w4 * w4;
    if (t4 < 0)
        n4 = 0.0;
    else {
        t4 *= t4;
        n4 = t4 * t4 * (this->grad4[gi4][0] * x4 + this->grad4[gi4][1] * y4 +
                        this->grad4[gi4][2] * z4 + this->grad4[gi4][3] * w4);

    }

    // Sum up and scale the result to cover the range [-1,1]
    return 27.0 * (n0 + n1 + n2 + n3 + n4);
}
