#include "materials/nodes/MeshSSSNodeMaterial.h"

#include "materials/nodes/MeshPhysicalNodeMaterial.h"
#include "materials/nodes/MeshSSSNodeMaterialSources.inc"
#include "nodes/accessors/Normal.h"
#include "nodes/accessors/Position.h"
#include "nodes/functions/PhysicalLightingModel.h"
#include "nodes/lighting/LightingNodeRuntime.h"
#include "nodes/tsl/TSLBase.h"

namespace {
using namespace LightingNodeRuntime;
JSWeakValue& lexical() { static thread_local JSWeakValue weak; return weak; }

JSValue direct(const JSValue& self, const Args& args, const Args& captures) {
    const auto input = arg(args, 0);
    const auto lightDirection = input.get("lightDirection");
    const auto lightColor = input.get("lightColor");
    const auto reflectedLight = input.get("reflectedLight");
    const auto builder = arg(args, 1);
    if (self.get("useSSS").strictEquals(true)) {
        const auto material = builder.get("material");
        const auto color = material.get("thicknessColorNode");
        const auto distortion = material.get("thicknessDistortionNode");
        const auto ambient = material.get("thicknessAmbientNode");
        const auto attenuation = material.get("thicknessAttenuationNode");
        const auto power = material.get("thicknessPowerNode");
        const auto scale = material.get("thicknessScaleNode");

        const auto addDirection = invoke(lightDirection, "add");
        const auto distortedNormal = invoke(nativeNormalBinding("normalView"), "mul")({distortion});
        const auto scatteringHalf = invoke(addDirection({distortedNormal}), "normalize")();
        const auto dot = invoke(nativePositionBinding("positionViewDirection"), "dot")(
            {invoke(scatteringHalf, "negate")()});
        const auto saturated = invoke(dot, "saturate")();
        const auto powered = invoke(saturated, "pow")({power});
        const auto scatteringDot = nativeTSLBaseBinding("float").call({}, {invoke(powered, "mul")({scale})});
        const auto scatteringIllu = nativeTSLBaseBinding("vec3").call({},
            {invoke(invoke(scatteringDot, "add")({ambient}), "mul")({color})});
        const auto addDiffuse = invoke(reflectedLight.get("directDiffuse"), "addAssign");
        const auto attenuatedColor = invoke(attenuation, "mul")({lightColor});
        addDiffuse({invoke(scatteringIllu, "mul")({attenuatedColor})});
    }

    // Resolve super from the method's home object, preserving changes to its
    // prototype and the original receiver. The forwarded input has only these
    // three properties, as the upstream destructuring/reconstruction specifies.
    const auto baseDirect = captures[0].getPrototype().get("direct", self);
    auto forwarded = JSValue::object();
    own(forwarded, "lightDirection", lightDirection);
    own(forwarded, "lightColor", lightColor);
    own(forwarded, "reflectedLight", reflectedLight);
    baseDirect.call(self, {forwarded, builder});
    return {};
}

JSValue lightingModelConstructor() {
    const auto base = nativePhysicalLightingModelConstructor();
    auto holder = JSValue::object(nullptr);
    auto ctor = JSValue::builtinConstructor(
        +[](const JSValue& target, const Args& args, const Args& captures) -> JSValue {
            Args flags;
            for (unsigned i = 0; i < 6; ++i) flags.push_back(arg(args, i, false));
            auto self = superConstruct(target, captures, flags);
            self.set("useSSS", arg(args, 6, false));
            return self;
        }, {holder}, "SSSLightingModel", 0);
    holder.set("constructor", ctor);
    ctor.setPrototype(base);
    ctor.setFunctionSourceText(MeshSSSNodeMaterialSource::lightingClassSource);
    auto prototype = ctor.get("prototype");
    prototype.setPrototype(base.get("prototype"));
    method(prototype, "direct", 2, direct, {prototype}, MeshSSSNodeMaterialSource::directSource);
    return ctor;
}
} // namespace

JSValue nativeMeshSSSNodeMaterialModule() {
    using namespace LightingNodeRuntime;
    static thread_local JSWeakValue weak;
    if (const auto cached = weak.lock(); cached && cached->isObject()) return *cached;
    static thread_local bool initializing = false;
    JSModuleInitializationScope scope(initializing, "MeshSSSNodeMaterial module");
    const auto base = nativeMeshPhysicalNodeMaterialConstructor();
    nativeNormalModule();
    nativePositionModule();
    nativeTSLBase();
    const auto model = lightingModelConstructor();
    const auto ctor = makeClass("MeshSSSNodeMaterial", 1, base,
        +[](const JSValue& target, const Args& args, const Args& captures) -> JSValue {
            auto self = superConstruct(target, captures, {arg(args, 0)});
            self.set("thicknessColorNode", nullptr);
            self.set("thicknessDistortionNode", nativeTSLBaseBinding("float").call({}, {0.1}));
            self.set("thicknessAmbientNode", nativeTSLBaseBinding("float").call({}, {0.0}));
            self.set("thicknessAttenuationNode", nativeTSLBaseBinding("float").call({}, {0.1}));
            self.set("thicknessPowerNode", nativeTSLBaseBinding("float").call({}, {2.0}));
            self.set("thicknessScaleNode", nativeTSLBaseBinding("float").call({}, {10.0}));
            return self;
        }, {}, MeshSSSNodeMaterialSource::classSource, MeshSSSNodeMaterialSource::typeSource);
    auto prototype = ctor.get("prototype");
    JSPropertyDescriptor useSSS;
    useSSS.configurable = true;
    useSSS.getter = JSValue::function(
        +[](const JSValue& self, const Args&, const Args&) -> JSValue {
            return !self.get("thicknessColorNode").isNull();
        }, {}, "get useSSS", 0);
    useSSS.getter->setFunctionSourceText(MeshSSSNodeMaterialSource::useSSSSource);
    prototype.defineProperty("useSSS", useSSS);
    method(prototype, "setupLightingModel", 0,
        +[](const JSValue& self, const Args&, const Args& captures) -> JSValue {
            Args flags;
            for (const auto* key : {"useClearcoat", "useSheen", "useIridescence", "useAnisotropy",
                                    "useTransmission", "useDispersion", "useSSS"}) flags.push_back(self.get(key));
            return captures[0].construct(flags);
        }, {model}, MeshSSSNodeMaterialSource::setupLightingModelSource);
    auto bindings = JSValue::object(nullptr);
    own(bindings, "default", ctor);
    return publish(bindings, lexical(), weak);
}

JSValue nativeMeshSSSNodeMaterialBinding(const std::string& name) {
    const auto keeper = nativeMeshSSSNodeMaterialModule();
    (void)keeper;
    return lexical().lock()->get(name);
}

JSValue nativeMeshSSSNodeMaterialConstructor() { return nativeMeshSSSNodeMaterialBinding("default"); }
