// Unit coverage for the ported examples/jsm/lights cluster
// (RectAreaLightTexturesLib + RectAreaLightUniformsLib).
//
// The differential probe (RectAreaLightParityProbe + compare-rect-area-light.mjs) owns the
// element-by-element comparison of both modules against the pinned checkout. These
// assertions pin the native module shape without an upstream process: the two class
// objects, the DataTexture construction (dimensions, format/type, filters and flags), a
// handful of exact table values taken from the pinned oracle, and the four UniformsLib keys
// with their identities.
#include "catch.hpp"
#include "constants.h"
#include "core/JSValue.h"
#include "core/NativeObject.h"
#include "core/TypedArray.h"
#include "lights/RectAreaLightTexturesLib.h"
#include "lights/RectAreaLightUniformsLib.h"
#include "renderers/shaders/UniformsLib.h"
#include "textures/DataTexture.h"
#include <cstddef>
#include <memory>
#include <string>
#include <vector>

namespace {
constexpr std::size_t PIXELS = 64 * 64 * 4;
const std::vector<std::string> STATIC_FIELDS{
    "LTC_HALF_1", "LTC_HALF_2", "LTC_FLOAT_1", "LTC_FLOAT_2"};
const std::vector<std::string> UNIFORM_FIELDS{
    "LTC_FLOAT_1", "LTC_FLOAT_2", "LTC_HALF_1", "LTC_HALF_2"};

// The native texture behind a module field, read through the wrapper payload.
std::shared_ptr<DataTexture> texture(const JSValue& value) {
    return std::static_pointer_cast<DataTexture>(
        std::static_pointer_cast<Texture>(value.hostPayload("three.Texture")));
}
const Float32Array& floatData(const std::shared_ptr<DataTexture>& value) {
    const auto* data = std::get_if<Float32Array>(&value->image->data);
    REQUIRE(data != nullptr);
    return *data;
}
const Uint16Array& halfData(const std::shared_ptr<DataTexture>& value) {
    const auto* data = std::get_if<Uint16Array>(&value->image->data);
    REQUIRE(data != nullptr);
    return *data;
}
void checkConstruction(const std::shared_ptr<DataTexture>& value, double type) {
    REQUIRE(value->isDataTexture);
    REQUIRE(value->image->width == 64);
    REQUIRE(value->image->height == 64);
    REQUIRE(value->format == RGBAFormat);
    REQUIRE(value->type == type);
    REQUIRE(value->mapping == UVMapping);
    REQUIRE(value->wrapS == ClampToEdgeWrapping);
    REQUIRE(value->wrapT == ClampToEdgeWrapping);
    REQUIRE(value->magFilter == LinearFilter);
    REQUIRE(value->minFilter == NearestFilter);
    REQUIRE(value->anisotropy == 1);
    REQUIRE_FALSE(value->flipY);
    REQUIRE_FALSE(value->generateMipmaps);
    REQUIRE(value->unpackAlignment == 1);
    REQUIRE(value->colorSpace == NoColorSpace);
    REQUIRE(value->version == 1);
    REQUIRE(value->source->version == 1);
    REQUIRE(value->mipmaps->empty());
}
} // namespace

TEST_CASE("RectAreaLightTexturesLib exposes the pinned class object", "[rectarealight]") {
    const auto module = RectAreaLightTexturesLib::nativeRectAreaLightTexturesLib();
    const auto texturesClass = RectAreaLightTexturesLib::rectAreaLightTexturesLib();
    REQUIRE(module.keys() == std::vector<std::string>{"RectAreaLightTexturesLib"});
    REQUIRE(module.get("RectAreaLightTexturesLib").strictEquals(texturesClass));
    // A class object: own metadata, the non-enumerable static method and the four enumerable
    // static fields, all of them null until init() runs.
    REQUIRE(texturesClass.isCallable());
    REQUIRE(texturesClass.get("name").string() == "RectAreaLightTexturesLib");
    REQUIRE(texturesClass.get("length").toNumber() == 0);
    REQUIRE(texturesClass.ownPropertyNames() == std::vector<std::string>({"length", "name", "prototype",
        "init", "LTC_HALF_1", "LTC_HALF_2", "LTC_FLOAT_1", "LTC_FLOAT_2"}));
    REQUIRE(texturesClass.keys() == STATIC_FIELDS);
    REQUIRE(texturesClass.hasOwn("init"));
    REQUIRE(texturesClass.get("init").isCallable());
    REQUIRE(texturesClass.get("init").get("name").string() == "init");
    REQUIRE(texturesClass.get("init").get("length").toNumber() == 0);
    REQUIRE(texturesClass.get("prototype").get("constructor").strictEquals(texturesClass));
    for (const auto& field : STATIC_FIELDS) REQUIRE(texturesClass.get(field).isNull());
    // Classes reject ordinary calls.
    REQUIRE_THROWS_AS(texturesClass.call(), JSTypeError);
    const auto instance = texturesClass.construct({});
    REQUIRE(instance.getPrototype().strictEquals(texturesClass.get("prototype")));
    REQUIRE(instance.keys().empty());
}

