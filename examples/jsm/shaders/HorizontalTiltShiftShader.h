#pragma once
#include "core/JSModuleObject.h"
#include "core/JSValue.h"

// Native port of the pinned examples/jsm/shaders/HorizontalTiltShiftShader.js (see upstream.json).
// The module exports the HorizontalTiltShiftShader definition.
// `nativeHorizontalTiltShiftShader()` returns the module namespace object whose keys are the
// upstream export names in module namespace order.
namespace HorizontalTiltShiftShader {
JSValue nativeHorizontalTiltShiftShader();
} // namespace HorizontalTiltShiftShader

inline constexpr JSModuleObject HorizontalTiltShiftShaderModule(HorizontalTiltShiftShader::nativeHorizontalTiltShiftShader);
