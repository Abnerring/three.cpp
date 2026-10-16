#include "math/ColorSpaces.h"
#include "constants.h"
#include "core/NativeObject.h"
#include "math/ColorManagement.h"
#include <initializer_list>
#include <string>

namespace {
JSPropertyDescriptor data(const JSValue& value) {
    JSPropertyDescriptor descriptor;
    descriptor.value = value;
    descriptor.writable = descriptor.enumerable = descriptor.configurable = true;
    return descriptor;
}
void literal(JSValue object, const char* key, const JSValue& value) {
    object.defineProperty(key, data(value));
}
JSValue array(std::initializer_list<double> values) {
    auto result = JSValue::array();
    unsigned index = 0;
    for (const auto value : values) result.defineProperty(std::to_string(index++), data(value));
    return result;
}
// The same construction the native ColorManagement uses for its space matrices: a real
// Matrix3 wrapper whose elements come from `set`.
JSValue matrix(std::initializer_list<double> values) {
    auto result = JSValue::object(nativeMatrix3Prototype());
    result.set("elements", array({1, 0, 0, 0, 1, 0, 0, 0, 1}));
    std::vector<JSValue> arguments;
    for (const auto value : values) arguments.emplace_back(value);
    const auto set = result.get("set");
    set.call(result, arguments);
    return result;
}
// The transfer functions of the spaces the implementations reference. ColorManagement
// compares `transfer` by identity, so the values come from the very same module state.
JSValue srgbTransfer() {
    return nativeColorManagement().get("spaces").get(SRGBColorSpace).get("transfer");
}
JSValue linearTransfer() {
    return nativeColorManagement().get("spaces").get(LinearSRGBColorSpace).get("transfer");
}
JSValue config(const char* key, const JSValue& value) {
    auto result = JSValue::object();
    literal(result, key, value);
    return result;
}
}

