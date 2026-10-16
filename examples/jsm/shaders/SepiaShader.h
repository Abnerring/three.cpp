#pragma once
#include "core/JSModuleObject.h"
#include "core/JSValue.h"

// Native port of the pinned examples/jsm/shaders/SepiaShader.js (see upstream.json).
// The module exports the SepiaShader definition.
// `nativeSepiaShader()` returns the module namespace object whose keys are the
// upstream export names in module namespace order.
namespace SepiaShader {
JSValue nativeSepiaShader();
} // namespace SepiaShader

inline constexpr JSModuleObject SepiaShaderModule(SepiaShader::nativeSepiaShader);
