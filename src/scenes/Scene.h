#pragma once
#include "core/Object3D.h"
#include "math/Color.h"
#include "scenes/Fog.h"

class Texture;

class Scene : public Object3D {
  public:
    // Texture backgrounds still require the native background value/renderer path.
    std::optional<Color> background;
    std::shared_ptr<Texture> environment;
    std::shared_ptr<FogBase> fog;
    HIGH_PRECISION backgroundBlurriness = 0, backgroundIntensity = 1, environmentIntensity = 1;
    Euler backgroundRotation, environmentRotation;
    std::shared_ptr<Material> overrideMaterial;

    Scene() {
        isScene = true;
        type = "Scene";
    }
    Scene& copy(const Object3D& source, bool recursive = true) override;
    std::shared_ptr<Object3D> clone(bool recursive = true) const override;
    using Object3D::toJSON;
    JsonValue toJSON(Object3DJSONContext* meta = nullptr) const override;

  protected:
    void serializeObjectResources(Object3DJSONContext& meta, JsonValue& object) const override;
    std::optional<JsonValue> propertyValue(const std::string& property) const override;
};
