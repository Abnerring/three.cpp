#include "math/Matrix2.h"
#include <limits>

Matrix2::Matrix2(double n11, double n12, double n21, double n22) {
    set(n11, n12, n21, n22);
}
std::span<double, 4> Matrix2::elements() {
    return elements_;
}
std::span<const double, 4> Matrix2::elements() const {
    return elements_;
}
Matrix2& Matrix2::identity() {
    return set(1, 0, 0, 1);
}
Matrix2& Matrix2::set(double n11, double n12, double n21, double n22) {
    elements_[0] = n11;
    elements_[2] = n12;
    elements_[1] = n21;
    elements_[3] = n22;
    return *this;
}
Matrix2& Matrix2::fromArray(const std::vector<double>& array, std::size_t offset) {
    for (std::size_t i = 0; i < 4; ++i)
        elements_[i] = offset < array.size() && i < array.size() - offset
                           ? array[offset + i]
                           : std::numeric_limits<double>::quiet_NaN();
    return *this;
}
