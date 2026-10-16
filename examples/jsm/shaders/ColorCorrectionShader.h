#pragma once
#include "core/JSModuleObject.h"
#include "core/JSValue.h"

// Native port of the pinned examples/jsm/shaders/ColorCorrectionShader.js (see upstream.json).
// The module exports the ColorCorrectionShader definition.
// `nativeColorCorrectionShader()` returns the module namespace object whose keys are the
// upstream export names in module namespace order.
namespace ColorCorrectionShader {
JSValue nativeColorCorrectionShader();
} // namespace ColorCorrectionShader

inline constexpr JSModuleObject ColorCorrectionShaderModule(ColorCorrectionShader::nativeColorCorrectionShader);
