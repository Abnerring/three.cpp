#pragma once
#include "core/JSModuleObject.h"
#include "core/JSValue.h"

// Native port of the pinned examples/jsm/shaders/BleachBypassShader.js (see upstream.json).
// The module exports the BleachBypassShader definition.
// `nativeBleachBypassShader()` returns the module namespace object whose keys are the
// upstream export names in module namespace order.
namespace BleachBypassShader {
JSValue nativeBleachBypassShader();
} // namespace BleachBypassShader

inline constexpr JSModuleObject BleachBypassShaderModule(BleachBypassShader::nativeBleachBypassShader);
