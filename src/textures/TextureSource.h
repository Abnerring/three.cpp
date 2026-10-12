#pragma once
#include "core/JsonValue.h"
#include "core/TypedArray.h"
#include "math/Vector2.h"
#include "math/Vector3.h"
#include <atomic>
#include <limits>
#include <memory>
#include <variant>

class Texture;
struct TextureJSONContext {
    OrderedMap<JsonValue> textures, images;
};

// Native image descriptors retain the identity of the image and of its pixel view.
// Browser image/video objects require a native host implementation separately.
struct TextureImage {
    using Data = std::variant<std::nullptr_t, Int8Array, Uint8Array, Int16Array, Uint16Array,
                              Int32Array, Uint32Array, Float32Array, Float64Array,
                              Uint8ClampedArray, JSValue>;
    using Ptr = std::shared_ptr<TextureImage>;
    using Faces = std::vector<Ptr>;
    Data data = nullptr;
    double width = std::numeric_limits<double>::quiet_NaN();
    double height = std::numeric_limits<double>::quiet_NaN();
    double depth = std::numeric_limits<double>::quiet_NaN();
    bool isArray = false;
    Faces faces;
    // A cube face may be a DataTexture, rather than its image descriptor.
    std::shared_ptr<Texture> texture;
    TextureImage() = default;
    TextureImage(double width, double height,
                 double depth = std::numeric_limits<double>::quiet_NaN())
        : width(width), height(height), depth(depth) {}
    TextureImage(Data data, double width, double height,
                 double depth = std::numeric_limits<double>::quiet_NaN())
        : data(std::move(data)), width(width), height(height), depth(depth) {}
    static Ptr array(Faces faces = {});
};

class TextureSource {
  public:
    struct UpdateFlag {
        TextureSource* owner;
        operator JSValue() const { return {}; }
        void operator=(const UpdateFlag&) const {}
        void operator=(const JSValue& value) const;
    };
    const bool isTextureSource = true;
    const std::uint64_t id = nextId++;
    std::string uuid;
    TextureImage::Ptr data;
    bool dataReady = true;
    double version = 0;
    UpdateFlag needsUpdate{this};
    explicit TextureSource(TextureImage::Ptr data = nullptr);
    virtual ~TextureSource() = default;
    TextureSource(const TextureSource&) = delete;
    TextureSource& operator=(const TextureSource&) = delete;
    virtual Vector2& getSize(Vector2& target) const;
    virtual Vector3& getSize(Vector3& target) const;
    virtual JsonValue toJSON(TextureJSONContext* meta = nullptr) const;
  private:
    inline static std::atomic<std::uint64_t> nextId{0};
};

class Source : public TextureSource {
  public:
    const bool isSource = true;
    explicit Source(TextureImage::Ptr data = nullptr);
};
