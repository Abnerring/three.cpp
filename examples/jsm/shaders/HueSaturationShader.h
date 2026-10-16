#pragma once
#include "core/JSModuleObject.h"
#include "core/JSValue.h"

// Native port of the pinned examples/jsm/shaders/HueSaturationShader.js (see upstream.json).
// The module exports the HueSaturationShader definition.
// `nativeHueSaturationShader()` returns the module namespace object whose keys are the
// upstream export names in module namespace order.
namespace HueSaturationShader {
JSValue nativeHueSaturationShader();
} // namespace HueSaturationShader

inline constexpr JSModuleObject HueSaturationShaderModule(HueSaturationShader::nativeHueSaturationShader);
