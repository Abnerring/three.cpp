#pragma once
#include "../constants.h"
#include "core/EventDispatcher.h"
#include "core/JSValue.h"
#include "math/Matrix3.h"
#include "textures/TextureSource.h"
#include <functional>
#include <optional>

JSValue nativeTextureConstructor();

class RenderTarget;
class Texture : public EventDispatcher {
  public:
    using Image = TextureImage::Ptr;
    using Mipmaps = std::shared_ptr<std::vector<Image>>;
    struct UpdateRange { double start, count; };
    struct UpdateFlag {
        Texture* owner;
        bool pmrem = false;
        operator JSValue() const { return {}; }
        void operator=(const UpdateFlag&) const {}
        void operator=(const JSValue& value) const;
    };
    struct ImageProperty {
        Texture* owner;
        Image get() const;
        operator Image() const { return get(); }
        TextureImage* operator->() const { return get().get(); }
        ImageProperty& operator=(Image value);
        ImageProperty& operator=(const ImageProperty& value) { return *this = value.get(); }
    };
    struct Dimension {
        const Texture* owner;
        unsigned axis;
        operator double() const;
    };
    class UpdateCallback {
      public:
        UpdateCallback() = default;
        UpdateCallback(std::nullptr_t) {}
        UpdateCallback(std::function<void(Texture&)> callback) { *this = std::move(callback); }
        UpdateCallback& operator=(std::function<void(Texture&)> callback) {
            target = callback ? std::make_shared<std::function<void(Texture&)>>(std::move(callback)) : nullptr;
            return *this;
        }
        UpdateCallback& operator=(std::nullptr_t) { target.reset(); return *this; }
        explicit operator bool() const { return bool(target); }
        void operator()(Texture& texture) const { const auto retained = target; if (retained) (*retained)(texture); }
      private:
        std::shared_ptr<std::function<void(Texture&)>> target;
    };
    inline static Image DEFAULT_IMAGE = nullptr;
    inline static double DEFAULT_MAPPING = UVMapping;
    inline static double DEFAULT_ANISOTROPY = 1;
    const bool isTexture = true;
    bool isDataTexture = false, isDataArrayTexture = false, isData3DTexture = false;
    bool isCubeTexture = false, isDepthTexture = false, isCubeDepthTexture = false;
    bool isCompressedTexture = false, isCompressedArrayTexture = false, isCompressedCubeTexture = false;
    bool isFramebufferTexture = false, isExternalTexture = false;
    const std::uint64_t id = nextId++;
    std::string uuid, name;
    std::shared_ptr<TextureSource> source;
    ImageProperty image{this};
    Dimension width{this, 0}, height{this, 1}, depth{this, 2};
    Mipmaps mipmaps = std::make_shared<std::vector<Image>>();
    double mapping, channel = 0, wrapS, wrapT, magFilter, minFilter;
    double anisotropy;
    double format, type;
    std::optional<std::string> internalFormat;
    Vector2 offset{0, 0}, repeat{1, 1}, center{0, 0};
    double rotation = 0;
    bool matrixAutoUpdate = true;
    Matrix3 matrix;
    bool generateMipmaps = true, mipmapsAutoUpdate = true, premultiplyAlpha = false, flipY = true;
    double unpackAlignment = 4;
    std::string colorSpace;
    JSValue userData = JSValue::object();
    std::shared_ptr<std::vector<std::shared_ptr<UpdateRange>>> updateRanges =
        std::make_shared<std::vector<std::shared_ptr<UpdateRange>>>();
    double version = 0, pmremVersion = 0;
    UpdateCallback onUpdate;
    RenderTarget* renderTarget = nullptr;
    bool isRenderTargetTexture = false, isArrayTexture = false, isPMREMTexture = false, normalized = false;
    UpdateFlag needsUpdate{this}, needsPMREMUpdate{this, true};

    explicit Texture(Image image = DEFAULT_IMAGE, double mapping = DEFAULT_MAPPING,
                     double wrapS = ClampToEdgeWrapping, double wrapT = ClampToEdgeWrapping,
                     double magFilter = LinearFilter, double minFilter = LinearMipmapLinearFilter,
                     double format = RGBAFormat, double type = UnsignedByteType,
                     double anisotropy = DEFAULT_ANISOTROPY, std::string colorSpace = NoColorSpace);
    Texture(const Texture&) = delete;
    Texture& operator=(const Texture&) = delete;
    virtual void updateMatrix();
    void addUpdateRange(double start, double count);
    void clearUpdateRanges();
    virtual std::shared_ptr<Texture> clone() const;
    virtual Texture& copy(const Texture& source);
    virtual void setValues(JSValue values);
    virtual JsonValue toJSON(TextureJSONContext* meta = nullptr) const;
    void dispose();
    virtual Vector2& transformUv(Vector2& uv) const;
  private:
    inline static std::atomic<std::uint64_t> nextId{0};
};
