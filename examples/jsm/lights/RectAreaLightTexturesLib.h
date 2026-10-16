#pragma once
#include "core/JSModuleObject.h"
#include "core/JSValue.h"

// Native port of the pinned examples/jsm/lights/RectAreaLightTexturesLib.js.
//
// The upstream module is one class with a static `init()` that builds four 64x64 RGBA data
// textures from the two 64x64x4 LTC BRDF tables (`LTC_MAT_1` / `LTC_MAT_2`) and assigns them
// to the four static fields of the class. The tables are the verbatim upstream number
// literals, stored as binary64 constants in the .cpp, so the `new Float32Array( LTC_MAT_1 )`
// and `DataUtils.toHalfFloat` conversions run on exactly the same inputs as upstream.
//
// The class object and its module namespace object are built once per realm (the same idiom
// as ColorSpaces), so `RectAreaLightTexturesLib.init()` keeps its receiver semantics and
// every call still builds fresh textures, exactly like the pinned module.
namespace RectAreaLightTexturesLib {

// The class object with the static `init` method and the four static texture fields, which
// start as `null` like the pinned module.
JSValue rectAreaLightTexturesLib();

// The module namespace object with the single export, in upstream export order.
JSValue nativeRectAreaLightTexturesLib();

// Upstream `RectAreaLightTexturesLib.init()`: builds the four data textures, assigns them to
// the receiver (`this`) in upstream order and returns the receiver. The three-argument form
// matches JSValue::NativeFunction and is the module's static method.
JSValue init(const JSValue& self, const std::vector<JSValue>& args, const std::vector<JSValue>& captures);
// The same call with the class object as the receiver, for native callers.
JSValue init();

} // namespace RectAreaLightTexturesLib

inline constexpr JSModuleObject RectAreaLightTexturesLibModule(
    RectAreaLightTexturesLib::nativeRectAreaLightTexturesLib);
