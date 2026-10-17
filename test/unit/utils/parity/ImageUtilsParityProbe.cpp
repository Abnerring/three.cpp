// Differential probe for the native src/extras/ImageUtils.js port.
//
// Each stdin line is `kind length value...`, where kind indexes the typed array wrappers (1 Uint8,
// 8 Uint8Clamped, 6 Float32, 7 Float64). The probe builds the record upstream expects, runs
// sRGBToLinear and prints the converted values with the width and height it was given.
#include "animation/AnimationUtils.h"
#include "core/JsonValue.h"
#include "core/JSValue.h"
#include "core/NativeArray.h"
#include "core/NativeObject.h"
#include "extras/ImageUtils.h"
#include <cstdint>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace {
// Builds a native typed array instance of the requested kind. Four kinds cover the two branches
// (eight bit versus floating point) and both storage widths.
template <class Element, class Array>
JSValue typedArray(const std::vector<double>& values) {
    std::vector<Element> converted(values.begin(), values.end());
    return nativeArray(AnimationArray(Array(TypedArray<Element>(converted))));
}
JSValue makeData(int kind, const std::vector<double>& values) {
    switch (kind) {
        case 1: return typedArray<std::uint8_t, Uint8Array>(values);
        case 8: return typedArray<std::uint8_t, Uint8ClampedArray>(values);
        case 6: return typedArray<float, Float32Array>(values);
        case 7: return typedArray<double, Float64Array>(values);
        default: throw JSTypeError("unsupported kind in fixture");
    }
}
}
int main() {
    const auto realm = JSValue::object();
    static_cast<void>(realm);
    std::string line;
    while (std::getline(std::cin, line)) {
        try {
            std::istringstream fields(line);
            int kind = 0;
            int length = 0;
            fields >> kind >> length;
            std::vector<double> values(static_cast<std::size_t>(length < 0 ? 0 : length));
            for (int index = 0; index < length; ++index) fields >> values[static_cast<std::size_t>(index)];
            auto image = JSValue::object();
            image.set("data", makeData(kind, values));
            image.set("width", 3.0);
            image.set("height", 1.0);
            const auto api = ImageUtils::object();
            const auto converted = api.get("sRGBToLinear").call(api, {image});
            const auto result = converted.get("data");
            JsonValue::Array numbers;
            for (int index = 0; index < length; ++index)
                numbers.emplace_back(result.get(std::to_string(index)).toNumber());
            std::cout << JsonValue(JsonValue::Object{
                {"data", JsonValue(numbers)},
                {"width", converted.get("width").toNumber()},
                {"height", converted.get("height").toNumber()}}).dump() << '\n';
        } catch (const std::exception& error) {
            std::cout << JsonValue(JsonValue::Object{{"probeError", error.what()}}).dump() << '\n';
        }
    }
    return 0;
}