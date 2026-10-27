#pragma once
#include "textures/Texture.h"
JSValue nativeDepthTextureConstructor();
JSValue nativeDepthTextureModule();
JSValue nativeDepthTextureBinding(const std::string& name);
class DepthTexture : public Texture {
  public:
    std::optional<double> compareFunction;
    explicit DepthTexture(double width = std::numeric_limits<double>::quiet_NaN(),
                          double height = std::numeric_limits<double>::quiet_NaN(),
                          double type = UnsignedIntType, double mapping = DEFAULT_MAPPING,
                          double wrapS = ClampToEdgeWrapping, double wrapT = ClampToEdgeWrapping,
                          double magFilter = NearestFilter, double minFilter = NearestFilter,
                          double anisotropy = DEFAULT_ANISOTROPY, double format = DepthFormat, double depth = 1);
    Texture& copy(const Texture& source) override;
    std::shared_ptr<Texture> clone() const override;
    JsonValue toJSON(TextureJSONContext* meta = nullptr) const override;
};
