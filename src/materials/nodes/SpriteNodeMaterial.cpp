#include "materials/nodes/SpriteNodeMaterial.h"

#include "materials/SpriteMaterial.h"
#include "materials/nodes/NodeMaterial.h"
#include "materials/nodes/NodeMaterialSources.inc"
#include "nodes/accessors/MaterialNode.h"
#include "nodes/accessors/ModelNode.h"
#include "nodes/accessors/Position.h"
#include "nodes/accessors/ReferenceBaseNode.h"
#include "nodes/lighting/LightingNodeRuntime.h"
#include "nodes/tsl/TSLBase.h"
#include "nodes/utils/RotateNode.h"

namespace {
using namespace LightingNodeRuntime;

JSWeakValue& lexical() { static thread_local JSWeakValue weak; return weak; }
// const _defaultValues = new SpriteMaterial();  created once and reused.
JSWeakValue& defaults() { static thread_local JSWeakValue weak; return weak; }

JSValue defaultValues() {
    if (const auto value = defaults().lock(); value && value->toBoolean()) return *value;
    const auto created = nativeSpriteMaterialConstructor().construct();
    defaults() = JSWeakValue(created);
    return created;
}

JSValue vec2Node(const JSValue& a, const JSValue& b) {
    return nativeTSLBaseBinding("vec2").call({}, {a, b});
}
JSValue vec3Node(const JSValue& value) {
    return nativeTSLBaseBinding("vec3").call({}, {value});
}
JSValue vec4Node(const JSValue& xyzw) {
    return nativeTSLBaseBinding("vec4").call({}, {xyzw});
}

// setupPositionView( builder ) { ... }
JSValue setupPositionView(const JSValue& self, const Args& args, const Args&) {
    const auto builder = arg(args, 0, nullptr);
    const auto object = builder.get("object");
    const auto camera = builder.get("camera");
    const auto positionNode = self.get("positionNode");
    const auto rotationNode = self.get("rotationNode");
    const auto scaleNode = self.get("scaleNode");
    const auto sizeAttenuation = self.get("sizeAttenuation");

    const auto modelViewMatrix = nativeModelNodeBinding("modelViewMatrix");
    const auto modelWorldMatrix = nativeModelNodeBinding("modelWorldMatrix");

    // const mvPosition = modelViewMatrix.mul( vec3( positionNode || 0 ) )
    auto mvPosition = invoke(modelViewMatrix, "mul")(
        {vec3Node(positionNode.toBoolean() ? positionNode : JSValue(0.0))});

    // let scale = vec2( modelWorldMatrix[0].xyz.length(), modelWorldMatrix[1].xyz.length() )
    auto scale = vec2Node(
        invoke(modelWorldMatrix.get("0").get("xyz"), "length")(),
        invoke(modelWorldMatrix.get("1").get("xyz"), "length")());
    if (!scaleNode.isNull()) {
        // vec2( scaleNode ) converts a single value, so it goes through the binding directly.
        scale = invoke(scale, "mul")({nativeTSLBaseBinding("vec2").call({}, {scaleNode})});
    }
    const bool perspective = camera.get("isPerspectiveCamera").toBoolean();
    if (perspective && sizeAttenuation.strictEquals(false)) {
        scale = invoke(scale, "mul")({invoke(mvPosition.get("z"), "negate")()});
    }

    // let alignedPosition = positionGeometry.xy
    auto alignedPosition = nativePositionBinding("positionGeometry").get("xy");
    const auto center = object.get("center");
    if (center.toBoolean() && center.get("isVector2").strictEquals(true)) {
        const auto centerReference =
            nativeReferenceBaseNodeBinding("reference").call({}, {JSValue("center"), JSValue("vec2"), object});
        alignedPosition = invoke(alignedPosition, "sub")(
            {invoke(centerReference, "sub")({JSValue(0.5)})});
    }
    alignedPosition = invoke(alignedPosition, "mul")({scale});

    const auto rotation = nativeTSLBaseBinding("float").call(
        {}, {rotationNode.toBoolean() ? rotationNode : nativeMaterialNodeBinding("materialRotation")});
    const auto rotatedPosition = nativeRotateNodeBinding("rotate").call({}, {alignedPosition, rotation});
    // return vec4( mvPosition.xy.add( rotatedPosition ), mvPosition.zw )
    const auto xy = invoke(mvPosition.get("xy"), "add")({rotatedPosition});
    return nativeTSLBaseBinding("vec4").call({}, {xy, mvPosition.get("zw")});
}

// get sizeAttenuation() { return this._useSizeAttenuation; }
JSValue sizeAttenuationGetter(const JSValue& self, const Args&, const Args&) {
    return self.get("_useSizeAttenuation");
}

// set sizeAttenuation( value ) { if ( this._useSizeAttenuation !== value ) { ...; this.needsUpdate = true; } }
JSValue sizeAttenuationSetter(const JSValue& self, const Args& args, const Args&) {
    const auto value = arg(args, 0, nullptr);
    if (!self.get("_useSizeAttenuation").strictEquals(value)) {
        auto selfRef = self;
        selfRef.set("_useSizeAttenuation", value);
        selfRef.set("needsUpdate", true);
    }
    return {};
}
}  // namespace

