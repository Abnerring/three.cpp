#include "geometries/ParametricFunctions.h"
#include "math/JSNumberMath.h"

// Every expression below is the pinned source expression with JSNumberMath::sin/cos standing
// in for Math.sin/Math.cos and the same reassignment order for the incoming parameters.
namespace {
// Math.PI as a binary64 literal.
constexpr double PI = 3.141592653589793;
}

namespace ParametricFunctions {

void klein(double v, double u, Vector3& target) {
    u *= PI;
    v *= 2 * PI;
    u = u * 2;
    double x, z;
    if (u < PI) {
        x = 3 * JSNumberMath::cos(u) * (1 + JSNumberMath::sin(u)) +
            (2 * (1 - JSNumberMath::cos(u) / 2)) * JSNumberMath::cos(u) * JSNumberMath::cos(v);
        z = -8 * JSNumberMath::sin(u) -
            2 * (1 - JSNumberMath::cos(u) / 2) * JSNumberMath::sin(u) * JSNumberMath::cos(v);
    } else {
        x = 3 * JSNumberMath::cos(u) * (1 + JSNumberMath::sin(u)) +
            (2 * (1 - JSNumberMath::cos(u) / 2)) * JSNumberMath::cos(v + PI);
        z = -8 * JSNumberMath::sin(u);
    }
    const double y = -2 * (1 - JSNumberMath::cos(u) / 2) * JSNumberMath::sin(v);
    target.set(x, y, z);
}

void plane(double u, double v, Vector3& target) {
    target.set(u, 0, v);
}

void mobius(double u, double t, Vector3& target) {
    u = u - 0.5;
    const double v = 2 * PI * t;
    const double a = 2;
    const double x = JSNumberMath::cos(v) * (a + u * JSNumberMath::cos(v / 2));
    const double y = JSNumberMath::sin(v) * (a + u * JSNumberMath::cos(v / 2));
    const double z = u * JSNumberMath::sin(v / 2);
    target.set(x, y, z);
}

void mobius3d(double u, double t, Vector3& target) {
    u *= PI;
    t *= 2 * PI;
    u = u * 2;
    const double phi = u / 2;
    const double major = 2.25, a = 0.125, b = 0.65;
    double x = a * JSNumberMath::cos(t) * JSNumberMath::cos(phi) - b * JSNumberMath::sin(t) * JSNumberMath::sin(phi);
    const double z = a * JSNumberMath::cos(t) * JSNumberMath::sin(phi) + b * JSNumberMath::sin(t) * JSNumberMath::cos(phi);
    const double y = (major + x) * JSNumberMath::sin(u);
    x = (major + x) * JSNumberMath::cos(u);
    target.set(x, y, z);
}

} // namespace ParametricFunctions
