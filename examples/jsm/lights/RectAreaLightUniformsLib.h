#pragma once
#include "core/JSModuleObject.h"
#include "core/JSValue.h"

// Native port of the pinned examples/jsm/lights/RectAreaLightUniformsLib.js.
//
// The upstream module is one class with a static `init()` that first calls
// `RectAreaLightTexturesLib.init()` and then copies the four data textures of that class into
// `UniformsLib.LTC_FLOAT_1`, `LTC_FLOAT_2`, `LTC_HALF_1` and `LTC_HALF_2` (in that order).
// The destination keys are added at runtime, so the shared native UniformsLib needs no
// change: the module assigns the four properties exactly like the pinned source.
namespace RectAreaLightUniformsLib {

// The class object with its single static `init` method.
JSValue rectAreaLightUniformsLib();

// The module namespace object with the single export, in upstream export order.
JSValue nativeRectAreaLightUniformsLib();

// Upstream `RectAreaLightUniformsLib.init()`. The three-argument form matches
// JSValue::NativeFunction and is the module's static method; the receiver is unused, exactly
// like the static method of the pinned class.
JSValue init(const JSValue& self, const std::vector<JSValue>& args, const std::vector<JSValue>& captures);
JSValue init();

} // namespace RectAreaLightUniformsLib

inline constexpr JSModuleObject RectAreaLightUniformsLibModule(
    RectAreaLightUniformsLib::nativeRectAreaLightUniformsLib);
