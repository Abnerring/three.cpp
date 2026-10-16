#pragma once
#include "core/JSModuleObject.h"
#include "core/JSValue.h"

// Native port of the pinned examples/jsm/shaders/HalftoneShader.js (see upstream.json).
// The module exports the HalftoneShader definition.
// `nativeHalftoneShader()` returns the module namespace object whose keys are the
// upstream export names in module namespace order.
namespace HalftoneShader {
JSValue nativeHalftoneShader();
} // namespace HalftoneShader

inline constexpr JSModuleObject HalftoneShaderModule(HalftoneShader::nativeHalftoneShader);
