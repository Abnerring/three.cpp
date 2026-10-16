#pragma once
#include "core/JSModuleObject.h"
#include "core/JSValue.h"

// Native port of the pinned examples/jsm/shaders/BasicShader.js (see upstream.json).
// The module exports the BasicShader definition.
// `nativeBasicShader()` returns the module namespace object whose keys are the
// upstream export names in module namespace order.
namespace BasicShader {
JSValue nativeBasicShader();
} // namespace BasicShader

inline constexpr JSModuleObject BasicShaderModule(BasicShader::nativeBasicShader);
