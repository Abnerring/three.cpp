#include "nodes/utils/PostProcessingUtils.h"

#include "constants.h"
#include "nodes/accessors/TextureNode.h"
#include "nodes/accessors/TextureSizeNode.h"
#include "nodes/math/MathNode.h"
#include "nodes/tsl/TSLBase.h"
#include "nodes/tsl/TSLCore.h"
#include "nodes/utils/UtilityNodeRuntime.h"

namespace {
using namespace UtilityNodeRuntime;

JSWeakValue& lexical() { static thread_local JSWeakValue weak; return weak; }
JSWeakValue& viewPositionCache() { static thread_local JSWeakValue weak; return weak; }

JSValue num(double value) { return nativeTSLBaseBinding("float").call({}, {JSValue(value)}); }
JSValue v2(const JSValue& a, const JSValue& b) {
    return nativeTSLBaseBinding("vec2").call({}, {a, b});
}
JSValue v3(const JSValue& a, const JSValue& b, const JSValue& c) {
    return nativeTSLBaseBinding("vec3").call({}, {a, b, c});
}
JSValue v4(const JSValue& value) { return nativeTSLBaseBinding("vec4").call({}, {value}); }
JSValue i2(const JSValue& a, const JSValue& b) {
    return nativeTSLBaseBinding("ivec2").call({}, {a, b});
}
JSValue loadDepth(const JSValue& depthTexture, const JSValue& coord) {
    return coord.isUndefined() ? nativeTextureNodeBinding("textureLoad").call({}, {depthTexture})
                               : nativeTextureNodeBinding("textureLoad").call({}, {depthTexture, coord});
}

// The Fn calling convention in this runtime is ( paramsIterable, builder ): the parameters arrive as
// one iterable and the builder as the second argument, as Normal.cpp and ViewportDepthNode.cpp use.
JSValue makeFn(JSValue::NativeFunction body) {
    auto callback = JSValue::function(body, {}, "", 0);
    return nativeTSLCoreBinding("Fn").call({}, {callback});
}

// Reads the first n parameters of the iterable and closes it.
std::vector<JSValue> takeParams(const JSValue& iterable, int count) {
    std::vector<JSValue> out;
    auto iterator = getJSIterator(iterable);
    for (int i = 0; i < count; ++i) {
        if (iterator.done) break;
        const auto value = stepJSIterator(iterator);
        out.push_back(value ? *value : JSValue{});
    }
    if (!iterator.done) closeJSIterator(iterator);
    return out;
}

// isBackgroundDepth = Fn( ( [ depth ], builder ) => builder.renderer.reversedDepthBuffer === true ? depth.lessThanEqual( 0.0 ) : depth.greaterThanEqual( 1.0 ) )
JSValue isBackgroundDepthBody(const JSValue&, const Args& args, const Args&) {
    const auto params = takeParams(arg(args, 0), 1);
    const auto depth = params.empty() ? JSValue{} : params[0];
    // The builder is only present when the function is called from a build. Without one the flag it
    // would read is absent, which is the same as the flag not being exactly true, so the ordinary
    // branch is taken; the reversed branch is only reachable when a builder is threaded in.
    const auto builder = arg(args, 1);
    const auto renderer = builder.isObject() ? builder.get("renderer") : JSValue{};
    const bool reversed =
        renderer.isObject() && renderer.get("reversedDepthBuffer").strictEquals(true);
    return reversed ? invoke(depth, "lessThanEqual")({num(0.0)})
                    : invoke(depth, "greaterThanEqual")({num(1.0)});
}

// getViewPosition = Fn( ( [ screenPosition, depth, projectionMatrixInverse ], builder ) => { ... } )
JSValue getViewPositionBody(const JSValue&, const Args& args, const Args&) {
    const auto params = takeParams(arg(args, 0), 3);
    const auto screenPosition = params.size() > 0 ? params[0] : JSValue{};
    const auto depth = params.size() > 1 ? params[1] : JSValue{};
    const auto projectionMatrixInverse = params.size() > 2 ? params[2] : JSValue{};

    const auto builderArg = arg(args, 1);
    const auto renderer = builderArg.isObject() ? builderArg.get("renderer") : JSValue{};
    const bool webgpu = renderer.isObject() &&
        renderer.get("coordinateSystem").toNumber() == static_cast<double>(WebGPUCoordinateSystem);
    const bool reversed =
        renderer.isObject() && renderer.get("reversedDepthBuffer").strictEquals(true);

    const auto flippedY = invoke(screenPosition.get("y"), "oneMinus")();
    JSValue clipSpacePosition;
    if (webgpu || reversed) {
        // The two screen coordinates go into clip space and the depth passes through untouched.
        const auto scaled = invoke(invoke(v2(screenPosition.get("x"), flippedY), "mul")({num(2.0)}),
                                   "sub")({num(1.0)});
        clipSpacePosition = v4(v3(scaled.get("x"), scaled.get("y"), depth));
    } else {
        const auto scaled = invoke(invoke(v3(screenPosition.get("x"), flippedY, depth), "mul")({num(2.0)}),
                                   "sub")({num(1.0)});
        clipSpacePosition = v4(scaled);
    }

    const auto viewSpacePosition =
        v4(invoke(projectionMatrixInverse, "mul")({clipSpacePosition}));
    return invoke(viewSpacePosition.get("xyz"), "div")({viewSpacePosition.get("w")});
}

// getScreenPosition = Fn( ( [ viewPosition, projectionMatrix ] ) => { ... }, { viewPosition: 'vec3', projectionMatrix: 'mat4', return: 'vec2' } )
JSValue getScreenPositionBody(const JSValue&, const Args& args, const Args&) {
    const auto params = takeParams(arg(args, 0), 2);
    const auto viewPosition = params.size() > 0 ? params[0] : JSValue{};
    const auto projectionMatrix = params.size() > 1 ? params[1] : JSValue{};

    const auto clip = invoke(projectionMatrix, "mul")({v4(viewPosition)});
    const auto uv = invoke(
        invoke(invoke(clip.get("xy"), "div")({clip.get("w")}), "mul")({num(0.5)}), "add")({num(0.5)});
    return v2(uv.get("x"), invoke(uv.get("y"), "oneMinus")());
}

// getScreenPositionFromClip = Fn( ( [ clipPosition ] ) => { ... }, { clipPosition: 'vec4', return: 'vec2' } )
JSValue getScreenPositionFromClipBody(const JSValue&, const Args& args, const Args&) {
    const auto params = takeParams(arg(args, 0), 1);
    const auto clipPosition = params.empty() ? JSValue{} : params[0];

    const auto screen = invoke(
        invoke(invoke(clipPosition.get("xy"), "div")({clipPosition.get("w")}), "mul")({num(0.5)}),
        "add")({num(0.5)});
    return v2(screen.get("x"), invoke(screen.get("y"), "oneMinus")());
}

// getNormalFromDepth = Fn( ( [ uv, depthTexture, projectionMatrixInverse ] ) => { ... } )
//
// The body calls the module's own getViewPosition, exactly as the source does. That export is looked
// up lazily because it is registered while this module is still being built.
JSValue getNormalFromDepthBody(const JSValue&, const Args& args, const Args&) {
    const auto params = takeParams(arg(args, 0), 3);
    const auto uv = params.size() > 0 ? params[0] : JSValue{};
    const auto depthTexture = params.size() > 1 ? params[1] : JSValue{};
    const auto projectionMatrixInverse = params.size() > 2 ? params[2] : JSValue{};

    JSValue viewPosition;
    if (const auto cached = viewPositionCache().lock(); cached && cached->isCallable()) {
        viewPosition = *cached;
    }
    if (!viewPosition.isCallable()) {
        throw std::runtime_error(
            "PostProcessingUtils::getNormalFromDepth: the getViewPosition export is not initialised");
    }
    const auto viewAt = [&](const JSValue& uvValue, const JSValue& depthValue) -> JSValue {
        return viewPosition.call({}, {uvValue, depthValue, projectionMatrixInverse});
    };

    const auto size = nativeTextureSizeNodeBinding("textureSize").call({}, {loadDepth(depthTexture, {})});
    // ivec2( uv.mul( size ) ): a single vector argument, not the two scalar form.
    const auto p = nativeTSLBaseBinding("ivec2").call({}, {invoke(uv, "mul")({size})});

    const auto c0 = loadDepth(depthTexture, p);
    const auto l2 = loadDepth(depthTexture, invoke(p, "sub")({i2(num(2), num(0))}));
    const auto l1 = loadDepth(depthTexture, invoke(p, "sub")({i2(num(1), num(0))}));
    const auto r1 = loadDepth(depthTexture, invoke(p, "add")({i2(num(1), num(0))}));
    const auto r2 = loadDepth(depthTexture, invoke(p, "add")({i2(num(2), num(0))}));
    const auto b2 = loadDepth(depthTexture, invoke(p, "add")({i2(num(0), num(2))}));
    const auto b1 = loadDepth(depthTexture, invoke(p, "add")({i2(num(0), num(1))}));
    const auto t1 = loadDepth(depthTexture, invoke(p, "sub")({i2(num(0), num(1))}));
    const auto t2 = loadDepth(depthTexture, invoke(p, "sub")({i2(num(0), num(2))}));

    // dl = abs( sub( float( 2 ).mul( l1 ).sub( l2 ), c0 ) ), and the same shape for r, b and t.
    const auto laplacian = [](const JSValue& inner, const JSValue& outer) -> JSValue {
        return invoke(invoke(invoke(num(2.0), "mul")({inner}), "sub")({outer}), "abs")();
    };
    const auto dl = laplacian(l1, l2);
    const auto dr = laplacian(r1, r2);
    const auto db = laplacian(b1, b2);
    const auto dt = laplacian(t1, t2);

    const auto centre = viewAt(uv, c0);
    const auto stepX = invoke(num(1.0), "div")({size.get("x")});
    const auto stepY = invoke(num(1.0), "div")({size.get("y")});

    const auto leftCandidate = invoke(centre, "sub")({viewAt(invoke(uv, "sub")({v2(stepX, num(0.0))}), l1)});
    const auto rightCandidate = invoke(invoke(centre, "negate")(),
                                       "add")({viewAt(invoke(uv, "add")({v2(stepX, num(0.0))}), r1)});
    const auto dpdx = invoke(invoke(dl, "lessThan")({dr}), "select")({leftCandidate, rightCandidate});

    const auto bottomCandidate =
        invoke(centre, "sub")({viewAt(invoke(uv, "add")({v2(num(0.0), stepY)}), b1)});
    const auto topCandidate = invoke(invoke(centre, "negate")(),
                                     "add")({viewAt(invoke(uv, "sub")({v2(num(0.0), stepY)}), t1)});
    const auto dpdy = invoke(invoke(db, "lessThan")({dt}), "select")({bottomCandidate, topCandidate});

    return nativeMathNodeBinding("normalize")
        .call({}, {nativeMathNodeBinding("cross").call({}, {dpdx, dpdy})});
}

// interleavedGradientNoise = Fn( ( [ position ] ) => fract( float( 52.9829189 ).mul( fract( dot( position, vec2( 0.06711056, 0.00583715 ) ) ) ) ), { position: 'vec2', return: 'float' } )
JSValue interleavedGradientNoiseBody(const JSValue&, const Args& args, const Args&) {
    const auto params = takeParams(arg(args, 0), 1);
    const auto position = params.empty() ? JSValue{} : params[0];
    const auto constants = v2(num(0.06711056), num(0.00583715));
    const auto dotted = nativeMathNodeBinding("dot").call({}, {position, constants});
    const auto inner = invoke(dotted, "fract")();
    const auto scaled = invoke(num(52.9829189), "mul")({inner});
    return invoke(scaled, "fract")();
}

// vogelDiskSample = Fn( ( [ sampleIndex, samplesCount, phi ] ) => { ... }, { sampleIndex: 'int', samplesCount: 'int', phi: 'float', return: 'vec2' } )
JSValue vogelDiskSampleBody(const JSValue&, const Args& args, const Args&) {
    const auto params = takeParams(arg(args, 0), 3);
    const auto sampleIndex = params.size() > 0 ? params[0] : JSValue{};
    const auto samplesCount = params.size() > 1 ? params[1] : JSValue{};
    const auto phi = params.size() > 2 ? params[2] : JSValue{};

    const auto goldenAngle = num(2.399963229728653);
    const auto indexAsFloat = nativeTSLBaseBinding("float").call({}, {sampleIndex});
    const auto shifted = invoke(indexAsFloat, "add")({num(0.5)});
    const auto ratio =
        invoke(shifted, "div")({nativeTSLBaseBinding("float").call({}, {samplesCount})});
    const auto r = nativeMathNodeBinding("sqrt").call({}, {ratio});
    const auto theta = invoke(invoke(indexAsFloat, "mul")({goldenAngle}), "add")({phi});
    const auto circle = v2(nativeMathNodeBinding("cos").call({}, {theta}),
                           nativeMathNodeBinding("sin").call({}, {theta}));
    return invoke(circle, "mul")({r});
}
}  // namespace

