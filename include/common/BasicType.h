#ifndef BASICTYPE
#define BASICTYPE

#include <numbers>
#include <random> 

#if defined(__x86_64__) || defined(__i386__)
  #define HIGH_PRECISION long double
#else
  #define HIGH_PRECISION double
#endif

#define MATH_PI std::numbers::pi

static thread_local std::mt19937 RANDOM_ENGINE(std::random_device{}());
static std::uniform_real_distribution<double> DIST(0.0, 1.0);

#endif