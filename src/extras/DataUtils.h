#pragma once
#include <cstdint>

std::uint16_t toHalfFloat(double value);
double fromHalfFloat(double value);

class DataUtils {
  public:
    static std::uint16_t toHalfFloat(double value) { return ::toHalfFloat(value); }
    static double fromHalfFloat(double value) { return ::fromHalfFloat(value); }
};
