#pragma once
#include "core/JSModuleObject.h"
#include "core/JSValue.h"

// Native port of the pinned examples/jsm/shaders/ToonShader.js (see upstream.json).
// The module exports the ToonShader1, ToonShader2, ToonShaderDotted, ToonShaderHatching definitions.
// `nativeToonShader()` returns the module namespace object whose keys are the
// upstream export names in module namespace order.
namespace ToonShader {
JSValue nativeToonShader();
} // namespace ToonShader

inline constexpr JSModuleObject ToonShaderModule(ToonShader::nativeToonShader);
