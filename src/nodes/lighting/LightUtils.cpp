#include "nodes/lighting/LightUtils.h"

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
    const auto lightDistance = inputs.get("lightDistance");
    const auto cutoffDistance = inputs.get("cutoffDistance");
    const auto decayExponent = inputs.get("decayExponent");

    auto distanceFalloff = invoke(lightDistance, "pow")({decayExponent});
    distanceFalloff = invoke(distanceFalloff, "max")({0.01});
    distanceFalloff = invoke(distanceFalloff, "reciprocal")();

    const auto cutoffPositive = invoke(cutoffDistance, "greaterThan")({0});
    auto cutoffWindow = invoke(lightDistance, "div")({cutoffDistance});
    cutoffWindow = invoke(cutoffWindow, "pow4")();
    cutoffWindow = invoke(cutoffWindow, "oneMinus")();
    cutoffWindow = invoke(cutoffWindow, "clamp")();
    cutoffWindow = invoke(cutoffWindow, "pow2")();
    const auto attenuated = invoke(distanceFalloff, "mul")({cutoffWindow});

    return invoke(cutoffPositive, "select")({attenuated, distanceFalloff});
}
} // namespace

JSValue nativeLightUtilsModule() {
    using namespace TSLFunctionRuntime;
    static thread_local JSWeakValue weak;
    if (const auto cached = weak.lock(); cached && cached->isObject()) return *cached;

    static thread_local bool initializing = false;
    JSModuleInitializationScope scope(initializing, "LightUtils module");
    auto callback = JSValue::function(body, {}, "", 1);
    callback.setFunctionSourceText(
        "( { lightDistance, cutoffDistance, decayExponent } ) => {\r\n\r\n"
        "\t// based upon Frostbite 3 Moving to Physically-based Rendering\r\n"
        "\t// page 32, equation 26: E[window1]\r\n"
        "\t// https://seblagarde.files.wordpress.com/2015/07/course_notes_moving_frostbite_to_pbr_v32.pdf\r\n"
        "\tconst distanceFalloff = lightDistance.pow( decayExponent ).max( 0.01 ).reciprocal();\r\n\r\n"
        "\treturn cutoffDistance.greaterThan( 0 ).select(\r\n"
        "\t\tdistanceFalloff.mul( lightDistance.div( cutoffDistance ).pow4().oneMinus().clamp().pow2() ),\r\n"
        "\t\tdistanceFalloff\r\n"
        "\t);\r\n\r\n"
        "}");
    auto layout = JSValue::object();
    own(layout, "lightDistance", "float");
    own(layout, "cutoffDistance", "float");
    own(layout, "decayExponent", "float");
    own(layout, "return", "float");
    const auto value = nativeTSLBaseBinding("Fn").call({}, {callback, layout});

    auto bindings = JSValue::object(nullptr);
    own(bindings, "getDistanceAttenuation", value);
    bindings.retainForHeapLifetime();
    lexical() = JSWeakValue(bindings);

    auto module = JSValue::object(nullptr);
    own(module, "getDistanceAttenuation", value);
    module.retainForHeapLifetime();
    weak = JSWeakValue(module);
    return module;
}

JSValue nativeLightUtilsBinding(const std::string& name) {
    const auto keeper = nativeLightUtilsModule();
    (void)keeper;
    return lexical().lock()->get(name);
}

JSValue nativeGetDistanceAttenuation() {
    return nativeLightUtilsBinding("getDistanceAttenuation");
}
