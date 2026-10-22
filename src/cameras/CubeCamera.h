#pragma once
#include "cameras/PerspectiveCamera.h"
#include "core/Object3D.h"
#include <memory>

class RenderTarget;

// Port of src/cameras/CubeCamera.js at the pinned revision, the camera rig only.
//
// The portable half is the rig itself: six perspective cameras at a negative ninety degree field of view and
// unit aspect, sharing this object layers, added as children, and re-oriented by updateCoordinateSystem to match
// the handedness of the renderer it will draw with. The half that is not portable is update and clear, which
// read and restore render target state on a renderer; those refuse by name here rather than pretending.
class CubeCamera : public Object3D {
  public:
    CubeCamera(double near = 0.1, double far = 2000, JSValue renderTarget = {});

    CubeCamera(double near, double far, std::shared_ptr<RenderTarget> renderTarget);

    CubeCamera& updateCoordinateSystem();
    // Upstream walks the six eyes, reorienting first if the renderer draws with a
    // different handedness, then renders the scene into each face and restores whatever
    // target was bound before. The renderer type is a template parameter so this header
    // stays independent of the backend.
    template <class Renderer, class Scene>
    CubeCamera& update(Renderer& renderer, Scene& scene) {
        if (parent == nullptr) updateMatrixWorld();
        if (coordinateSystem.toNumber() != renderer.coordinateSystem) {
            coordinateSystem = renderer.coordinateSystem;
            updateCoordinateSystem();
        }
        const auto currentTarget = renderer.getRenderTarget();
        const auto currentFace = renderer.getActiveCubeFace();
        const auto currentMip = renderer.getActiveMipmapLevel();
        for (std::uint32_t face = 0; face < 6; ++face) {
            auto eye = std::dynamic_pointer_cast<PerspectiveCamera>(children[face]);
            if (!eye) break;
            eye->updateMatrixWorld();
            renderer.setRenderTarget(renderTarget, face,
                                     static_cast<std::uint32_t>(activeMipmapLevel));
            renderer.render(scene, *eye);
        }
        renderer.setRenderTarget(currentTarget, currentFace, currentMip);
        return *this;
    }
    // A JS renderer handle cannot be unwrapped, so these report instead of guessing.
    CubeCamera& update(JSValue renderer = {}, JSValue scene = {});
    CubeCamera& clear(JSValue renderer = {});

    std::shared_ptr<RenderTarget> renderTarget;
    JSValue coordinateSystem;
    double activeMipmapLevel = 0;
};