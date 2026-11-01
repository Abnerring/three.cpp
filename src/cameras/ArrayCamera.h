#pragma once
#include "cameras/PerspectiveCamera.h"
#include "core/SharedArray.h"

class ArrayCamera : public PerspectiveCamera {
  public:
    SharedArray<std::shared_ptr<Camera>> cameras;
    explicit ArrayCamera(SharedArray<std::shared_ptr<Camera>> array = {});
    std::shared_ptr<Object3D> clone(bool recursive = true) const override;

  protected:
    std::optional<JsonValue> propertyValue(const std::string& property) const override;
};

// Native managed module, separate from the typed renderer adapter above.
#include "core/JSModuleObject.h"
JSValue nativeArrayCameraModule();
JSValue nativeArrayCameraBinding(const std::string& name);
JSValue nativeArrayCameraConstructor();
inline constexpr JSModuleObject ArrayCameraModule{nativeArrayCameraModule};
