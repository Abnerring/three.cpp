#include "core/RenderTarget.h"
#include <cmath>

namespace {
JSValue targetOptions(const JSValue& source) {
    auto result = JSValue::object();
    for (const auto& [key, value] : std::initializer_list<std::pair<const char*, JSValue>>{
             {"generateMipmaps", false}, {"internalFormat", nullptr}, {"minFilter", LinearFilter},
             {"depthBuffer", true}, {"stencilBuffer", false}, {"resolveColorBuffer", true},
             {"resolveDepthBuffer", true}, {"resolveStencilBuffer", true},
             {"storeMultisampledColorBuffer", true}, {"storeMultisampledDepthBuffer", true},
             {"storeMultisampledStencilBuffer", true}, {"depthTexture", nullptr},
             {"samples", 0}, {"count", 1}, {"depth", 1}, {"multiview", false},
             {"useArrayDepthTexture", false}}) result.set(key, value);
    // Object.assign visits own enumerable keys and evaluates each getter immediately.
    if (source.isObject()) {
        const auto keys = source.ownPropertyNames();
        for (const auto& key : keys) {
            const auto descriptor = source.getOwnPropertyDescriptor(key);
            if (descriptor && descriptor->enumerable.value_or(false)) result.set(key, source.get(key));
        }
    }
    return result;
}
std::shared_ptr<Texture> optionTexture(const JSValue& value) {
    if (value.isNull()) return nullptr;
    if (!value.isHostObject("three.Texture"))
        throw JSTypeError("RenderTarget.depthTexture requires a native Texture or null");
    return std::static_pointer_cast<Texture>(value.hostPayload("three.Texture"));
}
std::shared_ptr<Texture> attachment(const RenderTarget& target, std::size_t index) {
    if (index >= target.textures.size() || target.textures.storage()->holes.contains(index) || !target.textures[index])
        throw JSTypeError("Cannot read properties of undefined (render target texture)");
    return target.textures[index];
}
void setAttachment(SharedArray<std::shared_ptr<Texture>> textures, std::size_t index, std::shared_ptr<Texture> texture) {
    const auto storage = textures.storage();
    if (index >= storage->values.size()) {
        if (!storage->lengthWritable) throw JSTypeError("Cannot assign to read only array length");
        const auto oldSize = storage->values.size();
        storage->values.resize(index + 1);
        for (auto hole = oldSize; hole < index; ++hole) storage->holes.insert(hole);
    } else if (!storage->holes.contains(index) && storage->flags.contains(index) && !storage->flags.at(index).writable) {
        throw JSTypeError("Cannot assign to read only array element");
    }
    storage->values[index] = std::move(texture);
    if (storage->holes.erase(index)) storage->flags.erase(index);
}
}

