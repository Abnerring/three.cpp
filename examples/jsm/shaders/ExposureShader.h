#pragma once
#include "core/JSModuleObject.h"
#include "core/JSValue.h"

// Native port of the pinned examples/jsm/shaders/ExposureShader.js (see upstream.json).
// The module exports the ExposureShader definition.
// `nativeExposureShader()` returns the module namespace object whose keys are the
// upstream export names in module namespace order.
namespace ExposureShader {
JSValue nativeExposureShader();
} // namespace ExposureShader

inline constexpr JSModuleObject ExposureShaderModule(ExposureShader::nativeExposureShader);
