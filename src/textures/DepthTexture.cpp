#include "textures/DepthTexture.h"
namespace {
Texture::Image depthImage(double width, double height, double depth, double format) {
    if (format != DepthFormat && format != DepthStencilFormat)
        throw std::invalid_argument("THREE.DepthTexture: format must be either THREE.DepthFormat or THREE.DepthStencilFormat");
    return std::make_shared<TextureImage>(width, height, depth);
}
}
DepthTexture::DepthTexture(double width, double height, double type, double mapping, double wrapS, double wrapT,
                          double magFilter, double minFilter, double anisotropy, double format, double depth)
    : Texture(depthImage(width, height, depth, format), mapping, wrapS, wrapT, magFilter, minFilter, format, type, anisotropy) {
    isDepthTexture = true; flipY = false; generateMipmaps = false;
}
Texture& DepthTexture::copy(const Texture& other) {
    Texture::copy(other);
    auto descriptor = other.image.get() ? std::make_shared<TextureImage>(*other.image.get()) : std::make_shared<TextureImage>();
    descriptor->isArray = false; // Object.assign({}, array) keeps indexed fields but loses Array branding.
    source = std::make_shared<TextureSource>(std::move(descriptor));
    compareFunction = dynamic_cast<const DepthTexture&>(other).compareFunction;
    return *this;
}
std::shared_ptr<Texture> DepthTexture::clone() const {
    auto result = std::make_shared<DepthTexture>(); result->copy(*this); return result;
}
JsonValue DepthTexture::toJSON(TextureJSONContext* meta) const {
    auto result = Texture::toJSON(meta);
    result["compareFunction"] = compareFunction ? JsonValue(*compareFunction) : JsonValue{};
    return result;
}
