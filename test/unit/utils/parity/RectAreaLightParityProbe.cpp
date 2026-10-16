// Differential probe for the pinned examples/jsm/lights port
// (RectAreaLightTexturesLib + RectAreaLightUniformsLib).
//
// Protocol: the probe prints ONE JSON object describing the native modules and exits.
// compare-rect-area-light.mjs imports the real pinned modules (with 'three' rewritten to
// <checkout>/src/Three.js) and compares both trees with assert.deepEqual, without any
// tolerance.
//
// The description covers everything the pinned module decides:
//
//   * the module export names and the class-object shape (typeof, name, length, own key
//     order, the non-enumerable static `init`, the TypeError of an ordinary call, `new`
//     instances) of both classes;
//   * the four data textures: dimensions, format/type/mapping/wrapping/filters, the
//     DataTexture flags (flipY, generateMipmaps, unpackAlignment, premultiplyAlpha,
//     matrixAutoUpdate), colorSpace, version/source version and the FULL pixel array,
//     element by element, read through the native side of the wrapper so the bytes are
//     compared and not only the wrapper shape;
//   * the four writes RectAreaLightUniformsLib.init() performs on UniformsLib, their order
//     and the identity relations (UniformsLib.LTC_X === RectAreaLightTexturesLib.LTC_X);
//   * that a second init() rebuilds the textures, like the pinned module.
//
// Numbers use the JSON encoding of the other probes: finite values stay JSON numbers
// (JsonValue writes 17 significant digits, so they round-trip exactly) and -0/NaN/±Infinity
// become the strings "-0"/"NaN"/"Infinity"/"-Infinity".
#include "core/JSValue.h"
#include "core/JsonValue.h"
#include "core/NativeObject.h"
#include "core/TypedArray.h"
#include "lights/RectAreaLightTexturesLib.h"
#include "lights/RectAreaLightUniformsLib.h"
#include "renderers/shaders/UniformsLib.h"
#include "textures/DataTexture.h"
#include <cmath>
#include <cstddef>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

