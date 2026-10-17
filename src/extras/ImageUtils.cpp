#include "extras/ImageUtils.h"
#include "animation/AnimationUtils.h"
#include "core/NativeArray.h"
#include "core/NativeObject.h"
#include "math/ColorManagement.h"
#include <cmath>
#include <cstdint>
#include <string>
#include <vector>

namespace {
JSValue arg(const std::vector<JSValue>& args, std::size_t index) { return index < args.size() ? args[index] : JSValue{}; }
// The kind a native typed array carries: the same host slot the array wrappers read, in the order they
// are registered - Int8, Uint8, Int16, Uint16, Int32, Uint32, Float32, Float64, Uint8Clamped.
constexpr std::size_t kUint8 = 1, kUint8Clamped = 8;
double elementOf(const JSValue& data, double index) {
    return data.get(std::to_string(static_cast<long long>(index))).toNumber();
}
// Rebuilds the array with the element type it came in with, which is what upstream's slice preserves.
template <class Element, class Array>
JSValue convertedArray(const JSValue& data, double length, bool eightBit) {
    const auto count = static_cast<std::size_t>(length < 0 ? 0 : length);
    std::vector<Element> values(count);
    for (std::size_t index = 0; index < count; index += 1) {
        const double value = elementOf(data, static_cast<double>(index));
        if (eightBit) values[index] = static_cast<Element>(std::floor(SRGBToLinear(value / 255.0) * 255.0));
        else values[index] = static_cast<Element>(SRGBToLinear(value));
    }
    return nativeArray(AnimationArray(Array(TypedArray<Element>(values))));
}
JSValue convertedFor(std::size_t kind, const JSValue& data, double length) {
    const bool eightBit = kind == kUint8 || kind == kUint8Clamped;
    switch (kind) {
        case 0: return convertedArray<std::int8_t, Int8Array>(data, length, eightBit);
        case 1: return convertedArray<std::uint8_t, Uint8Array>(data, length, eightBit);
        case 2: return convertedArray<std::int16_t, Int16Array>(data, length, eightBit);
        case 3: return convertedArray<std::uint16_t, Uint16Array>(data, length, eightBit);
        case 4: return convertedArray<std::int32_t, Int32Array>(data, length, eightBit);
        case 5: return convertedArray<std::uint32_t, Uint32Array>(data, length, eightBit);
        case 6: return convertedArray<float, Float32Array>(data, length, eightBit);
        case 7: return convertedArray<double, Float64Array>(data, length, eightBit);
        case 8: return convertedArray<std::uint8_t, Uint8ClampedArray>(data, length, eightBit);
        default: return {};
    }
}
std::size_t arrayKind(const JSValue& value) {
    const auto slot = value.getHostSlot("kind");
    return slot.isUndefined() ? std::size_t(-1) : std::size_t(slot.number());
}
}

JSValue ImageUtils::getDataURL(JSValue image, JSValue type) {
    static_cast<void>(type);
    // Upstream short circuits data: sources and, with no HTMLCanvasElement in scope, falls through to
    // image.src; both produce the source unchanged.
    const auto source = image.get("src");
    if (source.isUndefined()) return {};
    return source;
}

JSValue ImageUtils::sRGBToLinear(JSValue image) {
    const auto data = image.get("data");
    if (data.isUndefined()) return image;   // upstream warns and returns the argument untouched
    const auto length = data.get("length").toNumber();
    const auto converted = convertedFor(arrayKind(data), data, length);
    if (converted.isUndefined()) return image;
    auto record = JSValue::object();
    record.set("data", converted);
    record.set("width", image.get("width"));
    record.set("height", image.get("height"));
    return record;
}

JSValue ImageUtils::object() {
    static thread_local auto value = [] {
        auto result = JSValue::object();
        JSPropertyDescriptor getDataUrl;
        getDataUrl.configurable = getDataUrl.writable = true;
        getDataUrl.value = JSValue::function([](const JSValue&, const std::vector<JSValue>& args, const std::vector<JSValue>&) {
            return ImageUtils::getDataURL(arg(args, 0), arg(args, 1));
        }, {}, "getDataURL", 2);
        result.defineProperty("getDataURL", getDataUrl);
        JSPropertyDescriptor convert;
        convert.configurable = convert.writable = true;
        convert.value = JSValue::function([](const JSValue&, const std::vector<JSValue>& args, const std::vector<JSValue>&) {
            return ImageUtils::sRGBToLinear(arg(args, 0));
        }, {}, "sRGBToLinear", 1);
        result.defineProperty("sRGBToLinear", convert);
        return result;
    }();
    return value;
}