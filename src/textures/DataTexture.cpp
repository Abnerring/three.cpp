#include "textures/DataTexture.h"
DataTexture::DataTexture(TextureImage::Data data, double width, double height, double format, double type,
                         double mapping, double wrapS, double wrapT, double magFilter, double minFilter,
                         double anisotropy, std::string colorSpace)
    : Texture(nullptr, mapping, wrapS, wrapT, magFilter, minFilter, format, type, anisotropy, std::move(colorSpace)) {
    isDataTexture = true;
    image = std::make_shared<TextureImage>(std::move(data), width, height);
    generateMipmaps = false; flipY = false; unpackAlignment = 1;
}
std::shared_ptr<Texture> DataTexture::clone() const {
    auto result = std::make_shared<DataTexture>(); result->copy(*this); return result;
}
