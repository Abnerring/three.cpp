#pragma once
#include "core/JSModuleObject.h"
#include "core/JSValue.h"

// Native port of the pinned examples/jsm/shaders/FocusShader.js (see upstream.json).
// The module exports the FocusShader definition.
// `nativeFocusShader()` returns the module namespace object whose keys are the
// upstream export names in module namespace order.
namespace FocusShader {
JSValue nativeFocusShader();
} // namespace FocusShader

inline constexpr JSModuleObject FocusShaderModule(FocusShader::nativeFocusShader);
