#pragma once
#include "core/JSModuleObject.h"
#include "core/JSValue.h"

// Native port of the pinned examples/jsm/shaders/DepthLimitedBlurShader.js (see upstream.json).
// The module exports DepthLimitedBlurShader and the BlurShaderUtils helper object.
// `nativeDepthLimitedBlurShader()` returns the module namespace object whose keys are the
// upstream export names in module namespace order.
namespace DepthLimitedBlurShader {
JSValue nativeDepthLimitedBlurShader();
} // namespace DepthLimitedBlurShader

inline constexpr JSModuleObject DepthLimitedBlurShaderModule(DepthLimitedBlurShader::nativeDepthLimitedBlurShader);
