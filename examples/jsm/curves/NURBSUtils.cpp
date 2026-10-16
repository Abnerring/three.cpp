#include "curves/NURBSUtils.h"
#include "core/JSValue.h"
#include <cmath>
#include <cstddef>
#include <limits>

namespace {
// JavaScript array reads: only an integral index inside the array yields a value, any
// other index is `undefined`, which the algorithms carry as NaN.
bool integerIndex(double index, std::size_t size) {
    return std::isfinite(index) && index >= 0 && std::trunc(index) == index && index < double(size);
}
double knotAt(const NURBSUtils::KnotVector& U, double index) {
    return integerIndex(index, U.size()) ? U[std::size_t(index)] : std::numeric_limits<double>::quiet_NaN();
}
// A missing control point is read through `.x` upstream and throws a TypeError.
const Vector4& controlAt(const NURBSUtils::ControlPoints& P, double index) {
    if (!integerIndex(index, P.size()))
        throw JSTypeError("Cannot read properties of undefined (reading 'x')");
    return P[std::size_t(index)];
}
std::size_t count(double value) {
    // Loop counters in the upstream source are integral; a negative or non-finite bound
    // produces an empty loop there, which maps to zero iterations here.
    if (!std::isfinite(value) || value <= 0) return 0;
    return std::size_t(std::trunc(value));
}
}