TEST_CASE("RectAreaLightTexturesLib.init builds the four LTC data textures", "[rectarealight]") {
    const auto texturesClass = RectAreaLightTexturesLib::rectAreaLightTexturesLib();
    REQUIRE(texturesClass.get("init").call(texturesClass).strictEquals(texturesClass));
    const auto float1 = texture(texturesClass.get("LTC_FLOAT_1"));
    const auto float2 = texture(texturesClass.get("LTC_FLOAT_2"));
    const auto half1 = texture(texturesClass.get("LTC_HALF_1"));
    const auto half2 = texture(texturesClass.get("LTC_HALF_2"));
    checkConstruction(float1, FloatType);
    checkConstruction(float2, FloatType);
    checkConstruction(half1, HalfFloatType);
    checkConstruction(half2, HalfFloatType);
    // The data arrays are the full 64x64 RGBA tables and are non-empty.
    const auto& floats1 = floatData(float1);
    const auto& floats2 = floatData(float2);
    const auto& halves1 = halfData(half1);
    const auto& halves2 = halfData(half2);
    REQUIRE(floats1.size() == PIXELS);
    REQUIRE(floats2.size() == PIXELS);
    REQUIRE(halves1.size() == PIXELS);
    REQUIRE(halves2.size() == PIXELS);
    REQUIRE_FALSE(floats1.empty());
    REQUIRE_FALSE(halves1.empty());
    // Exact oracle values of the pinned module: Float32Array( LTC_MAT_1 ) and
    // DataUtils.toHalfFloat( LTC_MAT_1[ index ] ).
    REQUIRE(double(floats1[0]) == 1);
    REQUIRE(double(floats1[3]) == 0.000019999999494757503);
    REQUIRE(double(floats1[1023]) == 1.1283199787139893);
    REQUIRE(double(floats1[4096]) == 0.871554970741272);
    REQUIRE(double(floats1[8188]) == 1.0000900030136108);
    REQUIRE(double(floats1[8190]) == -0.00012989000242669135);
    REQUIRE(double(floats1[16383]) == 1.6576999425888062);
    REQUIRE(halves1[0] == 0x3c00);
    REQUIRE(halves1[3] == 0x014f);
    REQUIRE(halves1[1023] == 0x3c83);
    REQUIRE(halves1[4096] == 0x3af8);
    REQUIRE(halves1[8189] == 0x0c70);
    REQUIRE(halves1[8190] == 0x8841);
    REQUIRE(halves1[16383] == 0x3ea1);
    // LTC_MAT_2 keeps its subnormal Float32 values, which underflow to zero halves.
    REQUIRE(double(floats2[0]) == 1);
    REQUIRE(double(floats2[5]) == 7.914210378775547e-31);
    REQUIRE(double(floats2[8188]) == 0.36235201358795166);
    REQUIRE(double(floats2[16380]) == 0.9321640133857727);
    REQUIRE(double(floats2[16383]) == 1);
    REQUIRE(halves2[0] == 0x3c00);
    REQUIRE(halves2[5] == 0x0000);
    REQUIRE(halves2[8189] == 0x10e6);
    REQUIRE(halves2[16380] == 0x3b75);
    REQUIRE(halves2[16383] == 0x3c00);
}

TEST_CASE("RectAreaLightUniformsLib.init writes the four UniformsLib keys", "[rectarealight]") {
    const auto module = RectAreaLightUniformsLib::nativeRectAreaLightUniformsLib();
    const auto uniformsClass = RectAreaLightUniformsLib::rectAreaLightUniformsLib();
    REQUIRE(module.keys() == std::vector<std::string>{"RectAreaLightUniformsLib"});
    REQUIRE(module.get("RectAreaLightUniformsLib").strictEquals(uniformsClass));
    REQUIRE(uniformsClass.isCallable());
    REQUIRE(uniformsClass.get("name").string() == "RectAreaLightUniformsLib");
    REQUIRE(uniformsClass.ownPropertyNames() ==
        std::vector<std::string>({"length", "name", "prototype", "init"}));
    REQUIRE(uniformsClass.keys().empty());
    REQUIRE_THROWS_AS(uniformsClass.call(), JSTypeError);

    // The shared UniformsLib only gains the four keys, in upstream assignment order.
    const auto uniformsLib = nativeUniformsLib();
    const auto keysBefore = uniformsLib.keys();
    for (const auto& field : UNIFORM_FIELDS) REQUIRE_FALSE(uniformsLib.hasOwn(field));
    REQUIRE(uniformsClass.get("init").call(uniformsClass).isUndefined());
    const auto keysAfter = uniformsLib.keys();
    REQUIRE(keysAfter.size() == keysBefore.size() + UNIFORM_FIELDS.size());
    REQUIRE(std::vector<std::string>(keysAfter.begin(), keysAfter.begin() + keysBefore.size()) == keysBefore);
    REQUIRE(std::vector<std::string>(keysAfter.end() - UNIFORM_FIELDS.size(), keysAfter.end()) == UNIFORM_FIELDS);
    // The assigned values are the very textures of RectAreaLightTexturesLib, which the call
    // above rebuilt once more.
    const auto texturesClass = RectAreaLightTexturesLib::rectAreaLightTexturesLib();
    for (const auto& field : UNIFORM_FIELDS) {
        REQUIRE(uniformsLib.hasOwn(field));
        REQUIRE(uniformsLib.get(field).strictEquals(texturesClass.get(field)));
    }
    REQUIRE(texture(uniformsLib.get("LTC_FLOAT_1"))->type == FloatType);
    REQUIRE(texture(uniformsLib.get("LTC_FLOAT_2"))->type == FloatType);
    REQUIRE(texture(uniformsLib.get("LTC_HALF_1"))->type == HalfFloatType);
    REQUIRE(texture(uniformsLib.get("LTC_HALF_2"))->type == HalfFloatType);
    REQUIRE(halfData(texture(uniformsLib.get("LTC_HALF_1"))).size() == PIXELS);
}
