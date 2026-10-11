#ifndef THREECPP_VULKAN_RENDERER_H
#define THREECPP_VULKAN_RENDERER_H

#include "math/Color.h"
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

class Scene;
class Camera;

struct VulkanRendererParameters {
    std::uint32_t width = 256;
    std::uint32_t height = 256;
    bool validation = false;
};

struct VulkanRendererInfo {
    std::string deviceName;
    std::uint64_t frame = 0;
    std::uint64_t calls = 0;
    std::uint64_t triangles = 0;
    std::uint64_t lines = 0;
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

    void setSize(std::uint32_t width, std::uint32_t height);
    std::uint32_t getWidth() const;
    std::uint32_t getHeight() const;
    void setClearColor(const Color& color, HIGH_PRECISION alpha = 1);
    const Color& getClearColor() const;
    HIGH_PRECISION getClearAlpha() const;
    void clear(bool color = true, bool depth = true);
    void render(Scene& scene, Camera& camera);
    std::vector<std::uint8_t> readPixels() const;
    const VulkanRendererInfo& getInfo() const;
    std::vector<std::string> getValidationMessages() const;

  private:
    struct Impl;
    std::unique_ptr<Impl> impl;
};

#endif
