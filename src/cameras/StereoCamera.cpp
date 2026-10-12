#include "cameras/StereoCamera.h"
#include "math/MathUtils.h"
#include <cmath>

namespace {
// The upstream module shares these across instances, including cache hits.
thread_local Matrix4 eyeRight, eyeLeft, projectionMatrix;
}
StereoCamera::StereoCamera()
    : cameraL(std::make_shared<PerspectiveCamera>()), cameraR(std::make_shared<PerspectiveCamera>()) {
    cameraL->layers.enable(1);
    cameraL->matrixAutoUpdate = false;
    cameraR->layers.enable(2);
    cameraR->matrixAutoUpdate = false;
}
void StereoCamera::update(const PerspectiveCamera& camera) {
    auto& cache = _cache;
    const bool needsUpdate = !cache.focus.strictEquals(camera.focus) || !cache.fov.strictEquals(camera.fov) ||
        !cache.aspect.strictEquals(camera.aspect * aspect) || !cache.near.strictEquals(camera.near) ||
        !cache.far.strictEquals(camera.far) || !cache.zoom.strictEquals(camera.zoom) || !cache.eyeSep.strictEquals(eyeSep);
    if (needsUpdate) {
        cache.focus = camera.focus; cache.fov = camera.fov; cache.aspect = camera.aspect * aspect;
        cache.near = camera.near; cache.far = camera.far; cache.zoom = camera.zoom; cache.eyeSep = eyeSep;
        projectionMatrix.copy(camera.projectionMatrix);
        const auto eyeSepHalf = cache.eyeSep.toNumber() / 2;
        const auto eyeSepOnProjection = eyeSepHalf * cache.near.toNumber() / cache.focus.toNumber();
        const auto ymax = (cache.near.toNumber() * std::tan((MATH_PI / 180) * cache.fov.toNumber() * .5)) / cache.zoom.toNumber();
        eyeLeft.elements()[12] = -eyeSepHalf;
        eyeRight.elements()[12] = eyeSepHalf;
        auto xmin = -ymax * cache.aspect.toNumber() + eyeSepOnProjection;
        auto xmax = ymax * cache.aspect.toNumber() + eyeSepOnProjection;
        projectionMatrix.elements()[0] = 2 * cache.near.toNumber() / (xmax - xmin);
        projectionMatrix.elements()[8] = (xmax + xmin) / (xmax - xmin);
        cameraL->projectionMatrix.copy(projectionMatrix);
        xmin = -ymax * cache.aspect.toNumber() - eyeSepOnProjection;
        xmax = ymax * cache.aspect.toNumber() - eyeSepOnProjection;
        projectionMatrix.elements()[0] = 2 * cache.near.toNumber() / (xmax - xmin);
        projectionMatrix.elements()[8] = (xmax + xmin) / (xmax - xmin);
        cameraR->projectionMatrix.copy(projectionMatrix);
    }
    cameraL->matrix.copy(camera.matrixWorld).multiply(eyeLeft);
    cameraL->matrixWorldNeedsUpdate = true;
    cameraR->matrix.copy(camera.matrixWorld).multiply(eyeRight);
    cameraR->matrixWorldNeedsUpdate = true;
}