namespace ColorSpaces {

JSValue displayP3ColorSpaceImpl() { return nativeColorSpaces().get("DisplayP3ColorSpaceImpl"); }
JSValue linearDisplayP3ColorSpaceImpl() { return nativeColorSpaces().get("LinearDisplayP3ColorSpaceImpl"); }
JSValue linearRec2020ColorSpaceImpl() { return nativeColorSpaces().get("LinearRec2020ColorSpaceImpl"); }
JSValue extendedSRGBColorSpaceImpl() { return nativeColorSpaces().get("ExtendedSRGBColorSpaceImpl"); }

JSValue nativeColorSpaces() {
    static thread_local JSWeakValue cache;
    if (const auto existing = cache.lock(); existing && existing->isObject()) return *existing;
    static thread_local bool initializing = false;
    JSModuleInitializationScope initialization(initializing, "ColorSpaces");

    // primaries, white point and luminance coefficients of the two extended families.
    const auto p3Primaries = array({0.680, 0.320, 0.265, 0.690, 0.150, 0.060});
    const auto p3Coefficients = array({0.2289, 0.6917, 0.0793});
    const auto rec2020Primaries = array({0.708, 0.292, 0.170, 0.797, 0.131, 0.046});
    const auto rec2020Coefficients = array({0.2627, 0.6780, 0.0593});
    const auto d65 = array({0.3127, 0.3290});
    // Both families share the Display P3 / Rec. 2020 matrices of the pinned module.
    const auto p3ToXYZ = matrix({0.4865709, 0.2656677, 0.1982173, 0.2289746, 0.6917385, 0.0792869, 0.0000000,
        0.0451134, 1.0439444});
    const auto p3FromXYZ = matrix({2.4934969, -0.9313836, -0.4027108, -0.8294890, 1.7626641, 0.0236247, 0.0358458,
        -0.0761724, 0.9568845});
    const auto rec2020ToXYZ = matrix({0.6369580, 0.1446169, 0.1688810, 0.2627002, 0.6779981, 0.0593017, 0.0000000,
        0.0280727, 1.0609851});
    const auto rec2020FromXYZ = matrix({1.7166512, -0.3556708, -0.2533663, -0.6666844, 1.6164812, 0.0157685,
        0.0176399, -0.0427706, 0.9421031});
    // These are module-level source globals, independent of replacement spaces.
    p3ToXYZ.retainForHeapLifetime();
    p3FromXYZ.retainForHeapLifetime();
    rec2020ToXYZ.retainForHeapLifetime();
    rec2020FromXYZ.retainForHeapLifetime();

    auto displayP3 = JSValue::object();
    literal(displayP3, "primaries", p3Primaries);
    literal(displayP3, "whitePoint", d65);
    literal(displayP3, "transfer", srgbTransfer());
    literal(displayP3, "toXYZ", p3ToXYZ);
    literal(displayP3, "fromXYZ", p3FromXYZ);
    literal(displayP3, "luminanceCoefficients", p3Coefficients);
    literal(displayP3, "outputColorSpaceConfig", config("drawingBufferColorSpace", DisplayP3ColorSpace));

    auto linearDisplayP3 = JSValue::object();
    literal(linearDisplayP3, "primaries", p3Primaries);
    literal(linearDisplayP3, "whitePoint", d65);
    literal(linearDisplayP3, "transfer", linearTransfer());
    literal(linearDisplayP3, "toXYZ", p3ToXYZ);
    literal(linearDisplayP3, "fromXYZ", p3FromXYZ);
    literal(linearDisplayP3, "luminanceCoefficients", p3Coefficients);
    literal(linearDisplayP3, "workingColorSpaceConfig", config("unpackColorSpace", DisplayP3ColorSpace));
    literal(linearDisplayP3, "outputColorSpaceConfig", config("drawingBufferColorSpace", DisplayP3ColorSpace));

    auto linearRec2020 = JSValue::object();
    literal(linearRec2020, "primaries", rec2020Primaries);
    literal(linearRec2020, "whitePoint", d65);
    literal(linearRec2020, "transfer", linearTransfer());
    literal(linearRec2020, "toXYZ", rec2020ToXYZ);
    literal(linearRec2020, "fromXYZ", rec2020FromXYZ);
    literal(linearRec2020, "luminanceCoefficients", rec2020Coefficients);

    // `{ ...ColorManagement.spaces[ SRGBColorSpace ], outputColorSpaceConfig: { ... } }`:
    // the own enumerable properties are copied by reference, so the shared matrices keep
    // their identity, and the output configuration is replaced.
    auto extendedSRGB = JSValue::object();
    const auto srgbSpace = nativeColorManagement().get("spaces").get(SRGBColorSpace);
    for (const auto& key : srgbSpace.keys()) extendedSRGB.set(key, srgbSpace.get(key));
    auto extendedConfig = JSValue::object();
    literal(extendedConfig, "drawingBufferColorSpace", SRGBColorSpace);
    literal(extendedConfig, "toneMappingMode", "extended");
    literal(extendedSRGB, "outputColorSpaceConfig", extendedConfig);

    auto result = JSValue::object();
    literal(result, "DisplayP3ColorSpace", DisplayP3ColorSpace);
    literal(result, "LinearDisplayP3ColorSpace", LinearDisplayP3ColorSpace);
    literal(result, "DisplayP3ColorSpaceImpl", displayP3);
    literal(result, "LinearDisplayP3ColorSpaceImpl", linearDisplayP3);
    literal(result, "LinearRec2020ColorSpace", LinearRec2020ColorSpace);
    literal(result, "LinearRec2020ColorSpaceImpl", linearRec2020);
    literal(result, "ExtendedSRGBColorSpace", ExtendedSRGBColorSpace);
    literal(result, "ExtendedSRGBColorSpaceImpl", extendedSRGB);
    result.retainForHeapLifetime();
    cache = JSWeakValue(result);
    return result;
}

} // namespace ColorSpaces
