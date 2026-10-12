#include "extras/core/Interpolations.h"

double CatmullRom(double t, double p0, double p1, double p2, double p3) {
    const double v0 = (p2 - p0) * 0.5;
    const double v1 = (p3 - p1) * 0.5;
    const double t2 = t * t;
    const double t3 = t * t2;
    return (2 * p1 - 2 * p2 + v0 + v1) * t3 + (-3 * p1 + 3 * p2 - 2 * v0 - v1) * t2 + v0 * t + p1;
}

double QuadraticBezier(double t, double p0, double p1, double p2) {
    const double k = 1 - t;
    return k * k * p0 + 2 * (1 - t) * t * p1 + t * t * p2;
}

double CubicBezier(double t, double p0, double p1, double p2, double p3) {
    const double k0 = 1 - t;
    const double k1 = 1 - t;
    return k0 * k0 * k0 * p0 + 3 * k1 * k1 * t * p1 + 3 * (1 - t) * t * t * p2 + t * t * t * p3;
}
