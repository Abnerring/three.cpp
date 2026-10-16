#pragma once
#include "core/JSModuleObject.h"
#include "core/JSValue.h"

// Native port of the pinned examples/jsm/shaders/FXAAShader.js (see upstream.json).
// The module exports the FXAAShader definition.
// `nativeFXAAShader()` returns the module namespace object whose keys are the
// upstream export names in module namespace order.
namespace FXAAShader {
JSValue nativeFXAAShader();
} // namespace FXAAShader

inline constexpr JSModuleObject FXAAShaderModule(FXAAShader::nativeFXAAShader);
