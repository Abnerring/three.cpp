#pragma once
#include "core/JSModuleObject.h"
#include "core/JSValue.h"

// Native port of the pinned examples/jsm/shaders/VolumeShader.js (see upstream.json).
// The module exports the VolumeRenderShader1 definition.
// `nativeVolumeShader()` returns the module namespace object whose keys are the
// upstream export names in module namespace order.
namespace VolumeShader {
JSValue nativeVolumeShader();
} // namespace VolumeShader

inline constexpr JSModuleObject VolumeShaderModule(VolumeShader::nativeVolumeShader);
