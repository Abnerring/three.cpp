#include "nodes/functions/material/getGeometryRoughness.h"

#include "nodes/accessors/Normal.h"
#include "nodes/tsl/TSLBase.h"
#include "nodes/tsl/TSLFunctionRuntime.h"

namespace {
using namespace TSLFunctionRuntime;

JSWeakValue& lexical() {
    static thread_local JSWeakValue value;
    return value;
}

JSValue body(const JSValue&, const Args& args, const Args&) {
    const auto builder = arg(args, 0);
    if (invoke(builder.get("geometry"), "hasAttribute")({"normal"}).strictEquals(false))
        return nativeTSLBaseBinding("float").call({}, {0});

    const auto normal = nativeNormalBinding("normalViewGeometry");
    const auto dxy = invoke(invoke(normal, "dFdx")(), "abs")();
    const auto dy = invoke(invoke(normal, "dFdy")(), "abs")();
    const auto maxXY = invoke(dxy, "max")({dy});
    return invoke(invoke(maxXY.get("x"), "max")({maxXY.get("y")}), "max")({maxXY.get("z")});
}
} // namespace

JSValue nativeGetGeometryRoughnessModule() {
    using namespace TSLFunctionRuntime;
    static thread_local JSWeakValue weak;
    if (const auto cached = weak.lock(); cached && cached->isObject()) return *cached;
    static thread_local bool initializing = false;
    JSModuleInitializationScope scope(initializing, "getGeometryRoughness module");
    auto callback = JSValue::function(body, {}, "", 1);
    callback.setFunctionSourceText("( builder ) => {\r\n\r\n\tif ( builder.geometry.hasAttribute( 'normal' ) === false ) {\r\n\r\n\t\treturn float( 0 );\r\n\r\n\t}\r\n\r\n\tconst dxy = normalViewGeometry.dFdx().abs().max( normalViewGeometry.dFdy().abs() );\r\n\tconst geometryRoughness = dxy.x.max( dxy.y ).max( dxy.z );\r\n\r\n\treturn geometryRoughness;\r\n\r\n}");
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

JSValue nativeGetGeometryRoughnessBinding(const std::string& name) {
    const auto keeper = nativeGetGeometryRoughnessModule();
    (void)keeper;
    return lexical().lock()->get(name);
}

JSValue nativeGetGeometryRoughness() {
    return nativeGetGeometryRoughnessBinding("default");
}
