#pragma once
#include <cstdint>
#include <span>
#include <string>
#include <vector>

// Port of the math half of examples/jsm/utils/GaussianSplatUtils.js at the pinned revision.
// The geometry building half needs the attribute plumbing and is left to the caller.
namespace GaussianSplatUtils {

inline constexpr double SH_C0 = 0.2820947917738781;
// Component counts and packed word counts per spherical harmonics band.
inline constexpr int SH_BAND_COMPONENTS[4] = {0, 9, 15, 21};
inline constexpr int SH_BAND_WORDS[4] = {0, 3, 4, 6};

double sigmoid(double value);
double sh0ToLinear(double coefficient);
double linearToSH0(double color);

// Writes the six covariance entries derived from a scale and a quaternion into target at
// offset, matching the upper triangle the shader expects.
void writeCovariance(float* target, std::size_t offset, double sx, double sy, double sz,
                     double qx, double qy, double qz, double qw);

// Writes four clamped colour bytes from spherical harmonics zero order coefficients.
void writeColorBytesFromSH0(std::uint8_t* target, std::size_t offset, double r, double g,
                            double b, double a);

}  // namespace GaussianSplatUtils
