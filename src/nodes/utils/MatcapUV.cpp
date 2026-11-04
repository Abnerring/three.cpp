#include "nodes/utils/MatcapUV.h"

#include "nodes/accessors/Normal.h"
#include "nodes/accessors/Position.h"
#include "nodes/tsl/TSLBase.h"
#include "nodes/tsl/TSLFunctionRuntime.h"

namespace {
using namespace TSLFunctionRuntime;

JSWeakValue& lexical() {
    static thread_local JSWeakValue value;
    return value;
}

JSValue body(const JSValue&, const Args&, const Args& captures) {
    const auto table = captures[0];
    const auto direction = table.get("positionViewDirection");
    const auto normal = table.get("normalView");
    const auto vec3 = table.get("vec3");
    const auto vec2 = table.get("vec2");

    const auto x = invoke(vec3.call({}, {direction.get("z"), 0,
                                          invoke(direction.get("x"), "negate")()}),
                          "normalize")();
    const auto y = invoke(direction, "cross")({x});
    const auto uv = vec2.call({}, {invoke(x, "dot")({normal}), invoke(y, "dot")({normal})});
    return invoke(invoke(uv, "mul")({0.495}), "add")({0.5});
}
} // namespace

JSValue nativeMatcapUVModule() {
    using namespace TSLFunctionRuntime;
    static thread_local JSWeakValue weak;
    if (const auto cached = weak.lock(); cached && cached->isObject()) return *cached;

    static thread_local bool initializing = false;
    JSModuleInitializationScope scope(initializing, "MatcapUV module");
    nativeNormalModule();
    nativePositionModule();
    auto table = JSValue::object(nullptr);
    own(table, "normalView", nativeNormalBinding("normalView"));
    own(table, "positionViewDirection", nativePositionBinding("positionViewDirection"));
    own(table, "vec2", nativeTSLBaseBinding("vec2"));
    own(table, "vec3", nativeTSLBaseBinding("vec3"));

    auto callback = JSValue::function(body, {table}, "", 0);
    callback.setFunctionSourceText("() => {\r\n\r\n\tconst x = vec3( positionViewDirection.z, 0, positionViewDirection.x.negate() ).normalize();\r\n\tconst y = positionViewDirection.cross( x );\r\n\r\n\treturn vec2( x.dot( normalView ), y.dot( normalView ) ).mul( 0.495 ).add( 0.5 ); // 0.495 to remove artifacts caused by undersized matcap disks\r\n\r\n}");
    auto scopes = JSValue::array();
    own(scopes, "0", "NORMAL");
    own(scopes, "1", "VERTEX");
    const auto once = invoke(nativeTSLBaseBinding("Fn").call({}, {callback}), "once")({scopes});
    const auto matcapUV = invoke(once.call(), "toVar")({"matcapUV"});

    auto bindings = JSValue::object(nullptr);
    own(bindings, "matcapUV", matcapUV);
    auto module = JSValue::object(nullptr);
    own(module, "matcapUV", matcapUV);
    bindings.retainForHeapLifetime();
    lexical() = JSWeakValue(bindings);
    module.retainForHeapLifetime();
    weak = JSWeakValue(module);
    return module;
}

JSValue nativeMatcapUVBinding(const std::string& name) {
    const auto keeper = nativeMatcapUVModule();
    (void)keeper;
    return lexical().lock()->get(name);
}

JSValue nativeMatcapUV() {
    return nativeMatcapUVBinding("matcapUV");
}