RenderTarget::RenderTarget(double w, double h, JSValue input)
    : width(w), height(h), depth(1), scissor(0, 0, w, h), viewport(0, 0, w, h) {
    const auto options = targetOptions(input);
    depth = options.get("depth").toNumber();
    const auto image = std::make_shared<TextureImage>(width, height, depth);
    const auto original = std::make_shared<Texture>(image);
    const auto count = options.get("count");
    for (std::size_t i = 0; double(i) < count.toNumber(); ++i) {
        if (i >= 0xffffffffULL) throw JSRangeError("Invalid array length");
        auto current = original->clone();
        textures.push_back(current);
        current->isRenderTargetTexture = true;
        current->renderTarget = this;
    }
    _setTextureOptions(options);
    depthBuffer = options.get("depthBuffer").toBoolean();
    stencilBuffer = options.get("stencilBuffer").toBoolean();
    resolveColorBuffer = options.get("resolveColorBuffer").toBoolean();
    resolveDepthBuffer = options.get("resolveDepthBuffer").toBoolean();
    resolveStencilBuffer = options.get("resolveStencilBuffer").toBoolean();
    storeMultisampledColorBuffer = options.get("storeMultisampledColorBuffer").toBoolean();
    storeMultisampledDepthBuffer = options.get("storeMultisampledDepthBuffer").toBoolean();
    storeMultisampledStencilBuffer = options.get("storeMultisampledStencilBuffer").toBoolean();
    depthTexture = optionTexture(options.get("depthTexture"));
    samples = options.get("samples").toNumber();
    multiview = options.get("multiview").toBoolean();
    useArrayDepthTexture = options.get("useArrayDepthTexture").toBoolean();
}
std::shared_ptr<Texture> RenderTarget::TextureProperty::get() const {
    return owner->textures.empty() || owner->textures.storage()->holes.contains(0) ? nullptr : owner->textures[0];
}
Texture* RenderTarget::TextureProperty::operator->() const {
    const auto value = get();
    if (!value) throw JSTypeError("Cannot read properties of undefined (render target texture)");
    return value.get();
}
RenderTarget::TextureProperty& RenderTarget::TextureProperty::operator=(std::shared_ptr<Texture> value) {
    setAttachment(owner->textures, 0, std::move(value));
    return *this;
}
RenderTarget::DepthTextureProperty& RenderTarget::DepthTextureProperty::operator=(std::shared_ptr<Texture> current) {
    if (owner->_depthTexture && owner->_depthTexture->renderTarget == owner) owner->_depthTexture->renderTarget = nullptr;
    if (current && current->renderTarget == nullptr) current->renderTarget = owner;
    owner->_depthTexture = std::move(current);
    return *this;
}
void RenderTarget::_setTextureOptions(JSValue options) {
    auto values = JSValue::object();
    values.set("minFilter", LinearFilter); values.set("generateMipmaps", false);
    values.set("flipY", false); values.set("internalFormat", nullptr);
    for (const char* key : {"mapping", "wrapS", "wrapT", "wrapR", "magFilter", "minFilter", "format", "type",
             "anisotropy", "colorSpace", "flipY", "generateMipmaps", "mipmapsAutoUpdate", "internalFormat"}) {
        if (!options.get(key).isUndefined()) values.set(key, options.get(key));
    }
    for (std::size_t i = 0; i < textures.size(); ++i) attachment(*this, i)->setValues(values);
}
void RenderTarget::setSize(double w, double h, double d) {
    if (width != w || height != h || depth != d) {
        width = w; height = h; depth = d;
        const auto count = textures.size();
        for (std::size_t i = 0; i < count; ++i) {
            const auto imageFor = [&]() {
                const auto image = attachment(*this, i)->image.get();
                if (!image) throw JSTypeError("Cannot set properties of null (render target image)");
                return image;
            };
            imageFor()->width = w; imageFor()->height = h; imageFor()->depth = d;
            if (!attachment(*this, i)->isData3DTexture) attachment(*this, i)->isArrayTexture = imageFor()->depth > 1;
        }
        dispose();
    }
    viewport.set(0, 0, w, h);
    scissor.set(0, 0, w, h);
}
std::shared_ptr<RenderTarget> RenderTarget::clone() const {
    auto result = std::make_shared<RenderTarget>(); result->copy(*this); return result;
}
RenderTarget& RenderTarget::copy(const RenderTarget& other) {
    width = other.width; height = other.height; depth = other.depth;
    scissor.copy(other.scissor); scissorTest = other.scissorTest; viewport.copy(other.viewport);
    textures.clear();
    const auto count = other.textures.size();
    for (std::size_t i = 0; i < count; ++i) {
        // Evaluate the assignment's array reference before invoking clone.
        auto destination = textures;
        auto current = attachment(other, i)->clone();
        setAttachment(destination, i, std::move(current));
        attachment(*this, i)->isRenderTargetTexture = true;
        attachment(*this, i)->renderTarget = this;
        const auto sourceImage = attachment(other, i)->image.get();
        auto image = sourceImage ? std::make_shared<TextureImage>(*sourceImage) : std::make_shared<TextureImage>();
        image->isArray = false;
        attachment(*this, i)->source = std::make_shared<TextureSource>(std::move(image));
    }
    depthBuffer = other.depthBuffer; stencilBuffer = other.stencilBuffer;
    resolveColorBuffer = other.resolveColorBuffer; resolveDepthBuffer = other.resolveDepthBuffer;
    resolveStencilBuffer = other.resolveStencilBuffer;
    storeMultisampledColorBuffer = other.storeMultisampledColorBuffer;
    storeMultisampledDepthBuffer = other.storeMultisampledDepthBuffer;
    storeMultisampledStencilBuffer = other.storeMultisampledStencilBuffer;
    if (other._depthTexture) {
        if (other._depthTexture->renderTarget == &other) {
            auto copied = other._depthTexture->clone(); copied->renderTarget = nullptr; depthTexture = std::move(copied);
        } else depthTexture = other._depthTexture;
    } else depthTexture = nullptr;
    samples = other.samples; multiview = other.multiview; useArrayDepthTexture = other.useArrayDepthTexture;
    return *this;
}
void RenderTarget::dispose() { dispatchEvent(Event{"dispose"}); }
