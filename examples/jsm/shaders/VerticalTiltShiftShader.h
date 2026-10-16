#pragma once
#include "core/JSModuleObject.h"
#include "core/JSValue.h"

// Native port of the pinned examples/jsm/shaders/VerticalTiltShiftShader.js (see upstream.json).
// The module exports the VerticalTiltShiftShader definition.
// `nativeVerticalTiltShiftShader()` returns the module namespace object whose keys are the
// upstream export names in module namespace order.
namespace VerticalTiltShiftShader {
JSValue nativeVerticalTiltShiftShader();
} // namespace VerticalTiltShiftShader

inline constexpr JSModuleObject VerticalTiltShiftShaderModule(VerticalTiltShiftShader::nativeVerticalTiltShiftShader);
