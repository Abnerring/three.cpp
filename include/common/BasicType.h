#ifndef BASIC_TYPE_H
#define BASIC_TYPE_H

#include <numbers>
#include <random> 

#if defined(__x86_64__) || defined(__i386__)
  #define HIGH_PRECISION long double
#else
  #define HIGH_PRECISION double
#endif

#define MATH_PI std::numbers::pi

static std::random_device rd;
static std::mt19937 RANDOM_ENGINE(rd());
static std::uniform_int_distribution<uint32_t> DIST_UINT(0, 0xFFFFFFFF);
static std::uniform_real_distribution<HIGH_PRECISION> DIST_DOUBLE(0.0, 1.0);

#endif