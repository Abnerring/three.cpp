#include "nodes/functions/material/getParallaxCorrectNormal.h"

#include "nodes/accessors/Position.h"
#include "nodes/tsl/TSLBase.h"
#include "nodes/tsl/TSLFunctionRuntime.h"

namespace {
using namespace TSLFunctionRuntime;

JSWeakValue& lexical() {
    static thread_local JSWeakValue value;
    return value;
}

JSValue body(const JSValue&, const Args& args, const Args&) {
    const auto inputs = arg(args, 0);
    const auto normal = inputs.get(JSValue(0));
    const auto cubeSize = inputs.get(JSValue(1));
    const auto cubePos = inputs.get(JSValue(2));
    const auto positionWorld = nativePositionBinding("positionWorld");
    const auto floatNode = nativeTSLBaseBinding("float");
    const auto vec3Node = nativeTSLBaseBinding("vec3");

    auto nDir = nativeTSLBaseBinding("normalize").call({}, {normal});
    nDir = invoke(nDir, "toVar")();

    auto rbmax = invoke(cubeSize, "mul")({0.5});
    rbmax = invoke(rbmax, "add")({cubePos});
    rbmax = invoke(rbmax, "sub")({positionWorld});
    rbmax = invoke(rbmax, "div")({nDir});
    rbmax = invoke(rbmax, "toVar")();

    auto rbmin = invoke(cubeSize, "mul")({-0.5});
    rbmin = invoke(rbmin, "add")({cubePos});
    rbmin = invoke(rbmin, "sub")({positionWorld});
    rbmin = invoke(rbmin, "div")({nDir});
    rbmin = invoke(rbmin, "toVar")();

    auto rbminmax = invoke(vec3Node.call(), "toVar")();
    const auto zero = floatNode.call({}, {0});
    for (const auto component : {"x", "y", "z"}) {
        const auto direction = nDir.get(component);
        const auto selection = invoke(invoke(direction, "greaterThan")({zero}), "select")({
            rbmax.get(component), rbmin.get(component)});
        invoke(rbminmax.get(component), "assign")({selection});
    }

    auto correction = nativeTSLBaseBinding("min").call({}, {
        rbminmax.get("x"), rbminmax.get("y"), rbminmax.get("z")});
    correction = invoke(correction, "toVar")();
    auto boxIntersection = invoke(positionWorld, "add")({invoke(nDir, "mul")({correction})});
    boxIntersection = invoke(boxIntersection, "toVar")();
    return invoke(boxIntersection, "sub")({cubePos});
}
} // namespace

JSValue nativeGetParallaxCorrectNormalModule() {
    using namespace TSLFunctionRuntime;
    static thread_local JSWeakValue weak;
    if (const auto cached = weak.lock(); cached && cached->isObject()) return *cached;

    static thread_local bool initializing = false;
    JSModuleInitializationScope scope(initializing, "getParallaxCorrectNormal module");
    nativePositionModule();
    auto callback = JSValue::function(body, {}, "", 1);
    callback.setFunctionSourceText("( [ normal, cubeSize, cubePos ] ) => {\r\n\r\n\tconst nDir = normalize( normal ).toVar();\r\n\tconst rbmax = cubeSize.mul( 0.5 ).add( cubePos ).sub( positionWorld ).div( nDir ).toVar();\r\n\tconst rbmin = cubeSize.mul( - 0.5 ).add( cubePos ).sub( positionWorld ).div( nDir ).toVar();\r\n\tconst rbminmax = vec3().toVar();\r\n\trbminmax.x = nDir.x.greaterThan( float( 0 ) ).select( rbmax.x, rbmin.x );\r\n\trbminmax.y = nDir.y.greaterThan( float( 0 ) ).select( rbmax.y, rbmin.y );\r\n\trbminmax.z = nDir.z.greaterThan( float( 0 ) ).select( rbmax.z, rbmin.z );\r\n\r\n\tconst correction = min( rbminmax.x, rbminmax.y, rbminmax.z ).toVar();\r\n\tconst boxIntersection = positionWorld.add( nDir.mul( correction ) ).toVar();\r\n\treturn boxIntersection.sub( cubePos );\r\n\r\n}");
    const auto value = nativeTSLBaseBinding("Fn").call({}, {callback});

    auto bindings = JSValue::object(nullptr);
    own(bindings, "default", value);
    auto module = JSValue::object(nullptr);
    own(module, "default", value);
    bindings.retainForHeapLifetime();
    lexical() = JSWeakValue(bindings);
    module.retainForHeapLifetime();
    weak = JSWeakValue(module);
    return module;
}

JSValue nativeGetParallaxCorrectNormalBinding(const std::string& name) {
    const auto keeper = nativeGetParallaxCorrectNormalModule();
    (void)keeper;
    return lexical().lock()->get(name);
}

JSValue nativeGetParallaxCorrectNormal() {
    return nativeGetParallaxCorrectNormalBinding("default");
}
