#pragma once
#include "core/JSValue.h"
#include <array>
#include <cstdint>
#include <vector>

// Native vertex-input command state consumed by the Vulkan draw backend.
// No OpenGL context is created. Managed records own every referenced buffer;
// the opaque VAO payload contains only an identity and a weak context handle.
class VulkanBindingCommands {
  public:
    struct Attribute {
        bool enabled = false, normalized = false, integer = false;
        std::uint32_t size = 4, type = 0x1406, divisor = 0;
        std::uint64_t stride = 0, offset = 0;
        JSValue buffer{nullptr};
        std::array<float,4> value{0,0,0,1};
    };
    struct Snapshot {
        std::vector<Attribute> attributes;
        JSValue index{nullptr};
    };
    VulkanBindingCommands(JSValue buffers, JSValue attachmentInterface, std::uint32_t maximumAttributes);
    JSValue object() const;
    Snapshot snapshot() const;
    void invalidDraw();
    void shutdown();
  private:
    JSValue state_, object_;
};
