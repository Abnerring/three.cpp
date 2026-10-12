#pragma once
#include "math/Vector3.h"
#include <array>
#include <vector>

class SphericalHarmonics3 {
  public:
    const bool isSphericalHarmonics3 = true;
    std::array<Vector3, 9> coefficients;
    SphericalHarmonics3& set(const std::array<Vector3, 9>& values);
    SphericalHarmonics3& set(const std::vector<Vector3>& values);
    SphericalHarmonics3& zero();
    Vector3& getAt(const Vector3& normal, Vector3& target) const;
    Vector3& getIrradianceAt(const Vector3& normal, Vector3& target) const;
    SphericalHarmonics3& add(const SphericalHarmonics3& sh);
    SphericalHarmonics3& addScaledSH(const SphericalHarmonics3& sh, double scale);
    SphericalHarmonics3& scale(double scale);
    SphericalHarmonics3& lerp(const SphericalHarmonics3& sh, double alpha);
    bool equals(const SphericalHarmonics3& sh) const;
    SphericalHarmonics3& copy(const SphericalHarmonics3& sh);
    SphericalHarmonics3 clone() const;
    SphericalHarmonics3& fromArray(const std::vector<double>& array, std::size_t offset = 0);
    std::vector<double>& toArray(std::vector<double>& array, std::size_t offset = 0) const;
    std::vector<double> toArray() const;
    static void getBasisAt(const Vector3& normal, std::vector<double>& shBasis);
};
