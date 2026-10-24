#pragma once
#include <array>
#include <span>
#include <vector>
#include "core/JSValue.h"

// Managed constructor preserves the source's arbitrary element values and live
// array/prototype operations; the numeric facade below remains available.
JSValue nativeMatrix2Constructor();

class Matrix2 {
  public:
    static constexpr bool isMatrix2 = true;
    Matrix2() = default;
    Matrix2(double n11, double n12, double n21, double n22);
    std::span<double, 4> elements();
    std::span<const double, 4> elements() const;
    Matrix2& identity();
    Matrix2& set(double n11, double n12, double n21, double n22);
    Matrix2& fromArray(const std::vector<double>& array, std::size_t offset = 0);

  private:
    std::array<double, 4> elements_{1, 0, 0, 1};
};
