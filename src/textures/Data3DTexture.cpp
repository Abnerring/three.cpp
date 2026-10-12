#include "textures/Data3DTexture.h"
Data3DTexture::Data3DTexture(TextureImage::Data data, double width, double height, double depth) : Texture(nullptr) {
    isData3DTexture = true;
    image = std::make_shared<TextureImage>(std::move(data), width, height, depth);
    magFilter = NearestFilter; minFilter = NearestFilter;
    generateMipmaps = false; flipY = false; unpackAlignment = 1;
}
Texture& Data3DTexture::copy(const Texture& source) {
    Texture::copy(source); wrapR = dynamic_cast<const Data3DTexture&>(source).wrapR; return *this;
}
std::shared_ptr<Texture> Data3DTexture::clone() const {
    auto result = std::make_shared<Data3DTexture>(); result->copy(*this); return result;
}
