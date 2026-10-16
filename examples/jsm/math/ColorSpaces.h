#pragma once
#include "core/JSModuleObject.h"
#include "core/JSValue.h"

// Native port of the pinned examples/jsm/math/ColorSpaces.js.
//
// The upstream module exports four colour space names and four space implementations as
// module-level objects. The C++ port builds the same module namespace object once per
// realm (see UniformsLib for the same idiom) and exposes the names as constants for native
// callers. `transfer` keeps the representation the native ColorManagement uses for its
// transfer functions (the SRGBTransfer / LinearTransfer constants), and the matrices are
// real native Matrix3 wrappers, so `toXYZ`/`fromXYZ` keep their identity when a space is
// spread, exactly like the pinned module.
namespace ColorSpaces {

inline constexpr auto DisplayP3ColorSpace = "display-p3";
inline constexpr auto LinearDisplayP3ColorSpace = "display-p3-linear";
inline constexpr auto LinearRec2020ColorSpace = "rec2020-linear";
inline constexpr auto ExtendedSRGBColorSpace = "extended-srgb";

// The four exported implementations; each call returns the same object of the realm.
JSValue displayP3ColorSpaceImpl();
JSValue linearDisplayP3ColorSpaceImpl();
JSValue linearRec2020ColorSpaceImpl();
JSValue extendedSRGBColorSpaceImpl();

// The module namespace object with the eight exports in their upstream export order.
JSValue nativeColorSpaces();

} // namespace ColorSpaces

inline constexpr JSModuleObject ColorSpacesModule(ColorSpaces::nativeColorSpaces);
