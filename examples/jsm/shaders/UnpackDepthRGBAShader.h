#pragma once
#include "core/JSModuleObject.h"
#include "core/JSValue.h"

// Native port of the pinned examples/jsm/shaders/UnpackDepthRGBAShader.js (see upstream.json).
// The module exports the UnpackDepthRGBAShader definition.
// `nativeUnpackDepthRGBAShader()` returns the module namespace object whose keys are the
// upstream export names in module namespace order.
namespace UnpackDepthRGBAShader {
JSValue nativeUnpackDepthRGBAShader();
} // namespace UnpackDepthRGBAShader

inline constexpr JSModuleObject UnpackDepthRGBAShaderModule(UnpackDepthRGBAShader::nativeUnpackDepthRGBAShader);
