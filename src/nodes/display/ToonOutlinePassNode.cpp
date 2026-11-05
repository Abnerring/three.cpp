#include "nodes/display/ToonOutlinePassNode.h"

#include "constants.h"
#include "math/Color.h"
#include "core/JSMap.h"
#include "core/JSWeakMap.h"
#include "materials/nodes/NodeMaterial.h"
#include "nodes/accessors/Camera.h"
#include "nodes/accessors/ModelNode.h"
#include "nodes/accessors/Normal.h"
#include "nodes/accessors/Position.h"
#include "nodes/display/PassNode.h"
#include "nodes/lighting/LightingNodeRuntime.h"
#include "nodes/math/MathNode.h"
#include "nodes/tsl/TSLBase.h"

namespace {
using namespace LightingNodeRuntime;
JSWeakValue& lexical() { static thread_local JSWeakValue weak; return weak; }

// _createMaterial() { const material = new NodeMaterial(); ... TSL outline graph ... }
JSValue createOutlineMaterialFor(const JSValue& self) {
    auto material = nativeNodeMaterialConstructor().construct({});
    material.set("isMeshToonOutlineMaterial", true);
    material.set("name", JSValue("Toon_Outline"));
    material.set("side", JSValue(static_cast<double>(BackSide)));

    const auto outlineNormal = invoke(nativeNormalBinding("normalLocal"), "negate")();
    const auto mvp = invoke(nativeCameraBinding("cameraProjectionMatrix"), "mul")(
        {nativeModelNodeBinding("modelViewMatrix")});
    const auto ratio = nativeTSLBaseBinding("float").call({}, {JSValue(1.0)});
    const auto positionLocal = nativePositionBinding("positionLocal");
    const auto vertexPosition = nativeTSLBaseBinding("vec4").call({}, {positionLocal, JSValue(1.0)});
    const auto pos = invoke(mvp, "mul")({vertexPosition});
    const auto biased = invoke(positionLocal, "add")({outlineNormal});
    const auto biasedPosition = nativeTSLBaseBinding("vec4").call({}, {biased, JSValue(1.0)});
    const auto pos2 = invoke(mvp, "mul")({biasedPosition});
    const auto norm = nativeMathNodeBinding("normalize").call({}, {invoke(pos, "sub")({pos2})});

    auto vertexNode = invoke(norm, "mul")({self.get("thicknessNode")});
    vertexNode = invoke(vertexNode, "mul")({pos.get("w")});
    vertexNode = invoke(vertexNode, "mul")({ratio});
    material.set("vertexNode", invoke(pos, "add")({vertexNode}));
    material.set("colorNode",
                 nativeTSLBaseBinding("vec4").call({}, {self.get("colorNode"), self.get("alphaNode")}));
    return material;
}

JSValue getOutlineMaterial(const JSValue& self, const JSValue& originalMaterial);

// _getOutlineMaterial( originalMaterial ) { if ( ! originalMaterial.positionNode ) return this._outlineMaterial; ... }
JSValue getOutlineMaterialMethod(const JSValue& self, const Args& args, const Args&) {
    return getOutlineMaterial(self, arg(args, 0, nullptr));
}

JSValue getOutlineMaterial(const JSValue& self, const JSValue& originalMaterial) {
    if (!originalMaterial.get("positionNode").toBoolean()) return self.get("_outlineMaterial");
    const auto cache = self.get("_materialCache");
    const auto cached = cache.get("get").call(cache, {originalMaterial});
    bool force = false;
    auto outlineMaterial = cached;
    if (!outlineMaterial.toBoolean()) {
        outlineMaterial = self.get("_createMaterial").call(self, {});
        cache.get("set").call(cache, {originalMaterial, outlineMaterial});
        const auto onDispose = self.get("_onMaterialDispose");
        originalMaterial.get("addEventListener").call(originalMaterial, {JSValue("dispose"), onDispose});
        force = true;
    }
    const auto version = originalMaterial.get("version");
    if (!outlineMaterial.get("version").strictEquals(version) || force) {
        outlineMaterial.set("positionNode", originalMaterial.get("positionNode"));
        outlineMaterial.set("version", version);
    }
    return outlineMaterial;
}

// dispose() { super.dispose(); this._outlineMaterial.dispose(); for ( const [ original, outline ] of cache ) { ... } }
JSValue createOutlineMaterialMethod(const JSValue& self, const Args&, const Args&) {
    return createOutlineMaterialFor(self);
}

JSValue disposeNode(const JSValue& rawSelf, const Args&, const Args&) {
    JSValue& self = const_cast<JSValue&>(rawSelf);
    const auto basePrototype = nativePassNodeConstructor().get("prototype");
    const auto baseDispose = basePrototype.get("dispose");
    if (baseDispose.toBoolean()) baseDispose.call(self, {});
    self.get("_outlineMaterial").get("dispose").call(self.get("_outlineMaterial"), {});
    const auto cache = self.get("_materialCache");
    const auto onDispose = self.get("_onMaterialDispose");
    auto entries = JSValue::array(0);
    auto index = JSValue(0.0);
    cache.get("forEach").call(cache, {JSValue::function(
        +[](const JSValue&, const Args& forEachArgs, const Args& captures) -> JSValue {
            auto entries = captures.at(0);
            const auto outlineMaterial = arg(forEachArgs, 0);
            const auto originalMaterial = arg(forEachArgs, 1);
            originalMaterial.get("removeEventListener")
                .call(originalMaterial, {JSValue("dispose"), captures.at(1)});
            outlineMaterial.get("dispose").call(outlineMaterial, {});
            const auto length = entries.get("length").toNumber();
            entries.set(JSValue(length), originalMaterial);
            return JSValue{};
        },
        {entries, onDispose})});
    cache.get("clear").call(cache, {});
    (void) index;
    return JSValue{};
}

// updateBefore( frame ) { ... wrap renderObjectFunction ... }
JSValue updateBeforeNode(const JSValue& rawSelf, const Args& args, const Args&) {
    JSValue& self = const_cast<JSValue&>(rawSelf);
    const auto frame = arg(args, 0);
    const auto renderer = frame.get("renderer");
    const auto currentRenderObjectFunction = renderer.get("getRenderObjectFunction").call(renderer, {});
    auto wrapper = JSValue::function(
        +[](const JSValue&, const Args& wrapperArgs, const Args& captures) -> JSValue {
            const auto selfNode = captures.at(0);
            const auto renderer = captures.at(1);
            const auto object = arg(wrapperArgs, 0);
            const auto scene = arg(wrapperArgs, 1);
            const auto camera = arg(wrapperArgs, 2);
            const auto geometry = arg(wrapperArgs, 3);
            const auto material = arg(wrapperArgs, 4);
            const auto group = arg(wrapperArgs, 5);
            const auto lightsNode = arg(wrapperArgs, 6);
            const auto clippingContext = arg(wrapperArgs, 7);
            const bool isToon = material.get("isMeshToonMaterial").toBoolean() ||
                                material.get("isMeshToonNodeMaterial").toBoolean();
            if (isToon && !material.get("wireframe").toBoolean()) {
                const auto outlineMaterial =
                    selfNode.get("_getOutlineMaterial").call(selfNode, {material});
                renderer.get("renderObject").call(
                    renderer, {object, scene, camera, geometry, outlineMaterial, group, lightsNode,
                               clippingContext});
            }
            renderer.get("renderObject").call(
                renderer,
                {object, scene, camera, geometry, material, group, lightsNode, clippingContext});
            return JSValue{};
        },
        {self, renderer});
    renderer.get("setRenderObjectFunction").call(renderer, {wrapper});
    const auto basePrototype = nativePassNodeConstructor().get("prototype");
    const auto baseUpdateBefore = basePrototype.get("updateBefore");
    if (baseUpdateBefore.toBoolean()) baseUpdateBefore.call(self, {frame});
    renderer.get("setRenderObjectFunction").call(renderer, {currentRenderObjectFunction});
    return JSValue{};
}
}  // namespace

