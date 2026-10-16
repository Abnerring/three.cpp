#pragma once
#include "core/JSModuleObject.h"
#include "core/JSValue.h"

// Native port of the pinned examples/jsm/shaders/AfterimageShader.js (see upstream.json).
// The module exports the AfterimageShader definition.
// `nativeAfterimageShader()` returns the module namespace object whose keys are the
// upstream export names in module namespace order.
namespace AfterimageShader {
JSValue nativeAfterimageShader();
} // namespace AfterimageShader

inline constexpr JSModuleObject AfterimageShaderModule(AfterimageShader::nativeAfterimageShader);
