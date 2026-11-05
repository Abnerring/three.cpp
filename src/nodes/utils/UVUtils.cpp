#include "nodes/utils/UVUtils.h"

#include "nodes/core/ContextNode.h"
#include "nodes/tsl/TSLBase.h"
#include "nodes/tsl/TSLFunctionRuntime.h"
#include "nodes/utils/RotateNode.h"

namespace {
using namespace TSLFunctionRuntime;

JSWeakValue& lexical() {
    static thread_local JSWeakValue weak;
    return weak;
}

JSValue constantUV(const JSValue&, const Args&, const Args& captures) {
    return captures[0];
}

JSValue replaceDefaultUV(const JSValue&, const Args& args, const Args&) {
    const auto callback = arg(args, 0);
    const auto node = args.size() > 1 && !args[1].isUndefined() ? args[1] : JSValue(nullptr);
    auto getUV = callback.isCallable()
        ? callback
        : JSValue::function(constantUV, {callback}, "", 0);
    if (!callback.isCallable()) getUV.setFunctionSourceText("() => callback");
    auto options = JSValue::object(nullptr);
    own(options, "getUV", getUV);
    return nativeContextNodeBinding("context").call({}, {node, options});
}

JSValue rotateUV(const JSValue&, const Args& args, const Args& captures) {
    const auto uv = arg(args, 0);
    const auto rotation = arg(args, 1);
    const auto center = args.size() > 2 && !args[2].isUndefined()
        ? args[2] : captures[0].call({}, {0.5});
    const auto delta = invoke(uv, "sub")({center});
    const auto rotated = captures[1].call({}, {delta, rotation});
    return invoke(rotated, "add")({center});
}

JSValue spherizeUV(const JSValue&, const Args& args, const Args& captures) {
    const auto uv = arg(args, 0);
    const auto strength = arg(args, 1);
    const auto center = args.size() > 2 && !args[2].isUndefined()
        ? args[2] : captures[0].call({}, {0.5});
    const auto delta = invoke(uv, "sub")({center});
    const auto delta2 = invoke(delta, "dot")({delta});
    const auto delta4 = invoke(delta2, "mul")({delta2});
    const auto deltaOffset = invoke(delta4, "mul")({strength});
    return invoke(uv, "add")({invoke(delta, "mul")({deltaOffset})});
}

constexpr const char* kReplaceSource =
    "( callback, node = null ) => {\r\n\r\n"
    "\tconst getUV = typeof callback === 'function' ? callback : () => callback;\r\n\r\n"
    "\treturn context( node, { getUV } );\r\n\r\n"
    "}";
constexpr const char* kRotateSource =
    "( [ uv, rotation, center = vec2( 0.5 ) ] ) => {\r\n\r\n"
    "\treturn rotate( uv.sub( center ), rotation ).add( center );\r\n\r\n"
    "}";
constexpr const char* kSpherizeSource =
    "( [ uv, strength, center = vec2( 0.5 ) ] ) => {\r\n\r\n"
    "\tconst delta = uv.sub( center );\r\n"
    "\tconst delta2 = delta.dot( delta );\r\n"
    "\tconst delta4 = delta2.mul( delta2 );\r\n"
    "\tconst deltaOffset = delta4.mul( strength );\r\n\r\n"
    "\treturn uv.add( delta.mul( deltaOffset ) );\r\n\r\n"
    "}";
}

JSValue nativeUVUtilsModule() {
    using namespace TSLFunctionRuntime;
    static thread_local JSWeakValue weak;
    if (const auto cached = weak.lock(); cached && cached->isObject()) return *cached;
    static thread_local bool initializing = false;
    JSModuleInitializationScope scope(initializing, "UVUtils module");
    nativeTSLBase();
    nativeContextNodeModule();
    nativeRotateNodeModule();

    auto bindings = JSValue::object(nullptr);
    auto replace = JSValue::function(replaceDefaultUV, {}, "replaceDefaultUV", 2);
    replace.setFunctionSourceText(kReplaceSource);
    own(bindings, "replaceDefaultUV", replace);

    auto vec2 = nativeTSLBaseBinding("vec2");
    auto rotate = JSValue::function(rotateUV, {vec2, nativeRotateNodeBinding("rotate")}, "rotateUV", 3);
    rotate.setFunctionSourceText(kRotateSource);
    own(bindings, "rotateUV", nativeTSLBaseBinding("Fn").call({}, {rotate}));

    auto spherize = JSValue::function(spherizeUV, {vec2}, "spherizeUV", 3);
    spherize.setFunctionSourceText(kSpherizeSource);
    own(bindings, "spherizeUV", nativeTSLBaseBinding("Fn").call({}, {spherize}));

    bindings.retainForHeapLifetime();
    lexical() = JSWeakValue(bindings);
    auto module = JSValue::object(nullptr);
    for (const auto& name : bindings.ownPropertyNames()) own(module, name.c_str(), bindings.get(name));
    module.retainForHeapLifetime();
    weak = JSWeakValue(module);
    return module;
}

JSValue nativeUVUtilsBinding(const std::string& name) {
    const auto keeper = nativeUVUtilsModule();
    (void)keeper;
    return lexical().lock()->get(name);
}

JSValue nativeReplaceDefaultUV() { return nativeUVUtilsBinding("replaceDefaultUV"); }
JSValue nativeRotateUV() { return nativeUVUtilsBinding("rotateUV"); }
JSValue nativeSpherizeUV() { return nativeUVUtilsBinding("spherizeUV"); }
