// Differential probe for the native src/lights/DirectionalLightShadow.js and PointLightShadow.js ports.
//
// Each stdin line names a case: the two base cases print the shadow state with the camera it builds, and the
// two clone cases print the same for a clone. What matters about these classes is the camera each one
// chooses and the defaults it inherits, so that is what is compared.
#include "cameras/OrthographicCamera.h"
#include "cameras/PerspectiveCamera.h"
#include "core/JsonValue.h"
#include "lights/DirectionalLightShadow.h"
#include "lights/LightShadow.h"
#include "lights/PointLightShadow.h"
#include <cmath>
#include <iostream>
#include <memory>
#include <string>

namespace {
JsonValue encode(double value) {
    if (value == 0 && std::signbit(value)) return JsonValue("-0");
    if (std::isnan(value)) return JsonValue("NaN");
    if (std::isinf(value)) return JsonValue(value > 0 ? "Infinity" : "-Infinity");
    return JsonValue(value);
}
JsonValue shadowState(const LightShadow& shadow, bool directional) {
    const auto& camera = shadow.camera;
    JsonValue cameraState;
    if (directional) {
        const auto* orthographic = dynamic_cast<const OrthographicCamera*>(camera.get());
        cameraState = JsonValue(JsonValue::Object{
            {"left", encode(orthographic->left)}, {"right", encode(orthographic->right)},
            {"top", encode(orthographic->top)}, {"bottom", encode(orthographic->bottom)},
            {"near", encode(orthographic->near)}, {"far", encode(orthographic->far)}});
    } else {
        const auto* perspective = dynamic_cast<const PerspectiveCamera*>(camera.get());
        cameraState = JsonValue(JsonValue::Object{
            {"fov", encode(perspective->fov)}, {"aspect", encode(perspective->aspect)},
            {"near", encode(perspective->near)}, {"far", encode(perspective->far)}});
    }
    return JsonValue(JsonValue::Object{
        {"directional", directional},
        {"bias", encode(shadow.bias)}, {"normalBias", encode(shadow.normalBias)},
        {"radius", encode(shadow.radius)}, {"blurSamples", encode(shadow.blurSamples)},
        {"intensity", encode(shadow.intensity)},
        {"mapSizeX", encode(shadow.mapSize.x)}, {"mapSizeY", encode(shadow.mapSize.y)},
        {"camera", cameraState}});
}
}
int main() {
    std::string line;
    while (std::getline(std::cin, line)) {
        JsonValue result;
        if (line == "directional") { DirectionalLightShadow shadow; result = shadowState(shadow, true); }
        else if (line == "point") { PointLightShadow shadow; result = shadowState(shadow, false); }
        else if (line == "directional.clone") {
            DirectionalLightShadow shadow;
            const auto copy = shadow.clone();
            result = shadowState(*copy, true);
        } else if (line == "point.clone") {
            PointLightShadow shadow;
            const auto copy = shadow.clone();
            result = shadowState(*copy, false);
        } else result = JsonValue("unknown");
        std::cout << result.dump() << '\n';
    }
    return 0;
}