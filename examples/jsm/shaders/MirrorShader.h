#pragma once
#include "core/JSModuleObject.h"
#include "core/JSValue.h"

// Native port of the pinned examples/jsm/shaders/MirrorShader.js (see upstream.json).
// The module exports the MirrorShader definition.
// `nativeMirrorShader()` returns the module namespace object whose keys are the
// upstream export names in module namespace order.
namespace MirrorShader {
JSValue nativeMirrorShader();
} // namespace MirrorShader

inline constexpr JSModuleObject MirrorShaderModule(MirrorShader::nativeMirrorShader);
