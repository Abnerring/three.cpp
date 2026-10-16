#include "utils/CameraUtils.h"
#include "math/JSNumberMath.h"
#include "math/MathUtils.h"
#include <algorithm>

namespace {
// Module-level temporaries of the pinned source, one set per owning thread.
thread_local Vector3 va, vb, vc, vr, vu, vn, vec;
thread_local Quaternion quat;
// MathUtils.RAD2DEG of the pinned module (180 / Math.PI).
constexpr double RAD2DEG = 180.0 / MATH_PI;
}

void CameraUtils::frameCorners(PerspectiveCamera& camera, const Vector3& bottomLeftCorner,
                              const Vector3& bottomRightCorner, const Vector3& topLeftCorner,
                              bool estimateViewFrustum) {
    const Vector3& pa = bottomLeftCorner;
    const Vector3& pb = bottomRightCorner;
    const Vector3& pc = topLeftCorner;
    auto& pe = camera.position;   // eye position
    const double n = camera.near; // distance of near clipping plane
    const double f = camera.far;  // distance of far clipping plane
    vr.copy(pb).sub(pa).normalize();
    vu.copy(pc).sub(pa).normalize();
    vn.crossVectors(vr, vu).normalize();
    va.copy(pa).sub(pe); // from pe to pa
    vb.copy(pb).sub(pe); // from pe to pb
    vc.copy(pc).sub(pe); // from pe to pc
    const double d = -va.dot(vn);        // distance from eye to screen
    const double l = vr.dot(va) * n / d; // distance to left screen edge
    const double r = vr.dot(vb) * n / d; // distance to right screen edge
    const double b = vu.dot(va) * n / d; // distance to bottom screen edge
    const double t = vu.dot(vc) * n / d; // distance to top screen edge
    // Set the camera rotation to match the focal plane to the corners' plane
    quat.setFromUnitVectors(vec.set(0, 1, 0), vu);
    camera.quaternion.setFromUnitVectors(vec.set(0, 0, 1).applyQuaternion(quat), vn).multiply(quat);
    // Set the off-axis projection matrix to match the corners
    camera.projectionMatrix.set(2.0 * n / (r - l), 0.0, (r + l) / (r - l), 0.0, 0.0, 2.0 * n / (t - b),
        (t + b) / (t - b), 0.0, 0.0, 0.0, (f + n) / (n - f), 2.0 * f * n / (n - f), 0.0, 0.0, -1.0, 0.0);
    camera.projectionMatrixInverse.copy(camera.projectionMatrix).invert();
    // FoV estimation to fix frustum culling
    if (estimateViewFrustum) {
        // Set fieldOfView to a conservative estimate
        // to make frustum tall/wide enough to encompass it
        camera.fov = RAD2DEG / MathUtils::min(1.0, camera.aspect) *
            JSNumberMath::atan((vec.copy(pb).sub(pa).length() + (vec.copy(pc).sub(pa).length())) / va.length());
    }
}