#include "nodes/lighting/IESSpotLightNode.h"

#include "nodes/accessors/TextureNode.h"
#include "nodes/accessors/TextureSizeNode.h"
#include "nodes/lighting/LightingEventNodeSources.inc"
#include "nodes/lighting/LightingNodeRuntime.h"
#include "nodes/lighting/SpotLightNode.h"
#include "nodes/math/MathNode.h"
#include "nodes/tsl/TSLBase.h"
#include "nodes/utils/Remap.h"

namespace {
using namespace LightingNodeRuntime;

JSWeakValue& lexical() {
    static thread_local JSWeakValue weak;
    return weak;
}

constexpr double kPi = 3.141592653589793;

// getSpotAttenuation( builder, angleCosine ) { ... }
JSValue getSpotAttenuation(const JSValue& self, const Args& args, const Args&) {
    const auto builder = arg(args, 0, nullptr);
    const auto angleCosine = arg(args, 1, nullptr);
    const auto iesMap = self.get("light").get("iesMap");

    // if ( iesMap && iesMap.isTexture === true )
    if (iesMap.toBoolean() && iesMap.get("isTexture").strictEquals(true)) {
        // const lightCoord = this.getLightCoord( builder ).sub( 0.5 );
        auto lightCoord = invoke(invoke(self, "getLightCoord")({builder}), "sub")({JSValue(0.5)});

        // this._iesTextureNode = texture( iesMap );
        const auto textureNode = nativeTextureNodeBinding("texture").call({}, {iesMap});
        // JSValue is a handle, so the copy still names the receiver's own property.
        auto selfRef = self;
        selfRef.set("_iesTextureNode", textureNode);

        // const texelInset = float( 0.5 ).div( vec2( textureSize( this._iesTextureNode ) ) );
        const auto half = nativeTSLBaseBinding("float").call({}, {JSValue(0.5)});
        const auto sizeNode =
            nativeTextureSizeNodeBinding("textureSize").call({}, {textureNode});
        const auto sizeVector = nativeTSLBaseBinding("vec2").call({}, {sizeNode});
        const auto texelInset = invoke(half, "div")({sizeVector});

        // const twistAngle = remap( atan( lightCoord.y, lightCoord.x ), - Math.PI, Math.PI ).add( texelInset.y );
        // The source reads the swizzles as properties; this object model reads them the same way.
        const auto atanNode = nativeMathNodeBinding("atan").call(
            {}, {lightCoord.get("y"), lightCoord.get("x")});
        const auto remappedTwist =
            nativeRemapBinding("remap").call({}, {atanNode, JSValue(-kPi), JSValue(kPi)});
        const auto twistAngle = invoke(remappedTwist, "add")({texelInset.get("y")});

        // const tiltAngle = remap( angleCosine.acos(), 0, Math.PI, texelInset.x, float( 1 ).sub( texelInset.x ) );
        const auto acosNode = nativeMathNodeBinding("acos").call({}, {angleCosine});
        const auto one = nativeTSLBaseBinding("float").call({}, {JSValue(1.0)});
        const auto upperBound = invoke(one, "sub")({texelInset.get("x")});
        const auto tiltAngle = nativeRemapBinding("remap").call(
            {}, {acosNode, JSValue(0), JSValue(kPi), texelInset.get("x"), upperBound});

        // this._iesTextureNode.sample( vec2( tiltAngle, twistAngle ) ).level( 0 ).r
        const auto uv = nativeTSLBaseBinding("vec2").call({}, {tiltAngle, twistAngle});
        const auto sampled = invoke(textureNode, "sample")({uv});
        const auto leveled = invoke(sampled, "level")({JSValue(0)});
        return leveled.get("r");
    }

    // super.getSpotAttenuation( builder, angleCosine ): the family does not dispatch to super, so
    // the parent implementation is fetched from its own prototype and called with this receiver.
    const auto parentPrototype = nativeSpotLightNodeBinding("default").get("prototype");
    return parentPrototype.get("getSpotAttenuation").call(self, {builder, angleCosine});
}

// update( frame ) { super.update( frame ); if ( this._iesTextureNode !== null && this.light.iesMap ) { this._iesTextureNode.value = this.light.iesMap; } }
JSValue update(const JSValue& self, const Args&, const Args&) {
    const auto light = self.get("light");
    if (light.isNull() || light.isUndefined()) return {};

    // super.update( frame ), inlined as this family does.
    auto color = self.get("color");
    invoke(color, "copy")({light.get("color")});
    const auto intensity = light.get("intensity").toNumber();
    color.set("r", color.get("r").toNumber() * intensity);
    color.set("g", color.get("g").toNumber() * intensity);
    color.set("b", color.get("b").toNumber() * intensity);

    // The cone uniforms are the parent's job; this override only refreshes the IES texture.
    auto iesTexture = self.get("_iesTextureNode");
    if (!iesTexture.isNull() && light.get("iesMap").toBoolean()) {
        iesTexture.set("value", light.get("iesMap"));
    }
    return {};
}
}  // namespace

JSValue nativeIESSpotLightNodeModule() {
    using namespace LightingNodeRuntime;
    static thread_local JSWeakValue weak;
    if (const auto cached = weak.lock(); cached && cached->isObject()) return *cached;

    static thread_local bool initializing = false;
    JSModuleInitializationScope scope(initializing, "IESSpotLightNode module");
    nativeTSLBase();
    nativeTextureNodeModule();
    nativeTextureSizeNodeModule();
    nativeMathNodeModule();
    nativeRemapModule();
    const auto base = nativeSpotLightNodeBinding("default");
    auto bindings = JSValue::object(nullptr);
    const auto ctor = makeClass(
        "IESSpotLightNode", 0, base,
        +[](const JSValue& target, const Args& args, const Args& captures) -> JSValue {
            const auto light = arg(args, 0, nullptr);
            auto self = superConstruct(target, captures, {light});
            self.set("_iesTextureNode", nullptr);
            return self;
        },
        {}, IESSpotLightNodeSource::classSource, IESSpotLightNodeSource::typeSource);
    method(ctor.get("prototype"), "getSpotAttenuation", 2, getSpotAttenuation, {},
           IESSpotLightNodeSource::getSpotAttenuationSource);
    method(ctor.get("prototype"), "update", 1, update, {}, IESSpotLightNodeSource::updateSource);
    own(bindings, "default", ctor);
    return publish(bindings, lexical(), weak);
}

JSValue nativeIESSpotLightNodeBinding(const std::string& name) {
    const auto keeper = nativeIESSpotLightNodeModule();
    (void) keeper;
    return lexical().lock()->get(name);
}

JSValue nativeIESSpotLightNodeConstructor() {
    return nativeIESSpotLightNodeBinding("default");
}

IESSpotLightNode::IESSpotLightNode(const JSValue& light)
    : object_(constructor().construct({light})) {}