JSValue nativeToonOutlinePassNodeModule() {
    using namespace LightingNodeRuntime;
    static thread_local JSWeakValue weak;
    if (const auto cached = weak.lock(); cached && cached->isObject()) return *cached;
    static thread_local bool initializing = false;
    JSModuleInitializationScope scope(initializing, "ToonOutlinePassNode module");
    nativePassNodeModule();
    nativeNodeMaterialModule();

    const auto base = nativePassNodeConstructor();
    JSValue bindings = JSValue::object(nullptr);
    auto constructor = makeClass(
        "ToonOutlinePassNode", 5, base,
        +[](const JSValue& target, const Args& args, const Args& captures) -> JSValue {
            auto self = superConstruct(target, captures,
                                       {JSValue("color"), arg(args, 0, nullptr), arg(args, 1, nullptr)});
            self.set("colorNode", arg(args, 2, nullptr));
            self.set("thicknessNode", arg(args, 3, nullptr));
            self.set("alphaNode", arg(args, 4, nullptr));
            self.set("_outlineMaterial", self.get("_createMaterial").call(self, {}));
            self.set("_materialCache", createNativeMap());
            auto onMaterialDispose = JSValue::function(
                +[](const JSValue&, const Args& eventArgs, const Args& captures) -> JSValue {
                    const auto selfNode = captures.at(0);
                    const auto event = arg(eventArgs, 0);
                    const auto target = event.get("target");
                    const auto cache = selfNode.get("_materialCache");
                    target.get("removeEventListener")
                        .call(target, {JSValue("dispose"), selfNode.get("_onMaterialDispose")});
                    const auto cachedMaterial = cache.get("get").call(cache, {target});
                    if (cachedMaterial.toBoolean()) {
                        cachedMaterial.get("dispose").call(cachedMaterial, {});
                        cache.get("delete").call(cache, {target});
                    }
                    return JSValue{};
                },
                {self});
            self.set("_onMaterialDispose", onMaterialDispose);
            self.set("name", JSValue("Outline Pass"));
            return self;
        },
        {}, "", "");
    {
        const auto prototype = constructor.get("prototype");
        method(prototype, "_createMaterial", 0, createOutlineMaterialMethod, {}, "");
        method(prototype, "_getOutlineMaterial", 1, getOutlineMaterialMethod, {}, "");
        method(prototype, "updateBefore", 1, updateBeforeNode, {}, "");
        method(prototype, "dispose", 0, disposeNode, {}, "");
    }
    own(bindings, "default", constructor);
    // toonOutlinePass( scene, camera, color = new Color(0,0,0), thickness = 0.003, alpha = 1 )
    own(bindings, "toonOutlinePass",
        JSValue::function(
            +[](const JSValue&, const Args& args, const Args& captures) -> JSValue {
                auto color = arg(args, 2, JSValue{});
                if (!color.toBoolean()) color = nativeColorConstructor().construct({JSValue(0), JSValue(0), JSValue(0)});
                auto thickness = arg(args, 3, JSValue{});
                if (!thickness.toBoolean()) thickness = nativeTSLBaseBinding("float").call({}, {JSValue(0.003)});
                auto alpha = arg(args, 4, JSValue{});
                if (!alpha.toBoolean()) alpha = nativeTSLBaseBinding("float").call({}, {JSValue(1.0)});
                return captures.at(0).construct({arg(args, 0, nullptr), arg(args, 1, nullptr), color,
                                                 thickness, alpha});
            },
            {constructor}));
    return publish(bindings, lexical(), weak);
}

JSValue nativeToonOutlinePassNodeBinding(const std::string& name) {
    const auto keeper = nativeToonOutlinePassNodeModule();
    (void) keeper;
    return lexical().lock()->get(name);
}

JSValue nativeToonOutlinePassNodeConstructor() {
    return nativeToonOutlinePassNodeBinding("default");
}