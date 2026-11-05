#include "nodes/lighting/ShadowFilterNode.h"

#include "nodes/accessors/ReferenceNode.h"
#include "nodes/accessors/TextureNode.h"
#include "nodes/core/UniformGroupNode.h"
#include "nodes/display/ScreenNode.h"
#include "nodes/lighting/LightingNodeRuntime.h"
#include "nodes/math/MathNode.h"
#include "nodes/math/OperatorNode.h"
#include "nodes/tsl/TSLBase.h"
#include "nodes/tsl/TSLCore.h"
#include "nodes/utils/PostProcessingUtils.h"

namespace {
using namespace LightingNodeRuntime;

JSWeakValue& lexical() { static thread_local JSWeakValue weak; return weak; }

// The Fn convention: the inputs arrive as one iterable whose single entry is the destructured object.
JSValue inputObject(const Args& args) {
    auto iterator = getJSIterator(arg(args, 0));
    JSValue inputs;
    if (!iterator.done) {
        const auto value = stepJSIterator(iterator);
        if (value) inputs = *value;
    }
    if (!iterator.done) closeJSIterator(iterator);
    return inputs;
}

JSValue floatNode(double value) { return nativeTSLBaseBinding("float").call({}, {JSValue(value)}); }

JSValue makeFn(JSValue::NativeFunction body) {
    auto callback = JSValue::function(body, {}, "", 0);
    return nativeTSLCoreBinding("Fn").call({}, {callback});
}

// texture( depthTexture, uv ), with the array texture depth layer applied when the texture is an array.
JSValue sampleDepth(const JSValue& depthTexture, const JSValue& uv, const JSValue& depthLayer) {
    auto sampled = nativeTextureNodeBinding("texture").call({}, {depthTexture, uv});
    if (depthTexture.get("isArrayTexture").toBoolean()) {
        sampled = invoke(sampled, "depth")({depthLayer});
    }
    return sampled;
}

// BasicShadowFilter = Fn( ( { depthTexture, shadowCoord, depthLayer } ) => { let basic = texture(...).setName('t_basic'); ...; return basic.compare( shadowCoord.z ); } )
JSValue basicShadowFilterBody(const JSValue&, const Args& args, const Args&) {
    const auto inputs = inputObject(args);
    const auto depthTexture = inputs.get("depthTexture");
    const auto shadowCoord = inputs.get("shadowCoord");

    auto basic = sampleDepth(depthTexture, shadowCoord.get("xy"), inputs.get("depthLayer"));
    basic = invoke(basic, "setName")({JSValue("t_basic")});
    return invoke(basic, "compare")({shadowCoord.get("z")});
}

// PCFShadowFilter = Fn( ( { depthTexture, shadowCoord, shadow, depthLayer } ) => { ... } )
JSValue pcfShadowFilterBody(const JSValue&, const Args& args, const Args&) {
    const auto inputs = inputObject(args);
    const auto depthTexture = inputs.get("depthTexture");
    const auto shadowCoord = inputs.get("shadowCoord");
    const auto shadow = inputs.get("shadow");
    const auto depthLayer = inputs.get("depthLayer");

    const auto depthCompare = [&](const JSValue& uv, const JSValue& compare) -> JSValue {
        const auto depth = sampleDepth(depthTexture, uv, depthLayer);
        return invoke(depth, "compare")({compare});
    };

    // The reference nodes are grouped into the render group, as the source does.
    const auto mapSize = invoke(
        nativeReferenceNodeBinding("reference").call(
            {}, {JSValue("mapSize"), JSValue("vec2"), shadow}),
        "setGroup")({nativeRenderGroup()});
    const auto radius = invoke(
        nativeReferenceNodeBinding("reference").call(
            {}, {JSValue("radius"), JSValue("float"), shadow}),
        "setGroup")({nativeRenderGroup()});

    const auto texelSize = invoke(nativeTSLBaseBinding("vec2").call({}, {JSValue(1)}), "div")({mapSize});
    const auto radiusScaled = invoke(radius, "mul")({texelSize.get("x")});

    // phi = interleavedGradientNoise( screenCoordinate.xy ) * 2 pi
    const auto phi = invoke(
        nativePostProcessingUtilsBinding("interleavedGradientNoise")
            .call({}, {nativeScreenNodeBinding("screenCoordinate").get("xy")}),
        "mul")({JSValue(6.28318530718)});

    // Five Vogel disk samples. The source passes all five to one variadic add; the sum is built as a
    // chain here, which carries the same value with the left to right grouping.
    const auto add = nativeOperatorNodeBinding("add");
    JSValue sum;
    for (int sample = 0; sample < 5; ++sample) {
        const auto offset = invoke(
            nativePostProcessingUtilsBinding("vogelDiskSample")
                .call({}, {JSValue(static_cast<double>(sample)), JSValue(5.0), phi}),
            "mul")({radiusScaled});
        const auto uv = invoke(shadowCoord.get("xy"), "add")({offset});
        const auto term = depthCompare(uv, shadowCoord.get("z"));
        sum = sample == 0 ? term : add.call({}, {sum, term});
    }
    return invoke(sum, "mul")({floatNode(1.0 / 5.0)});
}

// VSMShadowFilter = Fn( ( { depthTexture, shadowCoord, depthLayer }, builder ) => { ... } )
JSValue vsmShadowFilterBody(const JSValue&, const Args& args, const Args&) {
    const auto inputs = inputObject(args);
    const auto depthTexture = inputs.get("depthTexture");
    const auto shadowCoord = inputs.get("shadowCoord");
    const auto depthLayer = inputs.get("depthLayer");

    // let distribution = texture( depthTexture ).sample( shadowCoord.xy );
    auto distribution = invoke(nativeTextureNodeBinding("texture").call({}, {depthTexture}), "sample")(
        {shadowCoord.get("xy")});
    if (depthTexture.get("isArrayTexture").toBoolean()) {
        distribution = invoke(distribution, "depth")({depthLayer});
    }
    distribution = distribution.get("rg");

    const auto mean = distribution.get("x");
    const auto variance = nativeMathNodeBinding("max").call(
        {}, {JSValue(0.0000001),
             invoke(distribution.get("y"), "mul")({distribution.get("y")})});

    // The builder decides which way round the step goes. Without one the flag it would read is
    // absent, so the ordinary ordering is used; the reversed branch needs a builder threaded in.
    const auto builder = arg(args, 1);
    const auto renderer = builder.isObject() ? builder.get("renderer") : JSValue{};
    const bool reversed = renderer.isObject() && renderer.get("reversedDepthBuffer").strictEquals(true);
    const auto step = nativeMathNodeBinding("step");
    const auto hardShadow = reversed ? step.call({}, {mean, shadowCoord.get("z")})
                                     : step.call({}, {shadowCoord.get("z"), mean});

    // const output = float( 1 ).toVar();  default, fully lit
    const auto output = invoke(floatNode(1), "toVar")();

    // If( hardShadow.notEqual( 1.0 ), () => { ... } )
    const auto condition = invoke(hardShadow, "notEqual")({JSValue(1.0)});
    auto body = JSValue::function(
        +[](const JSValue&, const Args&, const Args& captures) -> JSValue {
            const auto shadowCoordRef = captures[0];
            const auto meanRef = captures[1];
            const auto varianceRef = captures[2];
            const auto hardShadowRef = captures[3];
            const auto outputRef = captures[4];

            // Distance from the mean, then Chebyshev's upper bound on the probability.
            const auto difference = nativeOperatorNodeBinding("sub").call(
                {}, {shadowCoordRef.get("z"), meanRef});
            auto pMax = invoke(varianceRef, "div")({
                nativeOperatorNodeBinding("add").call(
                    {}, {varianceRef, invoke(difference, "mul")({difference})})});
            // Remap [amount, 1] to [0, 1] to reduce light bleeding.
            pMax = nativeMathNodeBinding("clamp").call(
                {}, {invoke(nativeOperatorNodeBinding("sub").call({}, {pMax, JSValue(0.3)}),
                            "div")({JSValue(0.65)})});
            invoke(outputRef, "assign")(
                {nativeMathNodeBinding("max").call({}, {hardShadowRef, pMax})});
            return {};
        },
        {shadowCoord, mean, variance, hardShadow, output}, "", 0);

    const auto If = nativeTSLBaseBinding("If");
    if (!If.isCallable()) {
        // The source has no fallback here; failing loudly is better than returning a wrong value.
        throw std::runtime_error("ShadowFilterNode: TSL If is unavailable in this runtime");
    }
    If.call({}, {condition, body});
    return output;
}
}  // namespace

