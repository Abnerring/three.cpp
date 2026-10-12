#include "lights/PointLightShadow.h"
#include "cameras/PerspectiveCamera.h"
PointLightShadow::PointLightShadow() : LightShadow(std::make_shared<PerspectiveCamera>(90, 1, 0.5, 500)) {}
std::shared_ptr<LightShadow> PointLightShadow::clone() const { auto result = std::make_shared<PointLightShadow>(); result->copy(*this); return result; }
