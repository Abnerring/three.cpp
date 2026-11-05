// Complete native TSL port of pinned MaterialXColorTransform.js (11738a). No script evaluator.
#include "nodes/materialx/MaterialXCore.h"
#include "nodes/materialx/MaterialXNoise.h"
#include "nodes/materialx/MaterialXNodes.h"
#include "nodes/materialx/MaterialXColor.h"
#include "nodes/materialx/MaterialXColorTransform.h"
#include "core/JSGlobal.h"
#include "core/JSRegExp.h"
#include "nodes/lighting/LightingNodeRuntime.h"
#include "nodes/tsl/TSLBase.h"
#include "nodes/math/MathNode.h"
#include "nodes/math/OperatorNode.h"
#include "nodes/math/ConditionalNode.h"
#include "nodes/utils/FunctionOverloadingNode.h"
#include "nodes/utils/LoopNode.h"
#include "nodes/accessors/UV.h"
#include "nodes/utils/Timer.h"
#include <cmath>
#include <cstdint>
#include <algorithm>

namespace {
using namespace LightingNodeRuntime;
#include "MaterialXColorTransformSources.inc"
JSWeakValue& lexical(){static thread_local JSWeakValue value;return value;}
JSValue withSource(JSValue value,const char* source){value.setFunctionSourceText(source);return value;}
JSValue nextDestructured(JSIteratorRecord& iterator){if(iterator.done)return {};const auto value=stepJSIterator(iterator);return value?*value:JSValue{};}
JSValue array(const Args& values){auto out=JSValue::array();for(std::size_t i=0;i<values.size();++i)own(out,std::to_string(i).c_str(),values[i]);return out;}
JSValue typeOf(const JSValue& value){if(value.isUndefined())return "undefined";if(value.isCallable())return "function";if(value.isSymbol())return "symbol";switch(value.type()){case JSValue::Type::Boolean:return "boolean";case JSValue::Type::Number:return "number";case JSValue::Type::String:return "string";default:return "object";}}
JSValue leftShift(const JSValue& a,const JSValue& b){const auto x=a.toNumber();const auto y=b.toNumber();return static_cast<std::int32_t>(static_cast<std::uint32_t>(x) << (static_cast<std::uint32_t>(y)&31));}
}

JSValue nativeMaterialXColorTransformModule(){
    static thread_local JSWeakValue weak;if(const auto cached=weak.lock();cached&&cached->isObject())return *cached;
    static thread_local bool initializing=false;JSModuleInitializationScope scope(initializing,"MaterialXColorTransform module");
    auto module=JSValue::object(nullptr);
    own(module, "bvec3", nativeTSLBaseBinding("bvec3"));
    own(module, "vec3", nativeTSLBaseBinding("vec3"));
    own(module, "Fn", nativeTSLBaseBinding("Fn"));
    own(module, "greaterThan", nativeOperatorNodeBinding("greaterThan"));
    own(module, "max", nativeMathNodeBinding("max"));
    own(module, "pow", nativeMathNodeBinding("pow"));
    own(module, "mix", nativeMathNodeBinding("mix"));
    own(module, "mx_srgb_texture_to_lin_rec709", module.get("Fn").call({}, {withSource(JSValue::function(+[](const JSValue& self, const Args& args, const Args& captures) -> JSValue {
        auto module = captures.at(0);
        auto iterator1 = getJSIterator(arg(args, 0));
        auto color_immutable = nextDestructured(iterator1);
        if (!iterator1.done) closeJSIterator(iterator1);
        auto color = invoke(module.get("vec3").call({}, {color_immutable}), "toVar")({});
        auto isAbove = invoke(module.get("bvec3").call({}, {module.get("greaterThan").call({}, {color, module.get("vec3").call({}, {JSValue(0.04045)})})}), "toVar")({});
        auto linSeg = invoke(module.get("vec3").call({}, {invoke(color, "div")({JSValue(12.92)})}), "toVar")({});
        auto powSeg = invoke(module.get("vec3").call({}, {module.get("pow").call({}, {invoke(module.get("max").call({}, {invoke(color, "add")({module.get("vec3").call({}, {JSValue(0.055)})}), module.get("vec3").call({}, {JSValue(0.0)})}), "div")({JSValue(1.055)}), module.get("vec3").call({}, {JSValue(2.4)})})}), "toVar")({});
        return module.get("mix").call({}, {linSeg, powSeg, isAbove});
        return JSValue{};
    }, {module}, "", 1), callback1Source), ([&]() { auto object1 = JSValue::object(); own(object1, "color", JSValue("vec3")); own(object1, "return", JSValue("vec3")); return object1; }())}));
    module.retainForHeapLifetime();lexical()=JSWeakValue(module);auto facade=JSValue::object();
    own(facade, "mx_srgb_texture_to_lin_rec709", module.get("mx_srgb_texture_to_lin_rec709"));
    facade.retainForHeapLifetime();weak=JSWeakValue(facade);return facade;
}
JSValue nativeMaterialXColorTransformBinding(const std::string& name){nativeMaterialXColorTransformModule();return lexical().lock()->get(name);}
JSValue nativeMaterialXColorTransformPrivate(const std::string& name){return nativeMaterialXColorTransformBinding(name);}
