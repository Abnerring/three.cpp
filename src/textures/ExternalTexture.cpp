#include "textures/ExternalTexture.h"
ExternalTexture::ExternalTexture(std::shared_ptr<void> nativeTexture) : sourceTexture(std::move(nativeTexture)) {
    isExternalTexture = true;
}
Texture& ExternalTexture::copy(const Texture& source) {
    Texture::copy(source); sourceTexture = dynamic_cast<const ExternalTexture&>(source).sourceTexture; return *this;
}
std::shared_ptr<Texture> ExternalTexture::clone() const {
    auto result = std::make_shared<ExternalTexture>(); result->copy(*this); return result;
}
