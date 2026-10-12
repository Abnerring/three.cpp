#include "math/SphericalHarmonics3.h"
#include "core/JSValue.h"
#include <limits>

SphericalHarmonics3& SphericalHarmonics3::set(const std::array<Vector3, 9>& values) {
    for (std::size_t i = 0; i < 9; ++i) coefficients[i].copy(values[i]);
    return *this;
}
SphericalHarmonics3& SphericalHarmonics3::set(const std::vector<Vector3>& values) {
    for (std::size_t i = 0; i < 9; ++i) {
        if (i >= values.size()) throw JSTypeError("Cannot read properties of undefined (reading 'x')");
        coefficients[i].copy(values[i]);
    }
    return *this;
}
SphericalHarmonics3& SphericalHarmonics3::zero() {
    for (auto& coefficient : coefficients) coefficient.set(0, 0, 0);
    return *this;
}
Vector3& SphericalHarmonics3::getAt(const Vector3& normal, Vector3& target) const {
    const double x = normal.x, y = normal.y, z = normal.z;
    target.copy(coefficients[0]).multiplyScalar(0.282095);
    target.addScaledVector(coefficients[1], 0.488603 * y);
    target.addScaledVector(coefficients[2], 0.488603 * z);
    target.addScaledVector(coefficients[3], 0.488603 * x);
    target.addScaledVector(coefficients[4], 1.092548 * (x * y));
    target.addScaledVector(coefficients[5], 1.092548 * (y * z));
    target.addScaledVector(coefficients[6], 0.315392 * (3.0 * z * z - 1.0));
    target.addScaledVector(coefficients[7], 1.092548 * (x * z));
    target.addScaledVector(coefficients[8], 0.546274 * (x * x - y * y));
    return target;
}
Vector3& SphericalHarmonics3::getIrradianceAt(const Vector3& normal, Vector3& target) const {
    const double x = normal.x, y = normal.y, z = normal.z;
    target.copy(coefficients[0]).multiplyScalar(0.886227);
    target.addScaledVector(coefficients[1], 2.0 * 0.511664 * y);
    target.addScaledVector(coefficients[2], 2.0 * 0.511664 * z);
    target.addScaledVector(coefficients[3], 2.0 * 0.511664 * x);
    target.addScaledVector(coefficients[4], 2.0 * 0.429043 * x * y);
    target.addScaledVector(coefficients[5], 2.0 * 0.429043 * y * z);
    target.addScaledVector(coefficients[6], 0.743125 * z * z - 0.247708);
    target.addScaledVector(coefficients[7], 2.0 * 0.429043 * x * z);
    target.addScaledVector(coefficients[8], 0.429043 * (x * x - y * y));
    return target;
}
SphericalHarmonics3& SphericalHarmonics3::add(const SphericalHarmonics3& sh) {
    for (std::size_t i = 0; i < 9; ++i) coefficients[i].add(sh.coefficients[i]);
    return *this;
}
SphericalHarmonics3& SphericalHarmonics3::addScaledSH(const SphericalHarmonics3& sh, double s) {
    for (std::size_t i = 0; i < 9; ++i) coefficients[i].addScaledVector(sh.coefficients[i], s);
    return *this;
}
SphericalHarmonics3& SphericalHarmonics3::scale(double s) {
    for (auto& coefficient : coefficients) coefficient.multiplyScalar(s);
    return *this;
}
SphericalHarmonics3& SphericalHarmonics3::lerp(const SphericalHarmonics3& sh, double alpha) {
    for (std::size_t i = 0; i < 9; ++i) coefficients[i].lerp(sh.coefficients[i], alpha);
    return *this;
}
bool SphericalHarmonics3::equals(const SphericalHarmonics3& sh) const {
    for (std::size_t i = 0; i < 9; ++i) if (!coefficients[i].equals(sh.coefficients[i])) return false;
    return true;
}
SphericalHarmonics3& SphericalHarmonics3::copy(const SphericalHarmonics3& sh) { return set(sh.coefficients); }
SphericalHarmonics3 SphericalHarmonics3::clone() const { SphericalHarmonics3 result; result.copy(*this); return result; }
SphericalHarmonics3& SphericalHarmonics3::fromArray(const std::vector<double>& array, std::size_t offset) {
    const auto number = [&](std::size_t index) { return index < array.size() ? array[index] : std::numeric_limits<double>::quiet_NaN(); };
    for (std::size_t i = 0; i < 9; ++i) coefficients[i].set(number(offset + i * 3), number(offset + i * 3 + 1), number(offset + i * 3 + 2));
    return *this;
}
std::vector<double>& SphericalHarmonics3::toArray(std::vector<double>& array, std::size_t offset) const {
    for (std::size_t i = 0; i < 9; ++i) coefficients[i].toArray(array, offset + i * 3);
    return array;
}
std::vector<double> SphericalHarmonics3::toArray() const { std::vector<double> result; return toArray(result); }
void SphericalHarmonics3::getBasisAt(const Vector3& normal, std::vector<double>& shBasis) {
    const double x = normal.x, y = normal.y, z = normal.z;
    if (shBasis.size() < 9) shBasis.resize(9);
    shBasis[0] = 0.282095;
    shBasis[1] = 0.488603 * y;
    shBasis[2] = 0.488603 * z;
    shBasis[3] = 0.488603 * x;
    shBasis[4] = 1.092548 * x * y;
    shBasis[5] = 1.092548 * y * z;
    shBasis[6] = 0.315392 * (3 * z * z - 1);
    shBasis[7] = 1.092548 * x * z;
    shBasis[8] = 0.546274 * (x * x - y * y);
}