JSValue nativeSpriteNodeMaterialModule() {
    using namespace LightingNodeRuntime;
    static thread_local JSWeakValue weak;
    if (const auto value = weak.lock(); value && value->isObject()) return *value;
    static thread_local bool initializing = false;
    JSModuleInitializationScope scope(initializing, "SpriteNodeMaterial module");
    nativeNodeMaterialModule();
    nativeMaterialNodeModule();
    nativeModelNodeModule();
    nativePositionModule();
    nativeReferenceBaseNodeModule();
    nativeRotateNodeModule();
    nativeTSLBase();
    const auto base = nativeNodeMaterialBinding("default");
    auto bindings = JSValue::object(nullptr);
    const auto ctor = makeClass(
        "SpriteNodeMaterial", 1, base,
        +[](const JSValue& target, const Args& args, const Args& captures) -> JSValue {
            auto self = superConstruct(target, captures, {});
            self.set("isSpriteNodeMaterial", true);
            self.set("_useSizeAttenuation", true);
            self.set("positionNode", nullptr);
            self.set("rotationNode", nullptr);
            self.set("scaleNode", nullptr);
            self.set("transparent", true);
            invoke(self, "setDefaultValues")({defaultValues()});
            invoke(self, "setValues")({arg(args, 0)});
            return self;
        },
        {}, SpriteNodeMaterialSource::classSource, SpriteNodeMaterialSource::typeSource);
    auto prototype = ctor.get("prototype");
    method(prototype, "setupPositionView", 1, setupPositionView, {},
           SpriteNodeMaterialSource::setupPositionViewSource);

    // get sizeAttenuation() / set sizeAttenuation( value )
    JSPropertyDescriptor accessor;
    accessor.configurable = true;
    accessor.getter = JSValue::function(sizeAttenuationGetter, {}, "get sizeAttenuation", 0);
    accessor.getter->setFunctionSourceText(SpriteNodeMaterialSource::sizeAttenuationSource);
    accessor.setter = JSValue::function(sizeAttenuationSetter, {}, "set sizeAttenuation", 1);
    prototype.defineProperty("sizeAttenuation", accessor);

    own(bindings, "default", ctor);
    return publish(bindings, lexical(), weak);
}

JSValue nativeSpriteNodeMaterialBinding(const std::string& name) {
    const auto keeper = nativeSpriteNodeMaterialModule();
    (void) keeper;
    return lexical().lock()->get(name);
}

JSValue nativeSpriteNodeMaterialConstructor() {
    return nativeSpriteNodeMaterialBinding("default");
}