JSValue nativePostProcessingUtilsModule() {
    using namespace UtilityNodeRuntime;
    static thread_local JSWeakValue weak;
    if (const auto cached = weak.lock(); cached && cached->isObject()) return *cached;
    static thread_local bool initializing = false;
    JSModuleInitializationScope scope(initializing, "PostProcessingUtils module");
    nativeTSLCore();
    nativeTSLBase();
    nativeMathNodeModule();
    nativeTextureNodeModule();
    nativeTextureSizeNodeModule();

    auto bindings = JSValue::object(nullptr);
    own(bindings, "isBackgroundDepth", makeFn(isBackgroundDepthBody));
    const auto viewPosition = makeFn(getViewPositionBody);
    viewPositionCache() = JSWeakValue(viewPosition);
    own(bindings, "getViewPosition", viewPosition);
    own(bindings, "getScreenPosition", makeFn(getScreenPositionBody));
    own(bindings, "getScreenPositionFromClip", makeFn(getScreenPositionFromClipBody));
    own(bindings, "getNormalFromDepth", makeFn(getNormalFromDepthBody));
    own(bindings, "interleavedGradientNoise", makeFn(interleavedGradientNoiseBody));
    own(bindings, "vogelDiskSample", makeFn(vogelDiskSampleBody));

    auto module = JSValue::object(nullptr);
    for (const auto& key : bindings.ownPropertyNames()) own(module, key.c_str(), bindings.get(key));
    bindings.retainForHeapLifetime();
    lexical() = JSWeakValue(bindings);
    module.retainForHeapLifetime();
    weak = JSWeakValue(module);
    return module;
}

JSValue nativePostProcessingUtilsBinding(const std::string& name) {
    const auto keeper = nativePostProcessingUtilsModule();
    (void) keeper;
    return lexical().lock()->get(name);
}
