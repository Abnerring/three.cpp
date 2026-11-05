#include "nodes/functions/BSDF/LTC.h"

#include "nodes/lighting/LightingNodeRuntime.h"
#include "nodes/math/MathNode.h"
#include "nodes/tsl/TSLBase.h"
#include "nodes/tsl/TSLCore.h"

namespace {
using namespace LightingNodeRuntime;

JSWeakValue& lexical() { static thread_local JSWeakValue weak; return weak; }
// The two internal Fn helpers are built once and reused by the three exports.
JSWeakValue& clippedSphereCache() { static thread_local JSWeakValue weak; return weak; }
JSWeakValue& edgeVectorCache() { static thread_local JSWeakValue weak; return weak; }

JSValue vec2Node(double x, double y) {
    return nativeTSLBaseBinding("vec2").call({}, {JSValue(x), JSValue(y)});
}
JSValue floatNode(double value) {
    return nativeTSLBaseBinding("float").call({}, {JSValue(value)});
}

// Builds the single named input object the internal Fn helpers take.
JSValue makeInput(const char* key, const JSValue& value) {
    auto inputs = JSValue::object(nullptr);
    inputs.set(key, value);
    return inputs;
}

// The callbacks destructure the named inputs object supplied by TSL.
JSValue inputObject(const Args& args) {
    return arg(args, 0);
}

JSValue makeFn(JSValue::NativeFunction body,
               std::initializer_list<std::pair<const char*, const char*>> fields) {
    auto callback = JSValue::function(body, {}, "", 1);
    auto layout = JSValue::object();
    for (const auto& [name, type] : fields) own(layout, name, type);
    return nativeTSLCoreBinding("Fn").call({}, {callback, layout});
}

// LTC_ClippedSphereFormFactor = Fn( ( { f } ) => { const l = f.length(); return max( l.mul( l ).add( f.z ).div( l.add( 1.0 ) ), 0 ); } )
JSValue clippedSphereFormFactorBody(const JSValue&, const Args& args, const Args&) {
    const auto f = inputObject(args).get("f");
    const auto l = invoke(f, "length")();
    const auto numerator =
        invoke(invoke(invoke(l, "mul")({l}), "add")({f.get("z")}), "div")({invoke(l, "add")({JSValue(1.0)})});
    return nativeMathNodeBinding("max").call({}, {numerator, JSValue(0)});
}

JSValue clippedSphereFormFactor() {
    if (const auto cached = clippedSphereCache().lock(); cached && cached->isCallable()) return *cached;
    const auto fn = makeFn(clippedSphereFormFactorBody, {{"f", "vec3"}, {"return", "float"}});
    clippedSphereCache() = JSWeakValue(fn);
    return fn;
}

// LTC_EdgeVectorFormFactor = Fn( ( { v1, v2 } ) => { ... } )
JSValue edgeVectorFormFactorBody(const JSValue&, const Args& args, const Args&) {
    const auto inputs = inputObject(args);
    const auto v1 = inputs.get("v1");
    const auto v2 = inputs.get("v2");

    const auto x = invoke(v1, "dot")({v2});
    auto y = invoke(invoke(x, "abs")(), "toVar")();

    // rational polynomial approximation to theta / sin( theta ) / 2PI
    auto a = invoke(invoke(invoke(invoke(y, "mul")({JSValue(0.0145206)}), "add")({JSValue(0.4965155)}),
                           "mul")({y}),
                    "add")({JSValue(0.8543985)});
    a = invoke(a, "toVar")();
    auto b = invoke(invoke(invoke(y, "add")({JSValue(4.1616724)}), "mul")({y}),
                    "add")({JSValue(3.4175940)});
    b = invoke(b, "toVar")();
    const auto v = invoke(a, "div")({b});

    // x.greaterThan( 0.0 ).select( v, max( x.mul( x ).oneMinus(), 1e-7 ).inverseSqrt().mul( 0.5 ).sub( v ) )
    const auto fallback = invoke(
        invoke(invoke(nativeMathNodeBinding("max").call(
                   {}, {invoke(invoke(x, "mul")({x}), "oneMinus")(), JSValue(1e-7)}),
               "inverseSqrt")(),
               "mul")({JSValue(0.5)}),
        "sub")({v});
    const auto thetaSinTheta = invoke(invoke(x, "greaterThan")({JSValue(0.0)}), "select")({v, fallback});

    return invoke(invoke(v1, "cross")({v2}), "mul")({thetaSinTheta});
}

JSValue edgeVectorFormFactor() {
    if (const auto cached = edgeVectorCache().lock(); cached && cached->isCallable()) return *cached;
    const auto fn = makeFn(edgeVectorFormFactorBody, {{"v1", "vec3"}, {"v2", "vec3"}, {"return", "vec3"}});
    edgeVectorCache() = JSWeakValue(fn);
    return fn;
}

// The four edges of the rect each contribute one form factor term.
JSValue edgeSum(const JSValue& c0, const JSValue& c1, const JSValue& c2, const JSValue& c3) {
    const auto add = [](const JSValue& target, const JSValue& value) {
        invoke(target, "addAssign")({value});
    };
    auto sum = invoke(nativeTSLBaseBinding("vec3").call({}, {JSValue(0)}), "toVar")();
    const auto formFactor = edgeVectorFormFactor();
    auto pair = [&](const JSValue& left, const JSValue& right) {
        auto inputs = JSValue::object(nullptr);
        inputs.set("v1", left);
        inputs.set("v2", right);
        return formFactor.call({}, {inputs});
    };
    add(sum, pair(c0, c1));
    add(sum, pair(c1, c2));
    add(sum, pair(c2, c3));
    add(sum, pair(c3, c0));
    return sum;
}

// LTC_Uv = Fn( ( { N, V, roughness } ) => { ... }, { N: 'vec3', V: 'vec3', roughness: 'float', return: 'vec2' } )
JSValue ltcUvBody(const JSValue&, const Args& args, const Args&) {
    const auto inputs = inputObject(args);
    const auto N = inputs.get("N");
    const auto V = inputs.get("V");
    const auto roughness = inputs.get("roughness");

    constexpr double LUT_SIZE = 64.0;
    constexpr double LUT_SCALE = (LUT_SIZE - 1.0) / LUT_SIZE;
    constexpr double LUT_BIAS = 0.5 / LUT_SIZE;

    const auto dotNV = invoke(invoke(N, "dot")({V}), "saturate")();
    // texture parameterized by sqrt( GGX alpha ) and sqrt( 1 - cos( theta ) )
    auto uv = nativeTSLBaseBinding("vec2").call(
        {}, {roughness, invoke(invoke(dotNV, "oneMinus")(), "sqrt")()});
    invoke(uv, "assign")(
        {invoke(invoke(uv, "mul")({JSValue(LUT_SCALE)}), "add")({JSValue(LUT_BIAS)})});
    return uv;
}

// LTC_Evaluate = Fn( ( { N, V, P, mInv, p0, p1, p2, p3 } ) => { ... } )
JSValue ltcEvaluateBody(const JSValue&, const Args& args, const Args&) {
    const auto inputs = inputObject(args);
    const auto N = inputs.get("N");
    const auto V = inputs.get("V");
    const auto P = inputs.get("P");
    const auto mInv = inputs.get("mInv");
    const auto p0 = inputs.get("p0");
    const auto p1 = inputs.get("p1");
    const auto p2 = inputs.get("p2");
    const auto p3 = inputs.get("p3");

    // bail if point is on back side of plane of light, assuming ccw winding order
    const auto v1 = invoke(invoke(p1, "sub")({p0}), "toVar")();
    const auto v2 = invoke(invoke(p3, "sub")({p0}), "toVar")();
    const auto lightNormal = invoke(v1, "cross")({v2});
    auto result = invoke(nativeTSLBaseBinding("vec3").call({}, {}), "toVar")();

    const auto condition =
        invoke(lightNormal, "dot")({invoke(P, "sub")({p0})});
    const auto zero = JSValue(0);
    const auto guard = invoke(condition, "greaterThanEqual")({zero});

    auto body = JSValue::function(
        +[](const JSValue&, const Args&, const Args& captures) -> JSValue {
            const auto NRef = captures[0], VRef = captures[1], PRef = captures[2];
            const auto mInvRef = captures[3];
            const auto p0Ref = captures[4], p1Ref = captures[5], p2Ref = captures[6], p3Ref = captures[7];
            const auto resultRef = captures[8];

            // construct orthonormal basis around N
            const auto T1 = invoke(
                invoke(VRef, "sub")({invoke(NRef, "mul")({invoke(VRef, "dot")({NRef})})}),
                "normalize")();
            // negated from the paper, possibly due to a different handedness
            const auto T2 = invoke(invoke(NRef, "cross")({T1}), "negate")();

            // compute the transform
            auto basis = nativeTSLBaseBinding("mat3").call({}, {T1, T2, NRef});
            auto transposed = invoke(basis, "transpose")();
            auto mat = invoke(invoke(mInvRef, "mul")({transposed}), "toVar")();

            // transform the rect and project it onto the sphere
            const auto transform = [&](const JSValue& point) -> JSValue {
                return invoke(invoke(invoke(mat, "mul")({invoke(point, "sub")({PRef})}), "normalize")(),
                              "toVar")();
            };
            const auto coords0 = transform(p0Ref);
            const auto coords1 = transform(p1Ref);
            const auto coords2 = transform(p2Ref);
            const auto coords3 = transform(p3Ref);

            // calculate the vector form factor and adjust for horizon clipping
            const auto sum = edgeSum(coords0, coords1, coords2, coords3);
            invoke(resultRef, "assign")({nativeTSLBaseBinding("vec3").call(
                {}, {clippedSphereFormFactor().call({}, {makeInput("f", sum)})})});
            return {};
        },
        {N, V, P, mInv, p0, p1, p2, p3, result}, "", 0);

    const auto If = nativeTSLBaseBinding("If");
    if (!If.isCallable()) {
        throw std::runtime_error("LTC: TSL If is unavailable in this runtime");
    }
    If.call({}, {guard, body});
    return result;
}

// LTC_Evaluate_Volume = Fn( ( { P, p0, p1, p2, p3 } ) => { ... } )
JSValue ltcEvaluateVolumeBody(const JSValue&, const Args& args, const Args&) {
    const auto inputs = inputObject(args);
    const auto P = inputs.get("P");
    const auto p0 = inputs.get("p0");
    const auto p1 = inputs.get("p1");
    const auto p2 = inputs.get("p2");
    const auto p3 = inputs.get("p3");

    const auto v1 = invoke(invoke(p1, "sub")({p0}), "toVar")();
    const auto v2 = invoke(invoke(p3, "sub")({p0}), "toVar")();
    const auto lightNormal = invoke(v1, "cross")({v2});
    auto result = invoke(nativeTSLBaseBinding("vec3").call({}, {}), "toVar")();

    const auto guard = invoke(invoke(lightNormal, "dot")({invoke(P, "sub")({p0})}),
                              "greaterThanEqual")({JSValue(0)});

    auto body = JSValue::function(
        +[](const JSValue&, const Args&, const Args& captures) -> JSValue {
            const auto PRef = captures[0];
            const auto p0Ref = captures[1], p1Ref = captures[2], p2Ref = captures[3], p3Ref = captures[4];
            const auto resultRef = captures[5];

            const auto transform = [&](const JSValue& point) -> JSValue {
                return invoke(invoke(invoke(point, "sub")({PRef}), "normalize")(), "toVar")();
            };
            const auto coords0 = transform(p0Ref);
            const auto coords1 = transform(p1Ref);
            const auto coords2 = transform(p2Ref);
            const auto coords3 = transform(p3Ref);

            const auto sum = edgeSum(coords0, coords1, coords2, coords3);
            // The volume variant clips the absolute form factor.
            const auto absolute = invoke(sum, "abs")();
            invoke(resultRef, "assign")({nativeTSLBaseBinding("vec3").call(
                {}, {clippedSphereFormFactor().call({}, {makeInput("f", absolute)})})});
            return {};
        },
        {P, p0, p1, p2, p3, result}, "", 0);

    const auto If = nativeTSLBaseBinding("If");
    if (!If.isCallable()) {
        throw std::runtime_error("LTC: TSL If is unavailable in this runtime");
    }
    If.call({}, {guard, body});
    return result;
}
}  // namespace

