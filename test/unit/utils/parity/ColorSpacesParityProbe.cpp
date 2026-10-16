// Differential probe for the pinned examples/jsm/math/ColorSpaces.js port.
//
// The module is declarative, so the comparison covers the exported objects field by field
// and, more importantly, the identities the pinned module establishes:
//
//   * Display P3 uses the sRGB transfer function, the linear variants the linear one;
//   * Extended sRGB is a spread of ColorManagement.spaces[ 'srgb' ], so its primaries,
//     white point, transfer and both matrices must be the very same objects, while its
//     outputColorSpaceConfig is a new object;
//   * the Display P3 / Rec. 2020 matrices are the module's own globals.
//
// The probe prints one JSON object; compare-color-spaces.mjs builds the same description
// from the real module and compares with assert.deepEqual.
#include "core/JsonValue.h"
#include "core/JSValue.h"
#include "math/ColorManagement.h"
#include "math/ColorSpaces.h"
#include "constants.h"
#include <iostream>
#include <string>

namespace {
JsonValue number(double value) {
    if (std::isnan(value)) return "NaN";
    if (std::isinf(value)) return value < 0 ? "-Infinity" : "Infinity";
    if (value == 0 && std::signbit(value)) return "-0";
    return value;
}
JsonValue numbers(const JSValue& array) {
    JsonValue::Array values;
    for (const auto& key : array.keys()) values.push_back(number(array.get(key).toNumber()));
    return values;
}
JsonValue strings(const JSValue& object) {
    JsonValue::Array values;
    for (const auto& key : object.keys()) values.push_back(key);
    return values;
}
JsonValue config(const JSValue& value) {
    JsonValue::Object fields;
    for (const auto& key : value.keys()) fields[key] = value.get(key).toString();
    return fields;
}
JsonValue spaceJSON(const JSValue& space, const JSValue& srgbSpace, const JSValue& linearSpace) {
    const auto field = [&](const char* key) { return space.get(key); };
    const bool hasWorking = space.hasOwn("workingColorSpaceConfig");
    // Linear Rec. 2020 carries no configuration objects in the pinned module.
    const bool hasOutput = space.hasOwn("outputColorSpaceConfig");
    return JsonValue::Object{{"primaries", numbers(field("primaries"))},
        {"whitePoint", numbers(field("whitePoint"))},
        {"luminanceCoefficients", numbers(field("luminanceCoefficients"))},
        {"transferIsSRGB", field("transfer").strictEquals(srgbSpace.get("transfer"))},
        {"transferIsLinear", field("transfer").strictEquals(linearSpace.get("transfer"))},
        {"toXYZ", numbers(field("toXYZ").get("elements"))},
        {"fromXYZ", numbers(field("fromXYZ").get("elements"))},
        {"outputColorSpaceConfig", hasOutput ? config(field("outputColorSpaceConfig")) : JsonValue{}},
        {"hasOutputColorSpaceConfig", hasOutput},
        {"hasWorkingColorSpaceConfig", hasWorking},
        {"workingColorSpaceConfig", hasWorking ? config(field("workingColorSpaceConfig")) : JsonValue{}},
        {"sharesToXYZWithSRGB", field("toXYZ").strictEquals(srgbSpace.get("toXYZ"))},
        {"sharesPrimariesWithSRGB", field("primaries").strictEquals(srgbSpace.get("primaries"))}};
}
}

int main() {
    try {
        const auto module = ColorSpaces::nativeColorSpaces();
        const auto spaces = nativeColorManagement().get("spaces");
        const auto srgbSpace = spaces.get(SRGBColorSpace);
        const auto linearSpace = spaces.get(LinearSRGBColorSpace);
        JsonValue output = JsonValue::Object{};
        output["exportKeys"] = strings(module);
        output["names"] = JsonValue::Array{module.get("DisplayP3ColorSpace").toString(),
            module.get("LinearDisplayP3ColorSpace").toString(),
            module.get("LinearRec2020ColorSpace").toString(),
            module.get("ExtendedSRGBColorSpace").toString()};
        output["displayP3"] = spaceJSON(ColorSpaces::displayP3ColorSpaceImpl(), srgbSpace, linearSpace);
        output["linearDisplayP3"] = spaceJSON(ColorSpaces::linearDisplayP3ColorSpaceImpl(), srgbSpace, linearSpace);
        output["linearRec2020"] = spaceJSON(ColorSpaces::linearRec2020ColorSpaceImpl(), srgbSpace, linearSpace);
        const auto extended = ColorSpaces::extendedSRGBColorSpaceImpl();
        JsonValue extendedJSON = JsonValue::Object{};
        extendedJSON["ownKeys"] = strings(extended);
        extendedJSON["srgbOwnKeys"] = strings(srgbSpace);
        extendedJSON["sharesEverySpreadField"] = [&] {
            for (const auto* key : {"primaries", "whitePoint", "transfer", "toXYZ", "fromXYZ",
                     "luminanceCoefficients"})
                if (!extended.get(key).strictEquals(srgbSpace.get(key))) return false;
            return true;
        }();
        extendedJSON["outputColorSpaceConfig"] = config(extended.get("outputColorSpaceConfig"));
        extendedJSON["sharesOutputConfig"] =
            extended.get("outputColorSpaceConfig").strictEquals(srgbSpace.get("outputColorSpaceConfig"));
        extendedJSON["workingColorSpaceConfig"] = extended.hasOwn("workingColorSpaceConfig")
                                                      ? config(extended.get("workingColorSpaceConfig"))
                                                      : JsonValue{};
        output["extendedSRGB"] = std::move(extendedJSON);
        std::cout << output.dump() << '\n';
    } catch (const std::exception& error) {
        std::cout << JsonValue(JsonValue::Object{{"probeError", error.what()}}).dump() << '\n';
        return 1;
    }
    return 0;
}
