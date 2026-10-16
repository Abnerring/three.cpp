#pragma once
#include "core/JSModuleObject.h"
#include "core/JSValue.h"

// Native port of the pinned examples/jsm/shaders/NormalMapShader.js (see upstream.json).
// The module exports the NormalMapShader definition.
// `nativeNormalMapShader()` returns the module namespace object whose keys are the
// upstream export names in module namespace order.
namespace NormalMapShader {
JSValue nativeNormalMapShader();
} // namespace NormalMapShader

inline constexpr JSModuleObject NormalMapShaderModule(NormalMapShader::nativeNormalMapShader);
