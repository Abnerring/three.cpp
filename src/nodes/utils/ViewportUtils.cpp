#include "nodes/utils/ViewportUtils.h"

#include "nodes/display/ScreenNode.h"
#include "nodes/display/ViewportDepthNode.h"
#include "nodes/display/ViewportDepthTextureNode.h"
#include "nodes/tsl/TSLBase.h"
#include "nodes/tsl/TSLFunctionRuntime.h"

namespace {
using namespace TSLFunctionRuntime;

JSWeakValue& lexical() {
    static thread_local JSWeakValue weak;
    return weak;
}

JSValue body(const JSValue&, const Args& args, const Args& captures) {
    const auto table = captures[0];
    auto uv = arg(args, 0);
    if (uv.isUndefined()) uv = JSValue(nullptr);

    const auto depth = table.get("linearDepth").call();
    const auto viewportDepth = table.get("viewportDepthTexture").call({}, {uv});
    const auto depthDiff = invoke(table.get("linearDepth").call({}, {viewportDepth}), "sub")({depth});
    const auto selected = invoke(depthDiff, "lessThan")({0});
    return invoke(selected, "select")({table.get("screenUV"), uv});
}

constexpr const char* kSource =
    "( [ uv = null ] ) => {\r\n\r\n"
    "\tconst depth = linearDepth();\r\n"
    "\tconst depthDiff = linearDepth( viewportDepthTexture( uv ) ).sub( depth );\r\n"
    "\tconst finalUV = depthDiff.lessThan( 0 ).select( screenUV, uv );\r\n\r\n"
    "\treturn finalUV;\r\n\r\n"
    "}";

} // namespace

JSValue nativeViewportUtilsModule() {
    using namespace TSLFunctionRuntime;
    static thread_local JSWeakValue weak;
    if (const auto cached = weak.lock(); cached && cached->isObject()) return *cached;

    static thread_local bool initializing = false;
    JSModuleInitializationScope scope(initializing, "ViewportUtils module");
    nativeTSLBase();
    nativeScreenNodeModule();
    nativeViewportDepthNodeModule();
    nativeViewportDepthTextureNodeModule();

    auto table = JSValue::object(nullptr);
    own(table, "screenUV", nativeScreenNodeBinding("screenUV"));
    own(table, "viewportDepthTexture", nativeViewportDepthTextureNodeBinding("viewportDepthTexture"));
    own(table, "linearDepth", nativeViewportDepthNodeBinding("linearDepth"));

    auto callback = JSValue::function(body, {table}, "", 1);
    callback.setFunctionSourceText(kSource);
    const auto value = nativeTSLBaseBinding("Fn").call({}, {callback});

    auto bindings = JSValue::object(nullptr);
    own(bindings, "viewportSafeUV", value);
    auto module = JSValue::object(nullptr);
    own(module, "viewportSafeUV", value);
    bindings.retainForHeapLifetime();
    lexical() = JSWeakValue(bindings);
    module.retainForHeapLifetime();
    weak = JSWeakValue(module);
    return module;
}

JSValue nativeViewportUtilsBinding(const std::string& name) {
    const auto keeper = nativeViewportUtilsModule();
    (void)keeper;
    return lexical().lock()->get(name);
}

JSValue nativeViewportSafeUV() {
    return nativeViewportUtilsBinding("viewportSafeUV");
}
