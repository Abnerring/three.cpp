#include "nodes/functions/material/getRoughness.h"

#include "nodes/functions/material/getGeometryRoughness.h"
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
    const auto roughness = inputs.get("roughness");
    const auto geometryRoughness = nativeGetGeometryRoughness().call({}, {});

    // Minimum roughness, so even a perfect mirror samples a prefiltered level
    // of the environment map. Matches Filament's desktop minimum.
    auto roughnessFactor = invoke(roughness, "max")({0.045});
    roughnessFactor = invoke(roughnessFactor, "add")({geometryRoughness});
    return invoke(roughnessFactor, "min")({1.0});
}
} // namespace

JSValue nativeGetRoughnessModule() {
    using namespace TSLFunctionRuntime;
    static thread_local JSWeakValue weak;
    if (const auto cached = weak.lock(); cached && cached->isObject()) return *cached;

    static thread_local bool initializing = false;
    JSModuleInitializationScope scope(initializing, "getRoughness module");
    nativeGetGeometryRoughnessModule();
    auto callback = JSValue::function(body, {}, "", 1);
    callback.setFunctionSourceText(
        "( inputs ) => {\r\n\r\n\tconst { roughness } = inputs;\r\n\r\n\tconst geometryRoughness = getGeometryRoughness();\r\n\r\n\t// Minimum roughness, so even a perfect mirror samples a prefiltered level of the environment map.\r\n"
        "\t// Matches Filament's desktop MIN_PERCEPTUAL_ROUGHNESS: https:" "//github.com/google/filament/blob/main/shaders/src/surface_material.fs\r\n"
        "\tlet roughnessFactor = roughness.max( 0.045 );\r\n\troughnessFactor = roughnessFactor.add( geometryRoughness );\r\n\troughnessFactor = roughnessFactor.min( 1.0 );\r\n\r\n\treturn roughnessFactor;\r\n\r\n}");
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

JSValue nativeGetRoughnessBinding(const std::string& name) {
    const auto keeper = nativeGetRoughnessModule();
    (void)keeper;
    return lexical().lock()->get(name);
}

JSValue nativeGetRoughness() {
    return nativeGetRoughnessBinding("default");
}
