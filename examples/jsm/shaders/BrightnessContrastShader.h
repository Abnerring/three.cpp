#pragma once
#include "core/JSModuleObject.h"
#include "core/JSValue.h"

// Native port of the pinned examples/jsm/shaders/BrightnessContrastShader.js (see upstream.json).
// The module exports the BrightnessContrastShader definition.
// `nativeBrightnessContrastShader()` returns the module namespace object whose keys are the
// upstream export names in module namespace order.
namespace BrightnessContrastShader {
JSValue nativeBrightnessContrastShader();
} // namespace BrightnessContrastShader

inline constexpr JSModuleObject BrightnessContrastShaderModule(BrightnessContrastShader::nativeBrightnessContrastShader);
