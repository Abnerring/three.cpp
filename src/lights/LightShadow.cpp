#include "lights/LightShadow.h"
#include "lights/Light.h"
#include "core/RenderTarget.h"
#include <stdexcept>

LightShadow::LightShadow(std::shared_ptr<Camera> camera) : camera(std::move(camera)) {}
std::shared_ptr<Camera> LightShadow::getCamera(std::size_t) const { return camera; }
void LightShadow::updateMatrices(Light& light) {
    const auto shadowCamera = camera;
    if (!shadowCamera) throw JSTypeError("LightShadow.camera is not a Camera");
    Vector3 lightPositionWorld, lookTarget;
    lightPositionWorld.setFromMatrixPosition(light.matrixWorld);
    shadowCamera->position.copy(lightPositionWorld);
    const auto* target = light.shadowTarget();
    if (!target) throw JSTypeError("Cannot read properties of undefined (reading 'matrixWorld')");
    lookTarget.setFromMatrixPosition(target->matrixWorld);
    shadowCamera->lookAt(lookTarget);
    shadowCamera->updateMatrixWorld();
    _updateMatrix(*shadowCamera, matrix, _frustum);
}
void LightShadow::_updateMatrix(Camera& shadowCamera, Matrix4& shadowMatrix, Frustum& frustum, const Vector4* viewport) {
    Matrix4 projScreenMatrix;
    projScreenMatrix.multiplyMatrices(shadowCamera.projectionMatrix, shadowCamera.matrixWorldInverse);
    frustum.setFromProjectionMatrix(projScreenMatrix, shadowCamera.coordinateSystem, shadowCamera.reversedDepth);
    const double scaleX = viewport ? viewport->z / _frameExtents.x : 1;
    const double scaleY = viewport ? viewport->w / _frameExtents.y : 1;
    const double offsetX = viewport ? viewport->x / _frameExtents.x : 0;
    const double offsetY = viewport ? viewport->y / _frameExtents.y : 0;
    if (shadowCamera.coordinateSystem == WebGPUCoordinateSystem || shadowCamera.reversedDepth) {
        shadowMatrix.set(0.5 * scaleX, 0, 0, 0.5 * scaleX + offsetX,
                         0, 0.5 * scaleY, 0, 0.5 * scaleY + offsetY,
                         0, 0, 1, 0, 0, 0, 0, 1);
    } else {
        shadowMatrix.set(0.5 * scaleX, 0, 0, 0.5 * scaleX + offsetX,
                         0, 0.5 * scaleY, 0, 0.5 * scaleY + offsetY,
                         0, 0, 0.5, 0.5, 0, 0, 0, 1);
    }
    shadowMatrix.multiply(projScreenMatrix);
}
void LightShadow::dispose() {
    // Fetch mapPass after map disposal: listeners may replace it reentrantly.
    if (const auto retained = map) retained->dispose();
    if (const auto retained = mapPass) retained->dispose();
}
LightShadow& LightShadow::copy(const LightShadow& source) {
    const auto sourceCamera = source.camera;
    if (!sourceCamera) throw JSTypeError("Cannot read properties of undefined (reading 'clone')");
    camera = std::dynamic_pointer_cast<Camera>(sourceCamera->clone());
    intensity = source.intensity; bias = source.bias; radius = source.radius;
    autoUpdate = source.autoUpdate; needsUpdate = source.needsUpdate;
    normalBias = source.normalBias; blurSamples = source.blurSamples;
    mapSize.copy(source.mapSize); biasNode = source.biasNode;
    return *this;
}
std::shared_ptr<LightShadow> LightShadow::clone() const { auto result = std::make_shared<LightShadow>(); result->copy(*this); return result; }
JsonValue LightShadow::toJSON() const {
    JsonValue object = JsonValue::Object{{"intensity", intensity}, {"bias", bias}, {"normalBias", normalBias}, {"radius", radius}, {"blurSamples", blurSamples}, {"mapSize", JsonValue::Array{mapSize.x, mapSize.y}}};
    const auto retainedCamera = camera;
    if (!retainedCamera) throw JSTypeError("Cannot read properties of undefined (reading 'toJSON')");
    object["camera"] = retainedCamera->toJSON(false).at("object");
    object["camera"].get<JsonValue::Object>().erase("matrix");
    return object;
}