namespace NURBSUtils {

double findSpan(double p, double u, const KnotVector& U) {
    const double n = double(U.size()) - p - 1;
    if (u >= knotAt(U, n)) return n - 1;
    if (u <= knotAt(U, p)) return p;
    double low = p, high = n, mid = std::floor((low + high) / 2);
    while (u < knotAt(U, mid) || u >= knotAt(U, mid + 1)) {
        if (u < knotAt(U, mid)) {
            high = mid;
        } else {
            low = mid;
        }
        const double next = std::floor((low + high) / 2);
        // The upstream loop does not terminate for a non-monotonic knot vector; keep the
        // JavaScript result for every converging input and report the rest.
        if (next == mid) throw JSRangeError("NURBSUtils.findSpan: knot vector does not converge");
        mid = next;
    }
    return mid;
}

std::vector<double> calcBasisFunctions(double span, double u, double p, const KnotVector& U) {
    std::vector<double> N, left, right;
    N.resize(count(p) + 1, 0.);
    left.resize(count(p) + 1, 0.);
    right.resize(count(p) + 1, 0.);
    N[0] = 1.;
    for (std::size_t j = 1; j <= count(p); ++j) {
        left[j] = u - knotAt(U, span + 1 - double(j));
        right[j] = knotAt(U, span + double(j)) - u;
        double saved = 0.;
        for (std::size_t r = 0; r < j; ++r) {
            const double rv = right[r + 1];
            const double lv = left[j - r];
            const double temp = N[r] / (rv + lv);
            N[r] = saved + rv * temp;
            saved = lv * temp;
        }
        N[j] = saved;
    }
    return N;
}

Vector4 calcBSplinePoint(double p, const KnotVector& U, const ControlPoints& P, double u) {
    const double span = findSpan(p, u, U);
    const std::vector<double> N = calcBasisFunctions(span, u, p, U);
    Vector4 C(0, 0, 0, 0);
    for (std::size_t j = 0; j <= count(p); ++j) {
        const Vector4& point = controlAt(P, span - p + double(j));
        const double Nj = N[j];
        const double wNj = point.w * Nj;
        C.x += point.x * wNj;
        C.y += point.y * wNj;
        C.z += point.z * wNj;
        C.w += point.w * Nj;
    }
    return C;
}

std::vector<std::vector<double>> calcBasisFunctionDerivatives(double span, double u, double p, double n,
                                                             const KnotVector& U) {
    const std::size_t degree = count(p), derivatives = count(n);
    std::vector<double> zeroArr(degree + 1, 0.);
    std::vector<std::vector<double>> ders(derivatives + 1, zeroArr);
    std::vector<std::vector<double>> ndu(degree + 1, zeroArr);
    ndu[0][0] = 1.;
    std::vector<double> left = zeroArr, right = zeroArr;
    for (std::size_t j = 1; j <= degree; ++j) {
        left[j] = u - knotAt(U, span + 1 - double(j));
        right[j] = knotAt(U, span + double(j)) - u;
        double saved = 0.;
        for (std::size_t r = 0; r < j; ++r) {
            const double rv = right[r + 1];
            const double lv = left[j - r];
            ndu[j][r] = rv + lv;
            const double temp = ndu[r][j - 1] / ndu[j][r];
            ndu[r][j] = saved + rv * temp;
            saved = lv * temp;
        }
        ndu[j][j] = saved;
    }
    for (std::size_t j = 0; j <= degree; ++j) ders[0][j] = ndu[j][degree];
    for (std::size_t r = 0; r <= degree; ++r) {
        std::size_t s1 = 0, s2 = 1;
        std::vector<std::vector<double>> a(degree + 1, zeroArr);
        a[0][0] = 1.;
        for (std::size_t k = 1; k <= derivatives; ++k) {
            double d = 0.;
            const double rk = double(r) - double(k);
            const double pk = double(degree) - double(k);
            if (double(r) >= double(k)) {
                a[s2][0] = a[s1][0] / ndu[std::size_t(pk) + 1][std::size_t(rk)];
                d = a[s2][0] * ndu[std::size_t(rk)][std::size_t(pk)];
            }
            const double j1 = (rk >= -1) ? 1 : -rk;
            const double j2 = (double(r) - 1 <= pk) ? double(k) - 1 : double(degree) - double(r);
            for (double j = j1; j <= j2; j += 1) {
                a[s2][std::size_t(j)] = (a[s1][std::size_t(j)] - a[s1][std::size_t(j - 1)]) /
                                        ndu[std::size_t(pk) + 1][std::size_t(rk + j)];
                d += a[s2][std::size_t(j)] * ndu[std::size_t(rk + j)][std::size_t(pk)];
            }
            if (double(r) <= pk) {
                a[s2][k] = -a[s1][k - 1] / ndu[std::size_t(pk) + 1][r];
                d += a[s2][k] * ndu[r][std::size_t(pk)];
            }
            ders[k][r] = d;
            const std::size_t j = s1;
            s1 = s2;
            s2 = j;
        }
    }
    double r = p;
    for (std::size_t k = 1; k <= derivatives; ++k) {
        for (std::size_t j = 0; j <= degree; ++j) ders[k][j] *= r;
        r *= p - double(k);
    }
    return ders;
}

std::vector<Vector4> calcBSplineDerivatives(double p, const KnotVector& U, const ControlPoints& P, double u, double nd) {
    const double du = nd < p ? nd : p;
    const double span = findSpan(p, u, U);
    const std::vector<std::vector<double>> nders = calcBasisFunctionDerivatives(span, u, p, du, U);
    ControlPoints Pw(P.size());
    for (std::size_t i = 0; i < P.size(); ++i) {
        Vector4 point = P[i];
        const double w = point.w;
        point.x *= w;
        point.y *= w;
        point.z *= w;
        Pw[i] = point;
    }
    std::vector<Vector4> CK;
    for (std::size_t k = 0; k <= count(du); ++k) {
        // `Pw[ span - p ].clone().multiplyScalar( ... )` scales a copy, never Pw itself.
        Vector4 point = controlAt(Pw, span - p);
        point.multiplyScalar(nders[k][0]);
        for (std::size_t j = 1; j <= count(p); ++j) {
            Vector4 scaled = controlAt(Pw, span - p + double(j));
            point.add(scaled.multiplyScalar(nders[k][j]));
        }
        CK.push_back(point);
    }
    // `for ( let k = du + 1; k <= nd + 1; ++ k ) CK[ k ] = new Vector4( 0, 0, 0 )`
    // writes one entry past the derivative count, with the upstream default w = 1.
    for (std::size_t k = count(du) + 1; k <= count(nd) + 1; ++k) {
        CK.resize(k + 1);
        CK[k] = Vector4(0, 0, 0);
    }
    return CK;
}

double calcKoverI(double k, double i) {
    double nom = 1;
    for (double j = 2; j <= k; j += 1) nom *= j;
    double denom = 1;
    for (double j = 2; j <= i; j += 1) denom *= j;
    for (double j = 2; j <= k - i; j += 1) denom *= j;
    return nom / denom;
}

std::vector<Vector3> calcRationalCurveDerivatives(const std::vector<Vector4>& Pders) {
    const std::size_t nd = Pders.size();
    std::vector<Vector3> Aders;
    std::vector<double> wders;
    for (std::size_t i = 0; i < nd; ++i) {
        const Vector4& point = Pders[i];
        Aders.emplace_back(point.x, point.y, point.z);
        wders.push_back(point.w);
    }
    std::vector<Vector3> CK;
    for (std::size_t k = 0; k < nd; ++k) {
        Vector3 v = Aders[k];
        for (std::size_t i = 1; i <= k; ++i)
            v.sub(CK[k - i].clone().multiplyScalar(calcKoverI(double(k), double(i)) * wders[i]));
        CK.push_back(v.divideScalar(wders[0]));
    }
    return CK;
}

std::vector<Vector3> calcNURBSDerivatives(double p, const KnotVector& U, const ControlPoints& P, double u, double nd) {
    const std::vector<Vector4> Pders = calcBSplineDerivatives(p, U, P, u, nd);
    return calcRationalCurveDerivatives(Pders);
}

void calcSurfacePoint(double p, double q, const KnotVector& U, const KnotVector& V, const ControlGrid& P,
                      double u, double v, Vector3& target) {
    const double uspan = findSpan(p, u, U);
    const double vspan = findSpan(q, v, V);
    const std::vector<double> Nu = calcBasisFunctions(uspan, u, p, U);
    const std::vector<double> Nv = calcBasisFunctions(vspan, v, q, V);
    std::vector<Vector4> temp;
    for (std::size_t l = 0; l <= count(q); ++l) {
        temp.emplace_back(0, 0, 0, 0);
        for (std::size_t k = 0; k <= count(p); ++k) {
            const std::size_t row = std::size_t(uspan - p + double(k));
            const std::size_t column = std::size_t(vspan - q + double(l));
            if (row >= P.size() || column >= P[row].size())
                throw JSTypeError("Cannot read properties of undefined (reading 'clone')");
            Vector4 point = P[row][column];
            const double w = point.w;
            point.x *= w;
            point.y *= w;
            point.z *= w;
            temp[l].add(point.multiplyScalar(Nu[k]));
        }
    }
    Vector4 Sw(0, 0, 0, 0);
    for (std::size_t l = 0; l <= count(q); ++l) Sw.add(temp[l].multiplyScalar(Nv[l]));
    Sw.divideScalar(Sw.w);
    target.set(Sw.x, Sw.y, Sw.z);
}

void calcVolumePoint(double p, double q, double r, const KnotVector& U, const KnotVector& V, const KnotVector& W,
                     const ControlVolume& P, double u, double v, double w, Vector3& target) {
    const double uspan = findSpan(p, u, U);
    const double vspan = findSpan(q, v, V);
    const double wspan = findSpan(r, w, W);
    const std::vector<double> Nu = calcBasisFunctions(uspan, u, p, U);
    const std::vector<double> Nv = calcBasisFunctions(vspan, v, q, V);
    const std::vector<double> Nw = calcBasisFunctions(wspan, w, r, W);
    std::vector<std::vector<Vector4>> temp;
    for (std::size_t m = 0; m <= count(r); ++m) {
        temp.emplace_back();
        for (std::size_t l = 0; l <= count(q); ++l) {
            temp[m].emplace_back(0, 0, 0, 0);
            for (std::size_t k = 0; k <= count(p); ++k) {
                const std::size_t plane = std::size_t(uspan - p + double(k));
                const std::size_t row = std::size_t(vspan - q + double(l));
                const std::size_t column = std::size_t(wspan - r + double(m));
                if (plane >= P.size() || row >= P[plane].size() || column >= P[plane][row].size())
                    throw JSTypeError("Cannot read properties of undefined (reading 'clone')");
                Vector4 point = P[plane][row][column];
                const double weight = point.w;
                point.x *= weight;
                point.y *= weight;
                point.z *= weight;
                temp[m][l].add(point.multiplyScalar(Nu[k]));
            }
        }
    }
    Vector4 Sw(0, 0, 0, 0);
    for (std::size_t m = 0; m <= count(r); ++m)
        for (std::size_t l = 0; l <= count(q); ++l)
            Sw.add(temp[m][l].multiplyScalar(Nw[m]).multiplyScalar(Nv[l]));
    Sw.divideScalar(Sw.w);
    target.set(Sw.x, Sw.y, Sw.z);
}

} // namespace NURBSUtils
