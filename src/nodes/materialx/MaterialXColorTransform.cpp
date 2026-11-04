#include "nodes/materialx/MaterialXColorTransform.h"

#include "core/JSModuleObject.h"
#include "nodes/math/MathNode.h"
#include "nodes/tsl/TSLBase.h"

#include <string>
#include <vector>

namespace {
using Args = std::vector<JSValue>;
void own(JSValue object, const char* key, const JSValue& value) { JSPropertyDescriptor d; d.value = value; d.writable = d.enumerable = d.configurable = true; object.defineProperty(key, d); }
JSValue arg(const Args& args, std::size_t i) { return i < args.size() ? args[i] : JSValue{}; }
JSValue firstInput(const Args& args) { const auto v = arg(args, 0); return v.isObject() && !v.get("0").isUndefined() ? v.get("0") : v; }
struct Invocation { JSValue object, method; JSValue operator()(const Args& args = {}) const { return method.call(object, args); } };
Invocation invoke(const JSValue& o, const char* n) { return {o, o.get(n)}; }

JSValue body(const JSValue&, const Args& args, const Args& captures) {
    const auto module = captures[0];
    const auto color = invoke(module.get("vec3").call({}, {firstInput(args)}), "toVar")();
    const auto above = invoke(module.get("bvec3").call({}, {invoke(color, "greaterThan")({module.get("vec3").call({}, {0.04045})})}), "toVar")();
    const auto linear = invoke(module.get("vec3").call({}, {invoke(color, "div")({12.92})}), "toVar")();
    const auto positive = nativeMathNodeBinding("max").call({}, {invoke(color, "add")({module.get("vec3").call({}, {0.055})}), module.get("vec3").call({}, {0.0})});
    const auto powered = invoke(module.get("vec3").call({}, {nativeMathNodeBinding("pow").call({}, {invoke(positive, "div")({1.055}), module.get("vec3").call({}, {2.4})})}), "toVar")();
    return nativeMathNodeBinding("mix").call({}, {linear, powered, above});
}
}

JSValue nativeMaterialXColorTransformModule() {
    static thread_local JSWeakValue weak;
    if (const auto value = weak.lock(); value && value->isObject()) return *value;
    auto module = JSValue::object(nullptr); module.retainForHeapLifetime(); weak = JSWeakValue(module);
    for (const auto* n : {"vec3", "bvec3"}) own(module, n, nativeTSLBaseBinding(n));
    auto callback = JSValue::function(body, {module}, "mx_srgb_texture_to_lin_rec709", 1);
    callback.setFunctionSourceText(R"(( [ color_immutable ] ) => {

	const color = vec3( color_immutable ).toVar();
	const isAbove = bvec3( greaterThan( color, vec3( 0.04045 ) ) ).toVar();
	const linSeg = vec3( color.div( 12.92 ) ).toVar();
	const powSeg = vec3( pow( max( color.add( vec3( 0.055 ) ), vec3( 0.0 ) ).div( 1.055 ), vec3( 2.4 ) ) ).toVar();
	return mix( linSeg, powSeg, isAbove );
} )");
    auto layout = JSValue::object(); own(layout, "color", "vec3"); own(layout, "return", "vec3");
    own(module, "mx_srgb_texture_to_lin_rec709", nativeTSLBaseBinding("Fn").call({}, {callback, layout}));
    auto facade = JSValue::object(); own(facade, "mx_srgb_texture_to_lin_rec709", module.get("mx_srgb_texture_to_lin_rec709")); return facade;
}

JSValue nativeMaterialXColorTransformBinding(const std::string& name) { return nativeMaterialXColorTransformModule().get(name); }

