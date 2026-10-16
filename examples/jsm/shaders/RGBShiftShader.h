#pragma once
#include "core/JSModuleObject.h"
#include "core/JSValue.h"

// Native port of the pinned examples/jsm/shaders/RGBShiftShader.js (see upstream.json).
// The module exports the RGBShiftShader definition.
// `nativeRGBShiftShader()` returns the module namespace object whose keys are the
// upstream export names in module namespace order.
namespace RGBShiftShader {
JSValue nativeRGBShiftShader();
} // namespace RGBShiftShader

inline constexpr JSModuleObject RGBShiftShaderModule(RGBShiftShader::nativeRGBShiftShader);
