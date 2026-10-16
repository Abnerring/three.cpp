#pragma once
#include "core/JSModuleObject.h"
#include "core/JSValue.h"

// Native port of the pinned examples/jsm/shaders/DigitalGlitch.js (see upstream.json).
// The module exports the DigitalGlitch definition.
// `nativeDigitalGlitch()` returns the module namespace object whose keys are the
// upstream export names in module namespace order.
namespace DigitalGlitch {
JSValue nativeDigitalGlitch();
} // namespace DigitalGlitch

inline constexpr JSModuleObject DigitalGlitchModule(DigitalGlitch::nativeDigitalGlitch);
