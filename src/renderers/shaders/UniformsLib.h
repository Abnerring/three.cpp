#pragma once
#include "core/JSModuleObject.h"
// Mutable shared definitions in the current native realm. The facade itself
// owns no realm; explicit JSValue values retain the resolved object and heap.
JSValue nativeUniformsLib();
inline constexpr JSModuleObject UniformsLib(nativeUniformsLib);
