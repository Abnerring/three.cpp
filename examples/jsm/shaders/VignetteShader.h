#pragma once
#include "core/JSModuleObject.h"
#include "core/JSValue.h"

// Native port of the pinned examples/jsm/shaders/VignetteShader.js (see upstream.json).
// The module exports the VignetteShader definition.
// `nativeVignetteShader()` returns the module namespace object whose keys are the
// upstream export names in module namespace order.
namespace VignetteShader {
JSValue nativeVignetteShader();
} // namespace VignetteShader

inline constexpr JSModuleObject VignetteShaderModule(VignetteShader::nativeVignetteShader);
