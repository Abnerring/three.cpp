#pragma once

class CubeCamera;
class Scene;
class VulkanRenderer;

// Upstream CubeCamera.update against this backend. Lives on the renderer side so the
// camera header needs neither templates nor backend headers. Throws when the camera has
// no typed target set.
CubeCamera& updateCubeCamera(CubeCamera& camera, VulkanRenderer& renderer, Scene& scene);
