#include "textures/Texture.h"
#include "textures/DataArrayTexture.h"
#include "textures/Data3DTexture.h"
#include "textures/CompressedArrayTexture.h"
#include "textures/DepthTexture.h"
#include "math/MathUtils.h"
#include <cmath>
#include <iostream>
#include <unordered_set>

namespace { thread_local Vector3 textureSize; }
Texture::Texture(Image image, double mapping, double wrapS, double wrapT, double magFilter, double minFilter,
                 double format, double type, double anisotropy, std::string colorSpace)
    : uuid(MathUtils::generateUUID()), source(std::make_shared<TextureSource>(image)),
      mapping(mapping), wrapS(wrapS), wrapT(wrapT), magFilter(magFilter), minFilter(minFilter),
      anisotropy(anisotropy), format(format), type(type), colorSpace(std::move(colorSpace)) {
    isArrayTexture = image && image->depth > 1;
}
void Texture::UpdateFlag::operator=(const JSValue& value) const {
    if (!value.strictEquals(JSValue(true))) return;
    if (pmrem) ++owner->pmremVersion;
    else {
        ++owner->version;
        if (!owner->source) throw JSTypeError("Cannot set properties of null (setting 'needsUpdate')");
        owner->source->needsUpdate = true;
    }
}
Texture::Image Texture::ImageProperty::get() const {
    if (!owner->source) throw JSTypeError("Cannot read properties of null (reading 'data')");
    return owner->source->data;
}
Texture::ImageProperty& Texture::ImageProperty::operator=(Image value) {
    if (!owner->source) throw JSTypeError("Cannot set properties of null (setting 'data')");
    owner->source->data = std::move(value);
    return *this;
}
Texture::Dimension::operator double() const {
    const auto source = owner->source;
    if (!source) throw JSTypeError("Cannot read properties of null (reading 'getSize')");
    const auto& size = source->getSize(textureSize);
    return axis == 0 ? double(size.x) : axis == 1 ? double(size.y) : double(size.z);
}
void Texture::updateMatrix() {
    matrix.setUvTransform(offset.x, offset.y, repeat.x, repeat.y, rotation, center.x, center.y);
}
void Texture::addUpdateRange(double start, double count) {
    if (!updateRanges) throw JSTypeError("Cannot read properties of null (reading 'push')");
    updateRanges->push_back(std::make_shared<UpdateRange>(UpdateRange{start, count}));
}
void Texture::clearUpdateRanges() {
    if (!updateRanges) throw JSTypeError("Cannot set properties of null (setting 'length')");
    updateRanges->clear();
}
std::shared_ptr<Texture> Texture::clone() const {
    auto result = std::make_shared<Texture>(); result->copy(*this); return result;
}
Texture& Texture::copy(const Texture& other) {
    name = other.name;
    source = other.source;
    if (!other.mipmaps) throw JSTypeError("Cannot read properties of undefined (reading 'slice')");
    mipmaps = std::make_shared<std::vector<Image>>(*other.mipmaps);
    mapping = other.mapping; channel = other.channel;
    wrapS = other.wrapS; wrapT = other.wrapT;
    magFilter = other.magFilter; minFilter = other.minFilter; anisotropy = other.anisotropy;
    format = other.format; internalFormat = other.internalFormat; type = other.type; normalized = other.normalized;
    offset.copy(other.offset); repeat.copy(other.repeat); center.copy(other.center); rotation = other.rotation;
    matrixAutoUpdate = other.matrixAutoUpdate; matrix.copy(other.matrix);
    generateMipmaps = other.generateMipmaps; mipmapsAutoUpdate = other.mipmapsAutoUpdate;
    premultiplyAlpha = other.premultiplyAlpha; flipY = other.flipY; unpackAlignment = other.unpackAlignment;
    colorSpace = other.colorSpace; renderTarget = other.renderTarget;
    isRenderTargetTexture = other.isRenderTargetTexture; isPMREMTexture = other.isPMREMTexture;
    isArrayTexture = other.isArrayTexture;
    const auto json = other.userData.toJSON();
    if (!json) throw JSTypeError("Texture.copy: userData has no JSON representation");
    userData = JSValue::fromJSON(*json);
    needsUpdate = true;
    return *this;
}
void Texture::setValues(JSValue values) {
    // for-in includes inherited enumerable properties, with nearer properties shadowing prototypes.
    std::vector<std::string> keys;
    std::unordered_set<std::string> visited;
    for (JSValue object = values; object.isObject(); object = object.getPrototype()) {
        for (const auto& key : object.ownPropertyNames()) {
            if (!visited.insert(key).second) continue;
            const auto descriptor = object.getOwnPropertyDescriptor(key);
            if (descriptor && descriptor->enumerable.value_or(false)) keys.push_back(key);
        }
    }
    for (const auto& key : keys) {
        const JSValue value = values.get(key);
        if (value.isUndefined()) {
            std::cerr << "THREE.Texture.setValues(): parameter '" << key << "' has value of undefined.\n";
            continue;
        }
        if (key == "name") name = value.toString();
        else if (key == "uuid") uuid = value.toString();
        else if (key == "colorSpace") colorSpace = value.toString();
        else if (key == "internalFormat") internalFormat = value.isNull() ? std::nullopt : std::optional(value.toString());
        else if (key == "userData") userData = value;
        else if (key == "mapping") mapping = value.toNumber();
        else if (key == "channel") channel = value.toNumber();
        else if (key == "wrapS") wrapS = value.toNumber();
        else if (key == "wrapT") wrapT = value.toNumber();
        else if (key == "wrapR") {
            if (auto texture = dynamic_cast<DataArrayTexture*>(this)) texture->wrapR = value.toNumber();
            else if (auto texture = dynamic_cast<Data3DTexture*>(this)) texture->wrapR = value.toNumber();
            else if (auto texture = dynamic_cast<CompressedArrayTexture*>(this)) texture->wrapR = value.toNumber();
            else std::cerr << "THREE.Texture.setValues(): property 'wrapR' does not exist.\n";
        } else if (key == "compareFunction") {
            if (auto texture = dynamic_cast<DepthTexture*>(this))
                texture->compareFunction = value.isNull() ? std::nullopt : std::optional(value.toNumber());
            else std::cerr << "THREE.Texture.setValues(): property 'compareFunction' does not exist.\n";
        }
        else if (key == "magFilter") magFilter = value.toNumber();
        else if (key == "minFilter") minFilter = value.toNumber();
        else if (key == "format") format = value.toNumber();
        else if (key == "type") type = value.toNumber();
        else if (key == "unpackAlignment") unpackAlignment = value.toNumber();
        else if (key == "anisotropy") anisotropy = value.toNumber();
        else if (key == "rotation") rotation = value.toNumber();
        else if (key == "version") version = value.toNumber();
        else if (key == "pmremVersion") pmremVersion = value.toNumber();
        else if (key == "normalized") normalized = value.toBoolean();
        else if (key == "matrixAutoUpdate") matrixAutoUpdate = value.toBoolean();
        else if (key == "generateMipmaps") generateMipmaps = value.toBoolean();
        else if (key == "mipmapsAutoUpdate") mipmapsAutoUpdate = value.toBoolean();
        else if (key == "premultiplyAlpha") premultiplyAlpha = value.toBoolean();
        else if (key == "flipY") flipY = value.toBoolean();
        else if (key == "isRenderTargetTexture") isRenderTargetTexture = value.toBoolean();
        else if (key == "isPMREMTexture") isPMREMTexture = value.toBoolean();
        else if (key == "isArrayTexture") isArrayTexture = value.toBoolean();
        else if (key == "offset" || key == "repeat" || key == "center") {
            if (!value.isObject() || !value.get("isVector2").toBoolean())
                throw JSTypeError("Texture.setValues: replacing native Vector2 with an arbitrary value is not implemented");
            auto& target = key == "offset" ? offset : key == "repeat" ? repeat : center;
            target.x = value.get("x").toNumber(); target.y = value.get("y").toNumber();
        } else if (key == "matrix") {
            if (!value.isObject() || !value.get("isMatrix3").toBoolean())
                throw JSTypeError("Texture.setValues: replacing native Matrix3 with an arbitrary value is not implemented");
            const auto elements = value.get("elements");
            for (std::size_t i = 0; i < 9; ++i) matrix.elements()[i] = elements.get(std::to_string(i)).toNumber();
        } else if (key == "id" || key == "width" || key == "height" || key == "depth") {
            throw JSTypeError("Cannot assign to read only property '" + key + "'");
        } else if (key == "source" || key == "image" || key == "mipmaps" || key == "onUpdate" || key == "renderTarget" || key == "updateRanges") {
            throw JSTypeError("Texture.setValues: use the native field for '" + key + "'");
        } else {
            // The write-only update properties also read as undefined upstream.
            std::cerr << "THREE.Texture.setValues(): property '" << key << "' does not exist.\n";
        }
    }
}
JsonValue Texture::toJSON(TextureJSONContext* meta) const {
    if (meta && meta->textures.contains(uuid)) return meta->textures.at(uuid).sharedCopy();
    const auto imageSource = source;
    if (!imageSource) throw JSTypeError("Cannot read properties of null (reading 'toJSON')");
    JsonValue output = JsonValue::Object{
        {"metadata", JsonValue::Object{{"version", 4.7}, {"type", "Texture"}, {"generator", "Texture.toJSON"}}},
        {"uuid", uuid}, {"name", name}, {"image", imageSource->toJSON(meta).at("uuid")},
        {"mapping", mapping}, {"channel", channel},
        {"repeat", JsonValue::Array{repeat.x, repeat.y}}, {"offset", JsonValue::Array{offset.x, offset.y}},
        {"center", JsonValue::Array{center.x, center.y}}, {"rotation", rotation}, {"wrap", JsonValue::Array{wrapS, wrapT}},
        {"format", format}, {"internalFormat", internalFormat ? JsonValue(*internalFormat) : JsonValue{}},
        {"type", type}, {"normalized", normalized}, {"colorSpace", colorSpace}, {"minFilter", minFilter},
        {"magFilter", magFilter}, {"anisotropy", anisotropy}, {"flipY", flipY}, {"generateMipmaps", generateMipmaps},
        {"mipmapsAutoUpdate", mipmapsAutoUpdate}, {"premultiplyAlpha", premultiplyAlpha}, {"unpackAlignment", unpackAlignment}};
    const auto data = userData;
    if (data.isUndefined() || data.isNull()) throw JSTypeError("Cannot convert undefined or null to object");
    const bool hasData = data.isObject() ? !data.keys().empty()
        : data.type() == JSValue::Type::String && !data.string().empty();
    if (hasData) output["userData"] = userData.toJSON().value_or(JsonValue{});
    if (meta) meta->textures[uuid] = output.sharedCopy();
    return output;
}
void Texture::dispose() { dispatchEvent(Event{"dispose"}); }
Vector2& Texture::transformUv(Vector2& uv) const {
    if (mapping != UVMapping) return uv;
    uv.applyMatrix3(matrix);
    auto wrap = [](double coordinate, double wrapping) {
        if (coordinate < 0 || coordinate > 1) {
            if (wrapping == RepeatWrapping) return coordinate - std::floor(coordinate);
            if (wrapping == ClampToEdgeWrapping) return coordinate < 0 ? 0.0 : 1.0;
            if (wrapping == MirroredRepeatWrapping)
                return std::abs(std::fmod(std::floor(coordinate), 2)) == 1
                     ? std::ceil(coordinate) - coordinate : coordinate - std::floor(coordinate);
        }
        return coordinate;
    };
    uv.x = wrap(uv.x, wrapS); uv.y = wrap(uv.y, wrapT);
    if (flipY) uv.y = 1 - uv.y;
    return uv;
}
