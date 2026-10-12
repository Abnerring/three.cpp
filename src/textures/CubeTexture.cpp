#include "textures/CubeTexture.h"
CubeTexture::CubeTexture(Image images, double mapping, double wrapS, double wrapT, double magFilter, double minFilter,
                         double format, double type, double anisotropy, std::string colorSpace)
    : Texture(std::move(images), mapping, wrapS, wrapT, magFilter, minFilter, format, type, anisotropy, std::move(colorSpace)) {
    isCubeTexture = true; flipY = false;
}
std::shared_ptr<Texture> CubeTexture::clone() const {
    auto result = std::make_shared<CubeTexture>(); result->copy(*this); return result;
}
