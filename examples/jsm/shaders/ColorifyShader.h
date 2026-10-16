#pragma once
#include "core/JSModuleObject.h"
#include "core/JSValue.h"

// Native port of the pinned examples/jsm/shaders/ColorifyShader.js (see upstream.json).
// The module exports the ColorifyShader definition.
// `nativeColorifyShader()` returns the module namespace object whose keys are the
// upstream export names in module namespace order.
namespace ColorifyShader {
JSValue nativeColorifyShader();
} // namespace ColorifyShader

inline constexpr JSModuleObject ColorifyShaderModule(ColorifyShader::nativeColorifyShader);
