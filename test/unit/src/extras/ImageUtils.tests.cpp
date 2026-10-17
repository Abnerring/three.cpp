#include "catch.hpp"
#include "animation/AnimationUtils.h"
#include "core/JSValue.h"
#include "core/NativeArray.h"
#include "core/NativeObject.h"
#include "extras/ImageUtils.h"
#include "math/ColorManagement.h"
#include <cmath>
#include <cstdint>
#include <string>
#include <vector>

// The two DOM guarded branches of the pinned module are unreachable natively, so what is asserted here
// is the behaviour upstream defines for that situation: getDataURL hands back the source, and
// sRGBToLinear converts image.data, flooring scaled eight-bit values and converting other arrays as
// floating point, while an argument without data comes back untouched.
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
JSValue recordOf(int kind, const std::vector<double>& values) {
    auto record = JSValue::object();
    record.set("data", makeData(kind, values));
    record.set("width", 2.0);
    record.set("height", 1.0);
    return record;
}
double at(const JSValue& record, int index) {
    return record.get("data").get(std::to_string(index)).toNumber();
}
}

TEST_CASE("ImageUtils.getDataURL returns the source unchanged", "[imageutils]") {
    const auto realm = JSValue::object();
    static_cast<void>(realm);
    auto image = JSValue::object();
    image.set("src", "data:image/png;base64,AAAA");
    REQUIRE(ImageUtils::getDataURL(image).toString() == "data:image/png;base64,AAAA");
    auto plain = JSValue::object();
    plain.set("src", "texture.png");
    REQUIRE(ImageUtils::getDataURL(plain).toString() == "texture.png");
    // The type argument is accepted and ignored, as upstream does when it falls through to the source.
    REQUIRE(ImageUtils::getDataURL(plain, JSValue("image/jpeg")).toString() == "texture.png");
}

TEST_CASE("ImageUtils.sRGBToLinear floors eight bit data and keeps its array kind", "[imageutils]") {
    const auto realm = JSValue::object();
    static_cast<void>(realm);
    const auto converted = ImageUtils::sRGBToLinear(recordOf(1, {0, 128, 255}));
    REQUIRE(at(converted, 0) == 0.0);
    REQUIRE(at(converted, 2) == 255.0);
    const auto expected = std::floor(SRGBToLinear(128.0 / 255.0) * 255.0);
    REQUIRE(at(converted, 1) == expected);
    // The converted data is still a Uint8Array: kind 1 is the second wrapper registered.
    REQUIRE(converted.get("data").getHostSlot("kind").number() == 1.0);
    REQUIRE(converted.get("width").toNumber() == 2.0);
}

TEST_CASE("ImageUtils.sRGBToLinear treats non eight bit arrays as float data", "[imageutils]") {
    const auto realm = JSValue::object();
    static_cast<void>(realm);
    const auto converted = ImageUtils::sRGBToLinear(recordOf(6, {0.0, 0.5, 1.0}));
    REQUIRE(at(converted, 0) == 0.0);
    REQUIRE(std::abs(at(converted, 2) - 1.0) < 1e-6);              // Float32 storage, so compare loosely
    REQUIRE(std::abs(at(converted, 1) - SRGBToLinear(0.5)) < 1e-6);
}

TEST_CASE("ImageUtils.sRGBToLinear returns an argument without data untouched", "[imageutils]") {
    const auto realm = JSValue::object();
    static_cast<void>(realm);
    auto image = JSValue::object();
    image.set("width", 4.0);
    REQUIRE(ImageUtils::sRGBToLinear(image).strictEquals(image));
}