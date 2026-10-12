#pragma once
#include "textures/Texture.h"
class DataTexture : public Texture {
  public:
    explicit DataTexture(TextureImage::Data data = nullptr, double width = 1, double height = 1,
                         double format = RGBAFormat, double type = UnsignedByteType, double mapping = DEFAULT_MAPPING,
                         double wrapS = ClampToEdgeWrapping, double wrapT = ClampToEdgeWrapping,
                         double magFilter = NearestFilter, double minFilter = NearestFilter,
                         double anisotropy = DEFAULT_ANISOTROPY, std::string colorSpace = NoColorSpace);
    std::shared_ptr<Texture> clone() const override;
};
