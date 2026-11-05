#include "nodes/lighting/RectAreaLightNode.h"

#include <stdexcept>

#include "math/Matrix4.h"
#include "math/Vector3.h"
#include "nodes/accessors/Lights.h"
#include "nodes/accessors/TextureNode.h"
#include "nodes/core/UniformGroupNode.h"
#include "nodes/core/UniformNode.h"
#include "nodes/lighting/AnalyticLightNode.h"
#include "nodes/lighting/LightingEventNodeSources.inc"
#include "nodes/lighting/LightingNodeRuntime.h"

namespace {
using namespace LightingNodeRuntime;

JSWeakValue& lexical() { static thread_local JSWeakValue weak; return weak; }
// let _ltcLib = null;  filled by the static setLTC below.
JSWeakValue& ltcLibrary() { static thread_local JSWeakValue weak; return weak; }
// const _matrix41 = new Matrix4(); const _matrix42 = new Matrix4();
JSWeakValue& matrix41() { static thread_local JSWeakValue weak; return weak; }
JSWeakValue& matrix42() { static thread_local JSWeakValue weak; return weak; }

JSValue cachedMatrix(JSWeakValue& slot) {
    if (const auto cached = slot.lock(); cached && cached->toBoolean()) return *cached;
    const auto created = nativeMatrix4Constructor().construct();
    slot = JSWeakValue(created);
    return created;
}

JSValue renderUniform(const JSValue& value) {
    const auto uniform = nativeUniformFactory().call({}, {value});
    return invoke(uniform, "setGroup")({nativeRenderGroup()});
}

// update( frame ) { super.update( frame ); ... }
JSValue update(const JSValue& self, const Args& args, const Args&) {
    const auto light = self.get("light");
    if (light.isNull() || light.isUndefined()) return {};
    const auto frame = arg(args, 0, nullptr);

    // super.update( frame ): the analytic node's own update, called with this receiver.
    const auto analyticPrototype = nativeAnalyticLightNodeBinding("default").get("prototype");
    analyticPrototype.get("update").call(self, {frame});

    // const viewMatrix = frame.camera.matrixWorldInverse;
    const auto viewMatrix = frame.get("camera").get("matrixWorldInverse");
    const auto rotation = cachedMatrix(matrix41());
    const auto rotationOnly = cachedMatrix(matrix42());

    invoke(rotationOnly, "identity")();
    invoke(rotation, "copy")({light.get("matrixWorld")});
    invoke(rotation, "premultiply")({viewMatrix});
    invoke(rotationOnly, "extractRotation")({rotation});

    const auto width = light.get("width").toNumber() * 0.5;
    const auto height = light.get("height").toNumber() * 0.5;

    auto halfWidthValue = self.get("halfWidth").get("value");
    auto halfHeightValue = self.get("halfHeight").get("value");
    invoke(halfWidthValue, "set")({JSValue(width), JSValue(0.0), JSValue(0.0)});
    invoke(halfHeightValue, "set")({JSValue(0.0), JSValue(height), JSValue(0.0)});
    invoke(halfWidthValue, "applyMatrix4")({rotationOnly});
    invoke(halfHeightValue, "applyMatrix4")({rotationOnly});
    return {};
}

// setupDirectRectArea( builder ) { ... }
JSValue setupDirectRectArea(const JSValue& self, const Args& args, const Args&) {
    const auto builder = arg(args, 0, nullptr);
    const auto library = ltcLibrary().lock();
    if (!library || !library->toBoolean()) {
        // The source dereferences the library set by setLTC; failing loudly here says what is missing.
        throw std::runtime_error(
            "RectAreaLightNode::setupDirectRectArea: setLTC has not been called");
    }
    const bool filterable =
        invoke(builder, "isAvailable")({JSValue("float32Filterable")}).toBoolean();
    const auto first = library->get(filterable ? "LTC_FLOAT_1" : "LTC_HALF_1");
    const auto second = library->get(filterable ? "LTC_FLOAT_2" : "LTC_HALF_2");

    auto result = JSValue::object(nullptr);
    result.set("lightColor", self.get("colorNode"));
    result.set("lightPosition",
               nativeLightsBinding("lightViewPosition").call({}, {self.get("light")}));
    result.set("halfWidth", self.get("halfWidth"));
    result.set("halfHeight", self.get("halfHeight"));
    result.set("ltc_1", nativeTextureNodeBinding("texture").call({}, {first}));
    result.set("ltc_2", nativeTextureNodeBinding("texture").call({}, {second}));
    return result;
}

// static setLTC( ltc ) { _ltcLib = ltc; }
JSValue setLTC(const JSValue&, const Args& args, const Args&) {
    const auto ltc = arg(args, 0, nullptr);
    ltcLibrary() = JSWeakValue(ltc);
    return {};
}
}  // namespace

JSValue nativeRectAreaLightNodeModule() {
    using namespace LightingNodeRuntime;
    static thread_local JSWeakValue weak;
    if (const auto cached = weak.lock(); cached && cached->isObject()) return *cached;
    static thread_local bool initializing = false;
    JSModuleInitializationScope scope(initializing, "RectAreaLightNode module");
    nativeAnalyticLightNodeModule();
    nativeLightsModule();
    nativeTextureNodeModule();
    const auto base = nativeAnalyticLightNodeBinding("default");
    auto bindings = JSValue::object(nullptr);
    const auto ctor = makeClass(
        "RectAreaLightNode", 0, base,
        +[](const JSValue& target, const Args& args, const Args& captures) -> JSValue {
            auto self = superConstruct(target, captures, {arg(args, 0, nullptr)});
            self.set("halfHeight", renderUniform(nativeVector3Constructor().construct()));
            self.set("halfWidth", renderUniform(nativeVector3Constructor().construct()));
            // The light relies on the view matrix, which can change per render call.
            self.set("updateType", nativeNodeConstantsBinding("NodeUpdateType").get("RENDER"));
            return self;
        },
        {}, RectAreaLightNodeSource::classSource, RectAreaLightNodeSource::typeSource);
    const auto prototype = ctor.get("prototype");
    method(prototype, "update", 1, update, {}, RectAreaLightNodeSource::updateSource);
    method(prototype, "setupDirectRectArea", 1, setupDirectRectArea, {},
           RectAreaLightNodeSource::setupDirectRectAreaSource);
    auto setLTCFn = JSValue::function(setLTC, {}, "setLTC", 1);
    setLTCFn.setFunctionSourceText(RectAreaLightNodeSource::setLTCSource);
    // JSValue is a handle, so the copy still writes the property on the constructor itself.
    auto ctorRef = ctor;
    ctorRef.set("setLTC", setLTCFn);
    own(bindings, "default", ctor);
    return publish(bindings, lexical(), weak);
}

JSValue nativeRectAreaLightNodeBinding(const std::string& name) {
    const auto keeper = nativeRectAreaLightNodeModule();
    (void) keeper;
    return lexical().lock()->get(name);
}

JSValue nativeRectAreaLightNodeConstructor() {
    return nativeRectAreaLightNodeBinding("default");
}

RectAreaLightNode::RectAreaLightNode(const JSValue& light)
    : object_(constructor().construct({light})) {}
