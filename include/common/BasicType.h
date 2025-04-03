#ifndef BASICTYPE
#define BASICTYPE

#if defined(__x86_64__) || defined(__i386__)
  #define HIGH_PRECISION long double
#else
  #define HIGH_PRECISION double
#endif

#endif