#pragma once
#include "core/JSModuleObject.h"
#include "core/JSValue.h"

// Native port of the pinned examples/jsm/shaders/FreiChenShader.js (see upstream.json).
// The module exports the FreiChenShader definition.
// `nativeFreiChenShader()` returns the module namespace object whose keys are the
// upstream export names in module namespace order.
namespace FreiChenShader {
JSValue nativeFreiChenShader();
} // namespace FreiChenShader

inline constexpr JSModuleObject FreiChenShaderModule(FreiChenShader::nativeFreiChenShader);
