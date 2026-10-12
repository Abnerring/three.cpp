#include "textures/TextureSource.h"
#include "textures/Texture.h"
#include "math/MathUtils.h"
#include <cmath>
#include <iostream>
#include <mutex>

TextureImage::Ptr TextureImage::array(Faces faces) {
    auto result = std::make_shared<TextureImage>();
    result->isArray = true;
    result->faces = std::move(faces);
    return result;
}
TextureSource::TextureSource(TextureImage::Ptr image)
    : uuid(MathUtils::generateUUID()), data(std::move(image)) {}
void TextureSource::UpdateFlag::operator=(const JSValue& value) const {
    if (value.strictEquals(JSValue(true))) ++owner->version;
}
namespace {
double dimension(double value) { return value == 0 || std::isnan(value) ? 0 : value; }
std::string constructorName(const TextureImage::Data& data) {
    const auto retained = data;
    return std::visit([](const auto& value) -> std::string {
        using T = std::decay_t<decltype(value)>;
        if constexpr (std::is_same_v<T, std::nullptr_t>) throw JSTypeError("Cannot read properties of null (reading 'constructor')");
        else if constexpr (std::is_same_v<T, JSValue>) {
            if (value.isNull() || value.isUndefined()) throw JSTypeError("Cannot read properties of null or undefined (reading 'constructor')");
            if (value.has("constructor")) return value.get("constructor").get("name").toString();
            return value.type() == JSValue::Type::Array ? "Array" : "Object";
        } else if constexpr (std::is_same_v<T, Int8Array>) return "Int8Array";
        else if constexpr (std::is_same_v<T, Uint8Array>) return "Uint8Array";
        else if constexpr (std::is_same_v<T, Int16Array>) return "Int16Array";
        else if constexpr (std::is_same_v<T, Uint16Array>) return "Uint16Array";
        else if constexpr (std::is_same_v<T, Int32Array>) return "Int32Array";
        else if constexpr (std::is_same_v<T, Uint32Array>) return "Uint32Array";
        else if constexpr (std::is_same_v<T, Float32Array>) return "Float32Array";
        else if constexpr (std::is_same_v<T, Float64Array>) return "Float64Array";
        else return "Uint8ClampedArray";
    }, retained);
}
JsonValue serializeImage(const TextureImage::Ptr& image) {
    if (!image) throw JSTypeError("Cannot read properties of null (reading 'data')");
    const auto data = image->data;
    return std::visit([&](const auto& pixels) -> JsonValue {
        using T = std::decay_t<decltype(pixels)>;
        JsonValue values = JsonValue::Array{};
        if constexpr (std::is_same_v<T, std::nullptr_t>) {
            std::cerr << "THREE.Texture: Unable to serialize Texture.\n";
            return JsonValue::Object{};
        } else if constexpr (std::is_same_v<T, JSValue>) {
            if (!pixels.toBoolean()) {
                std::cerr << "THREE.Texture: Unable to serialize Texture.\n";
                return JsonValue::Object{};
            }
            for (std::uint32_t i = 0; i < pixels.length(); ++i)
                values.get<JsonValue::Array>().push_back(pixels.get(std::to_string(i)).toJSON().value_or(JsonValue{}));
        } else {
            for (std::size_t i = 0; i < pixels.size(); ++i)
                values.get<JsonValue::Array>().push_back(double(pixels[i]));
        }
        return JsonValue::Object{{"data", std::move(values)}, {"width", image->width},
                                 {"height", image->height}, {"type", constructorName(image->data)}};
    }, data);
}
}
Vector2& TextureSource::getSize(Vector2& target) const {
    const auto image = data;
    return target.set(image ? dimension(image->width) : 0, image ? dimension(image->height) : 0);
}
Vector3& TextureSource::getSize(Vector3& target) const {
    const auto image = data;
    return target.set(image ? dimension(image->width) : 0, image ? dimension(image->height) : 0,
                      image ? dimension(image->depth) : 0);
}
JsonValue TextureSource::toJSON(TextureJSONContext* meta) const {
    if (meta && meta->images.contains(uuid)) return meta->images.at(uuid).sharedCopy();
    JsonValue result = JsonValue::Object{{"uuid", uuid}, {"url", ""}};
    const auto image = data;
    if (image) {
        if (image->isArray) {
            JsonValue::Array urls;
            const auto length = image->faces.size();
            for (std::size_t i = 0; i < length; ++i) {
                const auto face = image->faces.at(i);
                if (!face) throw JSTypeError("Cannot read properties of null (reading 'isDataTexture')");
                urls.push_back(serializeImage(face->texture && face->texture->isDataTexture
                                               ? face->texture->image.get() : face));
            }
            result["url"] = std::move(urls);
        } else result["url"] = serializeImage(image);
    }
    if (meta) meta->images[uuid] = result.sharedCopy();
    return result;
}
Source::Source(TextureImage::Ptr image) : TextureSource(std::move(image)) {
    static std::once_flag warned;
    std::call_once(warned, [] { std::cerr << "THREE.Source: \"Source\" has been renamed to \"TextureSource\". Please update your code to use \"THREE.TextureSource\" instead.\n"; });
}
