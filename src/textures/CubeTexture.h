#pragma once
#include "textures/Texture.h"
class CubeTexture : public Texture {
  public:
    ImageProperty images{this};
    explicit CubeTexture(Image images = TextureImage::array(), double mapping = CubeReflectionMapping,
                         double wrapS = ClampToEdgeWrapping, double wrapT = ClampToEdgeWrapping,
                         double magFilter = LinearFilter, double minFilter = LinearMipmapLinearFilter,
                         double format = RGBAFormat, double type = UnsignedByteType,
                         double anisotropy = DEFAULT_ANISOTROPY, std::string colorSpace = NoColorSpace);
    std::shared_ptr<Texture> clone() const override;
};
