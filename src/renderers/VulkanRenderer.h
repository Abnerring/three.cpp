#ifndef THREECPP_VULKAN_RENDERER_H
#define THREECPP_VULKAN_RENDERER_H

#include "math/Color.h"
#include "core/JSValue.h"
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

class RenderTarget;
class Scene;
class Camera;

struct VulkanRendererParameters {
    std::uint32_t width = 256;
    std::uint32_t height = 256;
    bool validation = false;
};

struct VulkanRendererInfo {
    std::string deviceName;
    double frame = 0;
    double calls = 0;
    double triangles = 0;
    double lines = 0;
    double points = 0;
    std::uint64_t attributeBuffers = 0;
    std::uint64_t attributeUploadBuffers = 0;
    std::uint64_t textureImages = 0, textureUploads = 0;
    bool validationEnabled = false;
};

// Offscreen Vulkan graphics renderer. Pixel rows are returned from top to bottom.
// The scene API follows three.js; presentation/swapchains are not implemented yet.
class VulkanRenderer {
  public:
    explicit VulkanRenderer(const VulkanRendererParameters& parameters = {});
    ~VulkanRenderer();
    VulkanRenderer(const VulkanRenderer&) = delete;
    VulkanRenderer& operator=(const VulkanRenderer&) = delete;

    bool autoClear = true;
    bool autoClearColor = true;
    bool autoClearDepth = true;
    bool sortObjects = true;
    bool srgbOutput = true;
    JSValue clippingPlanes = JSValue::array();
    JSValue localClippingEnabled = false;

    // The same mutable statistics object exposed by WebGLRenderer. GPU draw
    // helpers retain the original object, as upstream closures do.
    JSValue info;

    void setSize(std::uint32_t width, std::uint32_t height);
    std::uint32_t getWidth() const;
    std::uint32_t getHeight() const;
    void setClearColor(const Color& color, HIGH_PRECISION alpha = 1);
    const Color& getClearColor() const;
    HIGH_PRECISION getClearAlpha() const;
    void clear(bool color = true, bool depth = true);
    void render(Scene& scene, Camera& camera);
    // Rendering into an offscreen target follows WebGL: the renderer temporarily takes
    // the target size, and restoring a null target restores the previous size.
    void setRenderTarget(std::shared_ptr<RenderTarget> target);
    std::shared_ptr<RenderTarget> getRenderTarget() const;
    // Rows are top to bottom, matching readPixels() on this renderer.
    std::vector<std::uint8_t> readRenderTargetPixels(RenderTarget& target, std::uint32_t x,
                                                  std::uint32_t y, std::uint32_t width,
                                                  std::uint32_t height);
    std::vector<std::uint8_t> readPixels() const;
    const VulkanRendererInfo& getInfo() const;
    std::vector<std::string> getValidationMessages() const;

  private:
    struct Impl;
    std::unique_ptr<Impl> impl;
};

#endif