JSValue nativeShadowFilterNodeModule() {
    using namespace LightingNodeRuntime;
    static thread_local JSWeakValue weak;
    if (const auto cached = weak.lock(); cached && cached->isObject()) return *cached;
    static thread_local bool initializing = false;
    JSModuleInitializationScope scope(initializing, "ShadowFilterNode module");
    nativeTSLCore();
    nativeTSLBase();
    nativeTextureNodeModule();
    nativeReferenceNodeModule();
    nativeScreenNodeModule();
    nativeMathNodeModule();
    nativeOperatorNodeModule();
    nativePostProcessingUtilsModule();

    auto bindings = JSValue::object(nullptr);
    own(bindings, "BasicShadowFilter", makeFn(basicShadowFilterBody));
    own(bindings, "PCFShadowFilter", makeFn(pcfShadowFilterBody));
    own(bindings, "VSMShadowFilter", makeFn(vsmShadowFilterBody));

    auto module = JSValue::object(nullptr);
    for (const auto& key : bindings.ownPropertyNames()) own(module, key.c_str(), bindings.get(key));
    bindings.retainForHeapLifetime();
    lexical() = JSWeakValue(bindings);
    module.retainForHeapLifetime();
    weak = JSWeakValue(module);
    return module;
}

JSValue nativeShadowFilterNodeBinding(const std::string& name) {
    const auto keeper = nativeShadowFilterNodeModule();
    (void) keeper;
    return lexical().lock()->get(name);
}
