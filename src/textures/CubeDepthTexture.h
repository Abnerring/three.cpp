#pragma once
#include "textures/DepthTexture.h"
class CubeDepthTexture : public DepthTexture {
  public:
    ImageProperty images{this};
    explicit CubeDepthTexture(double size = std::numeric_limits<double>::quiet_NaN(), double type = UnsignedIntType,
                              double mapping = CubeReflectionMapping, double wrapS = ClampToEdgeWrapping,
                              double wrapT = ClampToEdgeWrapping, double magFilter = NearestFilter,
                              double minFilter = NearestFilter, double anisotropy = DEFAULT_ANISOTROPY, double format = DepthFormat);
    std::shared_ptr<Texture> clone() const override;
};
