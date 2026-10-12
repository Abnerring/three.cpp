#pragma once
#include "constants.h"
#include "core/BufferGeometry.h"
#include "core/EventDispatcher.h"
#include "math/Color.h"
#include "math/Vector2.h"
#include "math/Euler.h"
#include <atomic>
#include <cstdint>
#include <optional>
#include <string_view>
#include <variant>

class Plane;
class VulkanRenderer;
class Scene;
class Camera;
class Object3D;
class Texture;
struct TextureJSONContext;
class Material : public EventDispatcher {
  public:
    struct UpdateFlag {
        double* version;
        void operator=(bool v) const {
            if (v)
                ++*version;
        }
        void operator=(const JSValue& v) const {
            if (v.strictEquals(true)) ++*version;
        }
        template<class T> void operator=(const T&) const {}
    };
    struct AlphaTest {
        double* version;
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
    struct Reflectivity {
        double* ior;
        operator double() const;
        Reflectivity& operator=(double value);
        Reflectivity& operator=(const Reflectivity& value) { return *this = double(value); }
    };
    using ClippingPlanes = std::optional<std::vector<std::shared_ptr<Plane>>>;
    // Every pointer denotes the authoritative native field. Adapters may use
    // these addresses for live properties and ownership edges, never snapshots.
    using PropertyReference = std::variant<bool*, int*, unsigned*, double*, std::string*,
        std::optional<int>*, std::optional<std::string>*, Color*, Vector2*, Euler*,
        std::shared_ptr<Texture>*, JsonValue*, JSValue*, AlphaTest*, Reflectivity*, ClippingPlanes*>;
    using PropertyVisitor = std::function<void(std::string_view, PropertyReference)>;
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
    double stencilWriteMask = 0xff;
    int stencilFunc = AlwaysStencilFunc, stencilRef = 0;
    double stencilFuncMask = 0xff;
    int stencilFail = KeepStencilOp, stencilZFail = KeepStencilOp, stencilZPass = KeepStencilOp;
    bool stencilWrite = false;
    ClippingPlanes clippingPlanes;
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
    // Storage for properties created dynamically by fromJSON or runtime callers.
    // Declared native fields are never duplicated here.
    JSValue extraProperties = JSValue::object();
    std::function<void(VulkanRenderer&, Scene&, Camera&, BufferGeometry*, Object3D*,
                       BufferGeometry::Group*)>
        onBeforeRender;
    double version = 0;
    UpdateFlag needsUpdate{&version};
    AlphaTest alphaTest{&version};

    Material();
    virtual ~Material() = default;
    Material(const Material&) = delete;
    Material& operator=(const Material&) = delete;
    virtual void setValues(const JsonValue& parameters);
    virtual void visitProperties(const PropertyVisitor& visitor);
    std::optional<PropertyReference> property(std::string_view key);
    virtual Material& copy(const Material& source);
    virtual std::shared_ptr<Material> clone() const;
    virtual JsonValue toJSON(TextureJSONContext* meta = nullptr) const;
    virtual Material& fromJSON(const JsonValue& json,
        const OrderedMap<std::shared_ptr<Texture>>& textures = {});
    void dispose();

  protected:
    JsonValue serializeCommonJSON() const;
    void serializeClippingPlanesJSON(JsonValue& result) const;
    void serializeUserDataJSON(JsonValue& result) const;

  private:
    inline static std::atomic<std::uint64_t> nextId{0};
};
