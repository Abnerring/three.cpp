#include "renderers/vulkan/CubeCameraVulkan.h"
#include "cameras/CubeCamera.h"
#include "cameras/PerspectiveCamera.h"
#include "core/RenderTarget.h"
#include "renderers/VulkanRenderer.h"
#include "scenes/Scene.h"

// Upstream CubeCamera.update, kept in a translation unit that sees every complete type so
// the camera header can stay free of templates and backend headers.
CubeCamera& updateCubeCamera(CubeCamera& camera, VulkanRenderer& renderer, Scene& scene) {
    if (camera.target == nullptr)
        throw std::logic_error("THREE.CubeCamera: update needs a render target");
    if (camera.parent == nullptr) camera.updateMatrixWorld();
    if (camera.coordinateSystem.toNumber() != renderer.coordinateSystem) {
        camera.coordinateSystem = renderer.coordinateSystem;
        camera.updateCoordinateSystem();
    }
    const auto currentTarget = renderer.getRenderTarget();
    const auto currentFace = renderer.getActiveCubeFace();
    const auto currentMip = renderer.getActiveMipmapLevel();
    for (std::uint32_t face = 0; face < 6; ++face) {
        auto eye = std::dynamic_pointer_cast<PerspectiveCamera>(camera.children[face]);
        if (!eye) break;
        eye->updateMatrixWorld();
        renderer.setRenderTarget(camera.target, face,
                                 static_cast<std::uint32_t>(camera.activeMipmapLevel));
        renderer.render(scene, *eye);
    }
    renderer.setRenderTarget(currentTarget, currentFace, currentMip);
    return camera;
}
