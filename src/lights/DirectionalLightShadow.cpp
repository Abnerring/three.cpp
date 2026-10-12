#include "lights/DirectionalLightShadow.h"
#include "cameras/OrthographicCamera.h"
DirectionalLightShadow::DirectionalLightShadow() : LightShadow(std::make_shared<OrthographicCamera>(-5, 5, 5, -5, 0.5, 500)) {}
std::shared_ptr<LightShadow> DirectionalLightShadow::clone() const { auto result = std::make_shared<DirectionalLightShadow>(); result->copy(*this); return result; }