namespace {

// The static fields of the class in upstream source order.
const std::vector<std::string> classFields{
    "LTC_HALF_1", "LTC_HALF_2", "LTC_FLOAT_1", "LTC_FLOAT_2"};
// The order in which RectAreaLightUniformsLib.init() writes them into UniformsLib.
const std::vector<std::string> uniformsFields{
    "LTC_FLOAT_1", "LTC_FLOAT_2", "LTC_HALF_1", "LTC_HALF_2"};

JsonValue number(double value) {
    if (std::isnan(value)) return "NaN";
    if (std::isinf(value)) return value < 0 ? "-Infinity" : "Infinity";
    if (value == 0 && std::signbit(value)) return "-0";
    return value;
}

JsonValue strings(const std::vector<std::string>& values) {
    JsonValue::Array result;
    for (const auto& value : values) result.emplace_back(value);
    return result;
}

JsonValue hasOwn(const JSValue& object, const std::vector<std::string>& fields) {
    JsonValue::Object result;
    for (const auto& field : fields) result[field] = object.hasOwn(field);
    return result;
}

// The native DataTexture behind a module field. The wrapper is a real native host object, so
// its payload is the very texture the module assigned.
std::shared_ptr<DataTexture> textureOf(const JSValue& value) {
    return std::static_pointer_cast<DataTexture>(
        std::static_pointer_cast<Texture>(value.hostPayload("three.Texture")));
}

JsonValue textureDescription(const JSValue& value) {
    const auto texture = textureOf(value);
    const auto image = texture->image.get();
    std::string dataType = "none";
    JsonValue::Array data;
    if (const auto* floats = std::get_if<Float32Array>(&image->data)) {
        dataType = "Float32Array";
        data.reserve(floats->size());
        for (std::size_t index = 0; index < floats->size(); ++index)
            data.push_back(number(double((*floats)[index])));
    } else if (const auto* halves = std::get_if<Uint16Array>(&image->data)) {
        dataType = "Uint16Array";
        data.reserve(halves->size());
        for (std::size_t index = 0; index < halves->size(); ++index)
            data.push_back(number(double((*halves)[index])));
    }
    return JsonValue::Object{
        {"isDataTexture", texture->isDataTexture},
        {"width", number(image->width)},
        {"height", number(image->height)},
        {"format", number(texture->format)},
        {"type", number(texture->type)},
        {"mapping", number(texture->mapping)},
        {"wrapS", number(texture->wrapS)},
        {"wrapT", number(texture->wrapT)},
        {"magFilter", number(texture->magFilter)},
        {"minFilter", number(texture->minFilter)},
        {"anisotropy", number(texture->anisotropy)},
        {"flipY", texture->flipY},
        {"generateMipmaps", texture->generateMipmaps},
        {"premultiplyAlpha", texture->premultiplyAlpha},
        {"matrixAutoUpdate", texture->matrixAutoUpdate},
        {"unpackAlignment", number(texture->unpackAlignment)},
        {"colorSpace", texture->colorSpace},
        {"version", number(texture->version)},
        {"sourceVersion", number(texture->source->version)},
        {"mipmapCount", number(double(texture->mipmaps->size()))},
        {"dataType", dataType},
        {"data", std::move(data)}};
}

JsonValue textures(const JSValue& object) {
    JsonValue::Object result;
    for (const auto& field : classFields) result[field] = textureDescription(object.get(field));
    return result;
}

JsonValue identities(const JSValue& object, const JSValue& other) {
    JsonValue::Object result;
    for (const auto& field : uniformsFields)
        result[field] = object.get(field).strictEquals(other.get(field));
    return result;
}

std::string ordinaryCall(const JSValue& function) {
    try {
        function.call();
        return "none";
    } catch (const JSTypeError&) {
        return "TypeError";
    }
}

bool prefixOf(const std::vector<std::string>& before, const std::vector<std::string>& after) {
    if (before.size() > after.size()) return false;
    for (std::size_t index = 0; index < before.size(); ++index)
        if (before[index] != after[index]) return false;
    return true;
}

JsonValue lastKeys(const std::vector<std::string>& keys, std::size_t count) {
    JsonValue::Array result;
    const auto first = keys.size() > count ? keys.size() - count : 0;
    for (std::size_t index = first; index < keys.size(); ++index) result.emplace_back(keys[index]);
    return result;
}

// Element-wise comparison of the two pixel storages, so the second init() is proven to
// rebuild the identical tables and not only new wrappers.
bool samePixels(const std::shared_ptr<DataTexture>& first, const std::shared_ptr<DataTexture>& second) {
    const auto* firstFloats = std::get_if<Float32Array>(&first->image->data);
    const auto* secondFloats = std::get_if<Float32Array>(&second->image->data);
    if (firstFloats && secondFloats) {
        if (firstFloats->size() != secondFloats->size()) return false;
        for (std::size_t index = 0; index < firstFloats->size(); ++index)
            if ((*firstFloats)[index] != (*secondFloats)[index]) return false;
        return true;
    }
    const auto* firstHalves = std::get_if<Uint16Array>(&first->image->data);
    const auto* secondHalves = std::get_if<Uint16Array>(&second->image->data);
    if (firstHalves && secondHalves) {
        if (firstHalves->size() != secondHalves->size()) return false;
        for (std::size_t index = 0; index < firstHalves->size(); ++index)
            if ((*firstHalves)[index] != (*secondHalves)[index]) return false;
        return true;
    }
    return false;
}
} // namespace

