#include "nodes/math/TriNoise3D.h"

#include "nodes/math/MathNode.h"
#include "nodes/tsl/TSLBase.h"
#include "nodes/utils/UtilityNodeRuntime.h"

namespace {
using namespace UtilityNodeRuntime;

JSWeakValue& lexical() {
    static thread_local JSWeakValue weak;
    return weak;
}

JSValue triBody(const JSValue&, const Args& args, const Args&) {
    const auto x = arg(args, 0);
    return invoke(invoke(invoke(x, "fract")({}), "sub")({0.5}), "abs")();
}

JSValue tri3Body(const JSValue&, const Args& args, const Args& captures) {
    const auto p = arg(args, 0);
    const auto tri = captures[0];
    const auto vec3 = captures[1];
    const auto z = p.get("z");
    const auto y = p.get("y");
    const auto x = p.get("x");
    const auto first = tri.call({}, {invoke(z, "add")({tri.call({}, {invoke(y, "mul")({1.0})})})});
    const auto second = tri.call({}, {invoke(z, "add")({tri.call({}, {invoke(x, "mul")({1.0})})})});
    const auto third = tri.call({}, {invoke(y, "add")({tri.call({}, {invoke(x, "mul")({1.0})})})});
    return vec3.call({}, {first, second, third});
}

JSValue triNoiseBody(const JSValue&, const Args& args, const Args& captures) {
    const auto inputs = arg(args, 0);
    const auto position = inputs.get(JSValue(0));
    const auto speed = inputs.get(JSValue(1));
    const auto time = inputs.get(JSValue(2));
    const auto vec3 = captures[0];
    const auto floatNode = captures[1];
    const auto tri = captures[2];
    const auto tri3 = captures[3];

    auto p = invoke(vec3.call({}, {position}), "toVar")();
    auto z = invoke(floatNode.call({}, {1.4}), "toVar")();
    auto rz = invoke(floatNode.call({}, {0.0}), "toVar")();
    auto bp = invoke(vec3.call({}, {p}), "toVar")();

    // The source loop has a constant range 0..3 inclusive. Keeping the four iterations
    // explicit preserves the generated expression graph while avoiding host-side execution.
    for (int i = 0; i < 4; ++i) {
        const auto dg = invoke(tri3.call({}, {invoke(bp, "mul")({2.0})}), "toVar")();
        const auto phase = invoke(time, "mul")({invoke(floatNode.call({}, {0.1}), "mul")({speed})});
        invoke(p, "addAssign")({invoke(dg, "add")({phase})});
        invoke(bp, "mulAssign")({1.8});
        invoke(z, "mulAssign")({1.5});
        invoke(p, "mulAssign")({1.2});
        const auto nested = tri.call({}, {invoke(p.get("x"), "add")({tri.call({}, {p.get("y")})})});
        const auto t = invoke(tri.call({}, {invoke(p.get("z"), "add")({nested})}), "toVar")();
        invoke(rz, "addAssign")({invoke(t, "div")({z})});
        invoke(bp, "addAssign")({0.14});
    }

    return rz;
}

} // namespace

JSValue nativeTriNoise3DModule() {
    using namespace UtilityNodeRuntime;
    static thread_local JSWeakValue weak;
    if (const auto cached = weak.lock(); cached && cached->isObject()) return *cached;

    static thread_local bool initializing = false;
    JSModuleInitializationScope scope(initializing, "TriNoise3D module");
    nativeTSLBase();
    nativeMathNodeModule();

    auto module = JSValue::object(nullptr);
    const auto vec3 = nativeTSLBaseBinding("vec3");
    const auto floatNode = nativeTSLBaseBinding("float");
    own(module, "vec3", vec3);
    own(module, "float", floatNode);

    auto triCallback = JSValue::function(triBody, {}, "", 1);
    triCallback.setFunctionSourceText("( [ x ] ) => {\r\n\r\n\treturn x.fract().sub( .5 ).abs();\r\n\r\n}");
    auto triLayout = JSValue::object();
    own(triLayout, "x", "float");
    own(triLayout, "return", "float");
    const auto tri = nativeTSLBaseBinding("Fn").call({}, {triCallback, triLayout});

    auto tri3Callback = JSValue::function(tri3Body, {tri, vec3}, "", 1);
    tri3Callback.setFunctionSourceText("( [ p ] ) => {\r\n\r\n\treturn vec3( tri( p.z.add( tri( p.y.mul( 1. ) ) ) ), tri( p.z.add( tri( p.x.mul( 1. ) ) ) ), tri( p.y.add( tri( p.x.mul( 1. ) ) ) ) );\r\n\r\n}");
    auto tri3Layout = JSValue::object();
    own(tri3Layout, "p", "vec3");
    own(tri3Layout, "return", "vec3");
    const auto tri3 = nativeTSLBaseBinding("Fn").call({}, {tri3Callback, tri3Layout});

    auto noiseCallback = JSValue::function(triNoiseBody, {vec3, floatNode, tri, tri3}, "", 1);
    noiseCallback.setFunctionSourceText("( [ position, speed, time ] ) => {\r\n\r\n\tconst p = vec3( position ).toVar();\r\n\tconst z = float( 1.4 ).toVar();\r\n\tconst rz = float( 0.0 ).toVar();\r\n\tconst bp = vec3( p ).toVar();\r\n\r\n\tLoop( { start: float( 0.0 ), end: float( 3.0 ), type: 'float', condition: '<=' }, () => {\r\n\r\n\t\tconst dg = vec3( tri3( bp.mul( 2.0 ) ) ).toVar();\r\n\t\tp.addAssign( dg.add( time.mul( float( 0.1 ).mul( speed ) ) ) );\r\n\t\tbp.mulAssign( 1.8 );\r\n\t\tz.mulAssign( 1.5 );\r\n\t\tp.mulAssign( 1.2 );\r\n\r\n\t\tconst t = float( tri( p.z.add( tri( p.x.add( tri( p.y ) ) ) ) ) ).toVar();\r\n\t\trz.addAssign( t.div( z ) );\r\n\t\tbp.addAssign( 0.14 );\r\n\r\n\t} );\r\n\r\n\treturn rz;\r\n\r\n}");
    auto noiseLayout = JSValue::object();
    own(noiseLayout, "position", "vec3");
    own(noiseLayout, "speed", "float");
    own(noiseLayout, "time", "float");
    own(noiseLayout, "return", "float");
    const auto value = nativeTSLBaseBinding("Fn").call({}, {noiseCallback, noiseLayout});

    auto bindings = JSValue::object(nullptr);
    own(bindings, "triNoise3D", value);
    auto facade = JSValue::object(nullptr);
    own(facade, "triNoise3D", value);
    bindings.retainForHeapLifetime();
    lexical() = JSWeakValue(bindings);
    facade.retainForHeapLifetime();
    weak = JSWeakValue(facade);
    return facade;
}

JSValue nativeTriNoise3DBinding(const std::string& name) {
    const auto keeper = nativeTriNoise3DModule();
    (void)keeper;
    return lexical().lock()->get(name);
}

JSValue nativeTriNoise3D() { return nativeTriNoise3DBinding("triNoise3D"); }

