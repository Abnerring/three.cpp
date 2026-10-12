#pragma once
#include "core/JSValue.h"
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <span>
#include <vector>

// Internal attribute command adapter, not a complete WebGL context. It accepts
// the ArrayBufferView bufferData(3 args) and bufferSubData(3/4/5 args) forms used
// by WebGLAttributes, with ARRAY_BUFFER / ELEMENT_ARRAY_BUFFER bindings.
// The owner must call shutdown before destroying its Vulkan device. Escaped JS
// handles become inert; no callback or buffer owns the renderer through JSValue.
class VulkanBufferCommands {
  public:
    struct Storage {
        virtual ~Storage() = default;
        // Each successful call commits immediately. On failure implementations
        // must leave storage bytes unchanged. Spans are valid only for the call.
        // Zero bytes are logical zero capacity; do not create a zero-size VkBuffer.
        virtual void setData(std::span<const std::byte> bytes) = 0;
        virtual void setSubData(std::size_t offset, std::span<const std::byte> bytes) = 0;
        virtual std::vector<std::byte> read() const = 0;
        virtual std::size_t size() const = 0;
    };
    // Native-only ownership: the factory and Storage must not hide JSValue roots.
    using Factory = std::function<std::shared_ptr<Storage>()>;
    explicit VulkanBufferCommands(Factory factory);
    JSValue object() const;
    std::uint64_t revision(const JSValue& buffer) const;
    std::vector<std::byte> read(const JSValue& buffer) const;
    void shutdown();
  private:
    JSValue state_, object_;
};