JSValue nativeLTCModule() {
    using namespace LightingNodeRuntime;
    static thread_local JSWeakValue weak;
    if (const auto cached = weak.lock(); cached && cached->isObject()) return *cached;
    static thread_local bool initializing = false;
    JSModuleInitializationScope scope(initializing, "LTC module");
    nativeTSLCore();
    nativeTSLBase();
    nativeMathNodeModule();

    auto bindings = JSValue::object(nullptr);
    own(bindings, "LTC_Uv", makeFn(ltcUvBody, {{"N", "vec3"}, {"V", "vec3"}, {"roughness", "float"}, {"return", "vec2"}}));
    own(bindings, "LTC_Evaluate", makeFn(ltcEvaluateBody, {{"N", "vec3"}, {"V", "vec3"}, {"P", "vec3"}, {"mInv", "mat3"}, {"p0", "vec3"}, {"p1", "vec3"}, {"p2", "vec3"}, {"p3", "vec3"}, {"return", "vec3"}}));
    own(bindings, "LTC_Evaluate_Volume", makeFn(ltcEvaluateVolumeBody, {{"P", "vec3"}, {"p0", "vec3"}, {"p1", "vec3"}, {"p2", "vec3"}, {"p3", "vec3"}, {"return", "vec3"}}));

    auto module = JSValue::object(nullptr);
    for (const auto& key : bindings.ownPropertyNames()) own(module, key.c_str(), bindings.get(key));
    bindings.retainForHeapLifetime();
    lexical() = JSWeakValue(bindings);
    module.retainForHeapLifetime();
    weak = JSWeakValue(module);
    return module;
}

JSValue nativeLTCBinding(const std::string& name) {
    const auto keeper = nativeLTCModule();
    (void) keeper;
    return lexical().lock()->get(name);
}
