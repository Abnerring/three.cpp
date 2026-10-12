#pragma once
#include "textures/Texture.h"
class CompressedTexture : public Texture {
  public:
    explicit CompressedTexture(Mipmaps mipmaps = nullptr, double width = std::numeric_limits<double>::quiet_NaN(),
                               double height = std::numeric_limits<double>::quiet_NaN(), double format = RGBAFormat,
                               double type = UnsignedByteType, double mapping = DEFAULT_MAPPING,
                               double wrapS = ClampToEdgeWrapping, double wrapT = ClampToEdgeWrapping,
                               double magFilter = LinearFilter, double minFilter = LinearMipmapLinearFilter,
                               double anisotropy = DEFAULT_ANISOTROPY, std::string colorSpace = NoColorSpace);
    std::shared_ptr<Texture> clone() const override;
};
