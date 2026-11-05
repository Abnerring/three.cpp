#pragma once
#include "lights/LightShadow.h"
class DirectionalLightShadow : public LightShadow {
  public:
    const bool isDirectionalLightShadow = true;
    DirectionalLightShadow();
    std::shared_ptr<LightShadow> clone() const override;
};

JSValue nativeDirectionalLightShadowModule();
JSValue nativeDirectionalLightShadowBinding(const std::string& name);
JSValue nativeDirectionalLightShadowConstructor();
