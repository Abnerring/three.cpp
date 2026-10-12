#include "textures/FramebufferTexture.h"
FramebufferTexture::FramebufferTexture(double width, double height) : Texture(std::make_shared<TextureImage>(width, height)) {
    isFramebufferTexture = true;
    magFilter = NearestFilter; minFilter = NearestFilter; generateMipmaps = false;
    needsUpdate = true;
}
Texture& FramebufferTexture::copy(const Texture& other) {
    Texture::copy(other);
    auto descriptor = other.image.get() ? std::make_shared<TextureImage>(*other.image.get()) : std::make_shared<TextureImage>();
    descriptor->isArray = false;
    source = std::make_shared<TextureSource>(std::move(descriptor));
    return *this;
}
std::shared_ptr<Texture> FramebufferTexture::clone() const {
    auto result = std::make_shared<FramebufferTexture>(); result->copy(*this); return result;
}
