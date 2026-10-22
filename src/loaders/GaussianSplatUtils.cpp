#include "loaders/GaussianSplatUtils.h"

#include "math/Matrix3.h"
#include "math/Matrix4.h"
#include "math/Quaternion.h"
#include "math/Vector3.h"

#include <cmath>

namespace GaussianSplatUtils {
namespace {
// Upstream shares these across calls, so the same scratch objects are reused here.
thread_local Matrix3 covarianceMatrix;
thread_local Matrix3 covarianceMatrixTranspose;
thread_local Matrix4 rotationScaleMatrix;
thread_local Quaternion quaternion;
thread_local Vector3 scale;
thread_local Vector3 zero;
}  // namespace

double sigmoid(double value) { return 1.0 / (1.0 + std::exp(-value)); }

double sh0ToLinear(double coefficient) { return coefficient * SH_C0 + 0.5; }

double linearToSH0(double color) { return (color - 0.5) / SH_C0; }

void writeCovariance(float* target, std::size_t offset, double sx, double sy, double sz,
                     double qx, double qy, double qz, double qw) {
    quaternion.set(qx, qy, qz, qw);
    quaternion.normalize();
    scale.set(sx, sy, sz);
    rotationScaleMatrix.compose(zero, quaternion, scale);

    covarianceMatrix.setFromMatrix4(rotationScaleMatrix);
    covarianceMatrixTranspose.copy(covarianceMatrix);
    covarianceMatrixTranspose.transpose();
    covarianceMatrix.multiply(covarianceMatrixTranspose);

    const auto& elements = covarianceMatrix.elements();
    // Only the upper triangle is stored, in the order the shader reads it.
    target[offset + 0] = static_cast<float>(elements[0]);
    target[offset + 1] = static_cast<float>(elements[3]);
    target[offset + 2] = static_cast<float>(elements[6]);
    target[offset + 3] = static_cast<float>(elements[4]);
    target[offset + 4] = static_cast<float>(elements[7]);
    target[offset + 5] = static_cast<float>(elements[8]);
}

void writeColorBytesFromSH0(std::uint8_t* target, std::size_t offset, double r, double g,
                            double b, double a) {
    // A clamped byte target rounds and clamps on assignment, which the C++ version does here.
    const auto clampByte = [](double value) {
        const double rounded = std::round(value);
        if (rounded < 0.0) return std::uint8_t(0);
        if (rounded > 255.0) return std::uint8_t(255);
        return static_cast<std::uint8_t>(rounded);
    };
    target[offset + 0] = clampByte(sh0ToLinear(r) * 255.0);
    target[offset + 1] = clampByte(sh0ToLinear(g) * 255.0);
    target[offset + 2] = clampByte(sh0ToLinear(b) * 255.0);
    target[offset + 3] = clampByte(a * 255.0);
}

}  // namespace GaussianSplatUtils
