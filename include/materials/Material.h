#pragma once
#include "constants.h"
#include "core/BufferGeometry.h"
#include "core/EventDispatcher.h"
#include "math/Color.h"
#include <atomic>
#include <cstdint>
#include <optional>

class Plane;
class VulkanRenderer;
class Scene;
class Camera;
class Object3D;
class Material : public EventDispatcher {
  public:
    struct UpdateFlag {
        std::uint64_t* version;
        void operator=(bool v) const {
            if (v)
                ++*version;
        }
    };
    struct AlphaTest {
        std::uint64_t* version;
        double value = 0;
        operator double() const { return value; }
        AlphaTest& operator=(const AlphaTest& other) { return *this = double(other); }
        AlphaTest& operator=(double v) {
            if ((value > 0) != (v > 0))
                ++*version;
            value = v;
            return *this;
        }
    };
    const bool isMaterial = true;
    bool isMeshBasicMaterial = false;
    const std::uint64_t id = nextId++;
    std::string uuid, name, type = "Material";
    int blending = NormalBlending, side = FrontSide;
    bool vertexColors = false;
    double opacity = 1;
    bool transparent = false, alphaHash = false;
    int blendSrc = SrcAlphaFactor, blendDst = OneMinusSrcAlphaFactor, blendEquation = AddEquation;
    std::optional<int> blendSrcAlpha, blendDstAlpha, blendEquationAlpha;
    Color blendColor{0.0, 0.0, 0.0};
    double blendAlpha = 0;
    int depthFunc = LessEqualDepth;
    bool depthTest = true, depthWrite = true;
    unsigned stencilWriteMask = 0xff;
    int stencilFunc = AlwaysStencilFunc, stencilRef = 0;
    unsigned stencilFuncMask = 0xff;
    int stencilFail = KeepStencilOp, stencilZFail = KeepStencilOp, stencilZPass = KeepStencilOp;
    bool stencilWrite = false;
    std::optional<std::vector<std::shared_ptr<Plane>>> clippingPlanes;
    bool clipIntersection = false, clipShadows = false;
    std::optional<int> shadowSide;
    bool colorWrite = true;
    std::optional<std::string> precision;
    bool polygonOffset = false;
    double polygonOffsetFactor = 0, polygonOffsetUnits = 0;
    bool dithering = false, alphaToCoverage = false, premultipliedAlpha = false,
         forceSinglePass = false;
    bool allowOverride = true, visible = true, toneMapped = true;
    JsonValue userData = JsonValue::Object{};
    std::function<void(VulkanRenderer&, Scene&, Camera&, BufferGeometry*, Object3D*,
                       BufferGeometry::Group*)>
        onBeforeRender;
    std::uint64_t version = 0;
    UpdateFlag needsUpdate{&version};
    AlphaTest alphaTest{&version};

    Material();
    virtual ~Material() = default;
    Material(const Material&) = delete;
    Material& operator=(const Material&) = delete;
    virtual void setValues(const JsonValue& parameters);
    virtual Material& copy(const Material& source);
    virtual std::shared_ptr<Material> clone() const;
    virtual JsonValue toJSON() const;
    void dispose();

  private:
    inline static std::atomic<std::uint64_t> nextId{0};
};
