#pragma once
#include "core/JSModuleObject.h"
#include "core/JSValue.h"

// Native port of the pinned examples/jsm/shaders/BlendShader.js (see upstream.json).
// The module exports the BlendShader definition.
// `nativeBlendShader()` returns the module namespace object whose keys are the
// upstream export names in module namespace order.
namespace BlendShader {
JSValue nativeBlendShader();
} // namespace BlendShader

inline constexpr JSModuleObject BlendShaderModule(BlendShader::nativeBlendShader);
