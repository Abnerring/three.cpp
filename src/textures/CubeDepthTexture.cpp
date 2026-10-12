#include "textures/CubeDepthTexture.h"
CubeDepthTexture::CubeDepthTexture(double size, double type, double mapping, double wrapS, double wrapT, double magFilter,
                                  double minFilter, double anisotropy, double format)
    : DepthTexture(size, size, type, mapping, wrapS, wrapT, magFilter, minFilter, anisotropy, format) {
    auto face = std::make_shared<TextureImage>(size, size, 1);
    image = TextureImage::array(TextureImage::Faces(6, face));
    isCubeDepthTexture = true; isCubeTexture = true;
}
std::shared_ptr<Texture> CubeDepthTexture::clone() const {
    auto result = std::make_shared<CubeDepthTexture>(); result->copy(*this); return result;
}
