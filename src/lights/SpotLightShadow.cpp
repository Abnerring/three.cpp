#include "lights/SpotLightShadow.h"
#include "lights/SpotLight.h"
#include "cameras/PerspectiveCamera.h"
#include <cmath>
#include <numbers>

SpotLightShadow::SpotLightShadow() : LightShadow(std::make_shared<PerspectiveCamera>(50, 1, 0.5, 500)) {}
void SpotLightShadow::updateMatrices(Light& light) {
    auto* spot = dynamic_cast<SpotLight*>(&light);
    const auto retainedCamera = camera;
    auto* perspective = dynamic_cast<PerspectiveCamera*>(retainedCamera.get());
    if (!spot || !perspective) throw JSTypeError("SpotLightShadow requires a SpotLight and PerspectiveCamera");
    const double fov = (180 / std::numbers::pi) * 2 * spot->angle * focus;
    const double cameraAspect = (mapSize.x / mapSize.y) * aspect;
    const double far = spot->distance != 0 && !std::isnan(spot->distance) ? spot->distance : perspective->far;
    if (fov != perspective->fov || cameraAspect != perspective->aspect || far != perspective->far) {
        perspective->fov = fov; perspective->aspect = cameraAspect; perspective->far = far;
        perspective->updateProjectionMatrix();
    }
    LightShadow::updateMatrices(light);
}
SpotLightShadow& SpotLightShadow::copy(const LightShadow& source) {
    const auto* shadow = dynamic_cast<const SpotLightShadow*>(&source);
    if (!shadow) throw JSTypeError("SpotLightShadow::copy requires a SpotLightShadow");
    LightShadow::copy(source); focus = shadow->focus; aspect = shadow->aspect; return *this;
}
std::shared_ptr<LightShadow> SpotLightShadow::clone() const { auto result = std::make_shared<SpotLightShadow>(); result->copy(*this); return result; }
JsonValue SpotLightShadow::toJSON() const { auto object = LightShadow::toJSON(); object["focus"] = focus; object["aspect"] = aspect; return object; }