int main() {
    try {
        const auto texturesModule = RectAreaLightTexturesLib::nativeRectAreaLightTexturesLib();
        const auto texturesClass = RectAreaLightTexturesLib::rectAreaLightTexturesLib();
        JsonValue output = JsonValue::Object{};

        output["texturesModuleKeys"] = strings(texturesModule.keys());
        output["texturesModuleExportIsClassObject"] =
            texturesModule.get("RectAreaLightTexturesLib").strictEquals(texturesClass);

        // The class object before init(): a real class function whose four static fields are null.
        output["classType"] = texturesClass.isCallable() ? "function" : "object";
        output["className"] = texturesClass.get("name").toString();
        output["classLength"] = number(texturesClass.get("length").toNumber());
        output["classOwnKeys"] = strings(texturesClass.ownPropertyNames());
        output["classEnumerableKeys"] = strings(texturesClass.keys());
        output["classPrototypeConstructorIsClass"] =
            texturesClass.get("prototype").get("constructor").strictEquals(texturesClass);
        output["ordinaryClassCall"] = ordinaryCall(texturesClass);
        const auto instance = texturesClass.construct({});
        output["instanceKeys"] = strings(instance.keys());
        output["instancePrototypeIsClassPrototype"] =
            instance.getPrototype().strictEquals(texturesClass.get("prototype"));

        const auto init = texturesClass.get("init");
        output["initType"] = init.isCallable() ? "function" : "object";
        output["initName"] = init.get("name").toString();
        output["initLength"] = number(init.get("length").toNumber());
        output["initOwnOnClass"] = texturesClass.hasOwn("init");

        JsonValue::Object beforeInit;
        for (const auto& field : classFields)
            beforeInit[field] = texturesClass.get(field).isNull() ? "null" : "not-null";
        output["staticFieldsBeforeInit"] = std::move(beforeInit);

        // RectAreaLightTexturesLib.init()
        output["initReturnsClassObject"] = init.call(texturesClass).strictEquals(texturesClass);
        output["textures"] = textures(texturesClass);
        output["classEnumerableKeysAfterInit"] = strings(texturesClass.keys());
        output["classStableAcrossInit"] =
            RectAreaLightTexturesLib::nativeRectAreaLightTexturesLib()
                .get("RectAreaLightTexturesLib")
                .strictEquals(texturesClass);

        // RectAreaLightUniformsLib.init()
        const auto uniformsModule = RectAreaLightUniformsLib::nativeRectAreaLightUniformsLib();
        const auto uniformsClass = RectAreaLightUniformsLib::rectAreaLightUniformsLib();
        output["uniformsModuleKeys"] = strings(uniformsModule.keys());
        output["uniformsModuleExportIsClassObject"] =
            uniformsModule.get("RectAreaLightUniformsLib").strictEquals(uniformsClass);
        output["uniformsClassName"] = uniformsClass.get("name").toString();
        output["uniformsClassType"] = uniformsClass.isCallable() ? "function" : "object";
        output["uniformsClassOwnKeys"] = strings(uniformsClass.ownPropertyNames());
        output["uniformsClassEnumerableKeys"] = strings(uniformsClass.keys());
        output["uniformsOrdinaryClassCall"] = ordinaryCall(uniformsClass);

        const auto uniformsLib = nativeUniformsLib();
        const auto keysBefore = uniformsLib.keys();
        output["uniformsHasOwnBeforeInit"] = hasOwn(uniformsLib, uniformsFields);

        const auto uniformsInit = uniformsClass.get("init");
        output["uniformsInitType"] = uniformsInit.isCallable() ? "function" : "object";
        output["uniformsInitName"] = uniformsInit.get("name").toString();
        output["uniformsInitLength"] = number(uniformsInit.get("length").toNumber());
        output["uniformsInitReturnsUndefined"] = uniformsInit.call(uniformsClass).isUndefined();

        const auto keysAfter = uniformsLib.keys();
        output["uniformsHasOwnAfterInit"] = hasOwn(uniformsLib, uniformsFields);
        output["uniformsKeysBeforeArePrefix"] = prefixOf(keysBefore, keysAfter);
        output["uniformsKeysAdded"] = number(double(keysAfter.size()) - double(keysBefore.size()));
        output["uniformsLastKeys"] = lastKeys(keysAfter, uniformsFields.size());
        output["uniformsIdentity"] = identities(uniformsLib, texturesClass);

        // A second init() rebuilds the four textures; UniformsLib follows the new objects and
        // the rebuilt tables are element-wise identical.
        std::vector<JSValue> previous;
        std::vector<std::shared_ptr<DataTexture>> previousTextures;
        previous.reserve(classFields.size());
        previousTextures.reserve(classFields.size());
        for (const auto& field : classFields) {
            previous.push_back(texturesClass.get(field));
            previousTextures.push_back(textureOf(previous.back()));
        }
        uniformsInit.call(uniformsClass);
        JsonValue::Object replaced;
        bool follows = true, identical = true;
        for (std::size_t index = 0; index < classFields.size(); ++index) {
            const auto& field = classFields[index];
            replaced[field] = !texturesClass.get(field).strictEquals(previous[index]);
            follows = follows && uniformsLib.get(field).strictEquals(texturesClass.get(field));
            identical = identical && samePixels(previousTextures[index], textureOf(texturesClass.get(field)));
        }
        output["secondInit"] = JsonValue::Object{
            {"fieldsReplaced", std::move(replaced)},
            {"uniformsFollowNewTextures", follows},
            {"rebuiltTablesIdentical", identical}};

        // Every further init() call rebuilds the textures and returns the same class object;
        // the native entry of the static method is the same call.
        output["nativeInitReturnsClassObject"] =
            RectAreaLightTexturesLib::init().strictEquals(texturesClass);

        std::cout << output.dump() << '\n';
    } catch (const std::exception& error) {
        std::cout << JsonValue(JsonValue::Object{{"probeError", error.what()}}).dump() << '\n';
        return 1;
    }
    return 0;
}
