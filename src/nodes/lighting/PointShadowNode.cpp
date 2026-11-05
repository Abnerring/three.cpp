#include "nodes/lighting/PointShadowNode.h"

#include "constants.h"
#include "core/JSWeakMap.h"
#include "core/NativeObject.h"
#include "nodes/accessors/CubeTextureNode.h"
#include "nodes/lighting/LightingNodeRuntime.h"
#include "nodes/lighting/ShadowNode.h"
#include "textures/CubeDepthTexture.h"

#include <memory>
#include <stdexcept>
#include <string>

namespace {
using namespace LightingNodeRuntime;
JSWeakValue& lexical() { static thread_local JSWeakValue weak; return weak; }

[[noreturn]] JSValue notPorted(const char* name) {
    throw std::runtime_error(std::string("PointShadowNode: ") + name + " is not ported yet");
}

// getShadowFilterFn( type ) { return type === BasicShadowMap ? BasicPointShadowFilter : PointShadowFilter; }
// Stage A resolves the basic variant through the cube texture compare and leaves the sampled variant raising.
JSValue getShadowFilterFn(const JSValue&, const Args& args, const Args&) {
    const auto type = arg(args, 0).toNumber();
    if (type == static_cast<double>(BasicShadowMap)) {
        return JSValue::function(
            +[](const JSValue&, const Args& callbackArgs, const Args&) -> JSValue {
                const auto inputs = callbackArgs.at(0);
                const auto depthTexture = inputs.get("depthTexture");
                const auto bd3D = inputs.get("bd3D");
                const auto dp = inputs.get("dp");
                const auto sampled =
                    nativeCubeTextureNodeBinding("cubeTexture").call({}, {depthTexture, bd3D});
                return invoke(sampled, "compare")({dp});
            });
    }
    return notPorted("getShadowFilterFn (PointShadowFilter)");
}

// setupShadowCoord( builder, shadowPosition ) { return shadowPosition; }
JSValue setupShadowCoord(const JSValue&, const Args& args, const Args&) { return arg(args, 1); }

// setupRenderTarget( shadow, builder ) { new CubeDepthTexture( width ) + createCubeRenderTarget( width ) }
JSValue setupRenderTarget(const JSValue&, const Args& args, const Args&) {
    const auto shadow = arg(args, 0);
    const auto builder = arg(args, 1);
    const auto mapSize = shadow.get("mapSize");
    const auto renderer = builder.get("renderer");

    auto depthTexture = nativeTexture(
        std::make_shared<CubeDepthTexture>(mapSize.get("width").toNumber()));
    depthTexture.set("name", JSValue("PointShadowDepthTexture"));
    const auto compareFunction = renderer.get("reversedDepthBuffer").toBoolean()
                                     ? JSValue(static_cast<double>(GreaterEqualCompare))
                                     : JSValue(static_cast<double>(LessEqualCompare));
    depthTexture.set("compareFunction", compareFunction);

    auto shadowMap = builder.get("createCubeRenderTarget").call(builder, {mapSize.get("width")});
    auto texture = shadowMap.get("texture");
    texture.set("name", JSValue("PointShadowMap"));
    shadowMap.set("depthTexture", depthTexture);

    auto result = JSValue::object(nullptr);
    result.set("shadowMap", shadowMap);
    result.set("depthTexture", depthTexture);
    return result;
}

JSValue setupShadowFilter(const JSValue&, const Args&, const Args&) {
    return notPorted("setupShadowFilter");
}
JSValue renderShadow(const JSValue&, const Args&, const Args&) { return notPorted("renderShadow"); }
}  // namespace

JSValue nativePointShadowNodeModule() {
    using namespace LightingNodeRuntime;
    static thread_local JSWeakValue weak;
    if (const auto cached = weak.lock(); cached && cached->isObject()) return *cached;
    static thread_local bool initializing = false;
    JSModuleInitializationScope scope(initializing, "PointShadowNode module");
    nativeShadowNodeModule();

    const auto base = nativeShadowNodeConstructor();
    JSValue bindings = JSValue::object(nullptr);
    const auto ctor = makeClass(
        "PointShadowNode", 2, base,
        +[](const JSValue& target, const Args& args, const Args& captures) -> JSValue {
            const auto light = arg(args, 0, nullptr);
            auto shadow = arg(args, 1, JSValue{});
            auto self = superConstruct(target, captures, {light, shadow});
            self.set("isPointShadowNode", true);
            return self;
        },
        {}, "", "");

    const auto prototype = ctor.get("prototype");
    method(prototype, "getShadowFilterFn", 1, getShadowFilterFn, {}, "");
    method(prototype, "setupShadowCoord", 2, setupShadowCoord, {}, "");
    method(prototype, "setupRenderTarget", 2, setupRenderTarget, {}, "");
    method(prototype, "setupShadowFilter", 2, setupShadowFilter, {}, "");
    method(prototype, "renderShadow", 1, renderShadow, {}, "");
    own(bindings, "default", ctor);
    return publish(bindings, lexical(), weak);
}

JSValue nativePointShadowNodeBinding(const std::string& name) {
    const auto keeper = nativePointShadowNodeModule();
    (void) keeper;
    return lexical().lock()->get(name);
}

JSValue nativePointShadowNodeConstructor() { return nativePointShadowNodeBinding("default"); }