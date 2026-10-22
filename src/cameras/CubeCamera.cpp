#include "cameras/CubeCamera.h"

#include "cameras/PerspectiveCamera.h"
#include <stdexcept>

namespace {
// A negative field of view is not an error: it is how upstream asks for the cube face projection.
constexpr double kFov = -90.0;
constexpr double kAspect = 1.0;
}

CubeCamera::CubeCamera(double near, double far, JSValue renderTarget) : renderTarget(std::move(renderTarget)) {
    type = "CubeCamera";
    coordinateSystem = JSValue(nullptr);
    // The six faces are added in the upstream order: positive and negative x, y, then z.
    for (int face = 0; face < 6; face += 1) {
        auto camera = std::make_shared<PerspectiveCamera>(kFov, kAspect, near, far);
        camera->layers = layers;
        add(camera);
    }
}

CubeCamera& CubeCamera::updateCoordinateSystem() {
    // Every child is detached and re-added after being oriented, which is what upstream does so the ordering
    // is rebuilt rather than assumed.
    std::vector<std::shared_ptr<Object3D>> cameras(children.begin(), children.end());
    for (const auto& camera : cameras) remove(camera);
    if (cameras.size() < 6) return *this;
    const auto cameraPX = std::dynamic_pointer_cast<PerspectiveCamera>(cameras[0]);
    const auto cameraNX = std::dynamic_pointer_cast<PerspectiveCamera>(cameras[1]);
    const auto cameraPY = std::dynamic_pointer_cast<PerspectiveCamera>(cameras[2]);
    const auto cameraNY = std::dynamic_pointer_cast<PerspectiveCamera>(cameras[3]);
    const auto cameraPZ = std::dynamic_pointer_cast<PerspectiveCamera>(cameras[4]);
    const auto cameraNZ = std::dynamic_pointer_cast<PerspectiveCamera>(cameras[5]);
    if (!cameraPX || !cameraNX || !cameraPY || !cameraNY || !cameraPZ || !cameraNZ) return *this;

    const auto system = coordinateSystem.toNumber();
    if (system == WebGLCoordinateSystem) {
        cameraPX->up.set(0, 1, 0); cameraPX->lookAt(1, 0, 0);
        cameraNX->up.set(0, 1, 0); cameraNX->lookAt(-1, 0, 0);
        cameraPY->up.set(0, 0, -1); cameraPY->lookAt(0, 1, 0);
        cameraNY->up.set(0, 0, 1); cameraNY->lookAt(0, -1, 0);
        cameraPZ->up.set(0, 1, 0); cameraPZ->lookAt(0, 0, 1);
        cameraNZ->up.set(0, 1, 0); cameraNZ->lookAt(0, 0, -1);
    } else if (system == WebGPUCoordinateSystem) {
        cameraPX->up.set(0, -1, 0); cameraPX->lookAt(-1, 0, 0);
        cameraNX->up.set(0, -1, 0); cameraNX->lookAt(1, 0, 0);
        cameraPY->up.set(0, 0, 1); cameraPY->lookAt(0, 1, 0);
        cameraNY->up.set(0, 0, -1); cameraNY->lookAt(0, -1, 0);
        cameraPZ->up.set(0, -1, 0); cameraPZ->lookAt(0, 0, 1);
        cameraNZ->up.set(0, -1, 0); cameraNZ->lookAt(0, 0, -1);
    }
    // The rig is re-attached in the same order so the faces keep their positions in children.
    for (const auto& camera : cameras) add(camera);
    return *this;
}

CubeCamera& CubeCamera::update(JSValue renderer, JSValue scene) {
    static_cast<void>(renderer);
    static_cast<void>(scene);
    throw std::runtime_error("THREE.CubeCamera: update needs a renderer to bind its cube target");
}

CubeCamera& CubeCamera::clear(JSValue renderer) {
    static_cast<void>(renderer);
    throw std::runtime_error("THREE.CubeCamera: clear needs a renderer to bind its cube target");
}