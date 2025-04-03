#ifndef MATH_UTILS_H
#define MATH_UTILS_H

#include "common/BasicType.h"

namespace MathUtils
{
    std::string generateUUID();
    HIGH_PRECISION clamp(HIGH_PRECISION value, HIGH_PRECISION min, HIGH_PRECISION max);
    int randInt(int low, int high);
}

#endif