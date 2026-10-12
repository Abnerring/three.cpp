#include "textures/CompressedTexture.h"
CompressedTexture::CompressedTexture(Mipmaps levels, double width, double height, double format, double type,
                                     double mapping, double wrapS, double wrapT, double magFilter, double minFilter,
                                     double anisotropy, std::string colorSpace)
    : Texture(nullptr, mapping, wrapS, wrapT, magFilter, minFilter, format, type, anisotropy, std::move(colorSpace)) {
    isCompressedTexture = true;
    image = std::make_shared<TextureImage>(width, height);
    mipmaps = std::move(levels); flipY = false; generateMipmaps = false;
}
std::shared_ptr<Texture> CompressedTexture::clone() const {
    auto result = std::make_shared<CompressedTexture>(); result->copy(*this); return result;
}
