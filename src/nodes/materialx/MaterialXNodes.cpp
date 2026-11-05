// Complete native TSL port of pinned MaterialXNodes.js (11738a). No script evaluator.
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
#include "MaterialXNodesSources.inc"
JSWeakValue& lexical(){static thread_local JSWeakValue value;return value;}
JSValue withSource(JSValue value,const char* source){value.setFunctionSourceText(source);return value;}
JSValue nextDestructured(JSIteratorRecord& iterator){if(iterator.done)return {};const auto value=stepJSIterator(iterator);return value?*value:JSValue{};}
JSValue array(const Args& values){auto out=JSValue::array();for(std::size_t i=0;i<values.size();++i)own(out,std::to_string(i).c_str(),values[i]);return out;}
JSValue typeOf(const JSValue& value){if(value.isUndefined())return "undefined";if(value.isCallable())return "function";if(value.isSymbol())return "symbol";switch(value.type()){case JSValue::Type::Boolean:return "boolean";case JSValue::Type::Number:return "number";case JSValue::Type::String:return "string";default:return "object";}}
JSValue leftShift(const JSValue& a,const JSValue& b){const auto x=a.toNumber();const auto y=b.toNumber();return static_cast<std::int32_t>(static_cast<std::uint32_t>(x) << (static_cast<std::uint32_t>(y)&31));}
}

JSValue nativeMaterialXNodesModule(){
    static thread_local JSWeakValue weak;if(const auto cached=weak.lock();cached&&cached->isObject())return *cached;
    static thread_local bool initializing=false;JSModuleInitializationScope scope(initializing,"MaterialXNodes module");
    auto module=JSValue::object(nullptr);
    own(module, "mx_perlin_noise_float", nativeMaterialXNoiseBinding("mx_perlin_noise_float"));
    own(module, "mx_perlin_noise_vec3", nativeMaterialXNoiseBinding("mx_perlin_noise_vec3"));
    own(module, "worley_noise_float_2d", nativeMaterialXNoiseBinding("mx_worley_noise_float_2d"));
    own(module, "worley_noise_float_3d", nativeMaterialXNoiseBinding("mx_worley_noise_float_3d"));
    own(module, "worley_noise_vec2", nativeMaterialXNoiseBinding("mx_worley_noise_vec2"));
    own(module, "worley_noise_vec3", nativeMaterialXNoiseBinding("mx_worley_noise_vec3"));
    own(module, "worley_noise_vec3_style", nativeMaterialXNoiseBinding("mx_worley_noise_vec3_style"));
    own(module, "cell_noise_float", nativeMaterialXNoiseBinding("mx_cell_noise_float"));
    own(module, "cell_noise_vec3", nativeMaterialXNoiseBinding("mx_cell_noise_vec3"));
    own(module, "unifiednoise2d", nativeMaterialXNoiseBinding("mx_unifiednoise2d"));
    own(module, "unifiednoise3d", nativeMaterialXNoiseBinding("mx_unifiednoise3d"));
    own(module, "fractal_noise_float_2d", nativeMaterialXNoiseBinding("mx_fractal_noise_float_2d"));
    own(module, "fractal_noise_float", nativeMaterialXNoiseBinding("mx_fractal_noise_float"));
    own(module, "fractal_noise_vec2", nativeMaterialXNoiseBinding("mx_fractal_noise_vec2"));
    own(module, "fractal_noise_vec3", nativeMaterialXNoiseBinding("mx_fractal_noise_vec3"));
    own(module, "fractal_noise_vec4", nativeMaterialXNoiseBinding("mx_fractal_noise_vec4"));
    own(module, "mx_rotate2d", nativeMaterialXCoreBinding("mx_rotate2d"));
    own(module, "mx_rotate3d", nativeMaterialXCoreBinding("mx_rotate3d"));
    own(module, "mx_hsvtorgb", nativeMaterialXColorBinding("mx_hsvtorgb"));
    own(module, "mx_rgbtohsv", nativeMaterialXColorBinding("mx_rgbtohsv"));
    own(module, "mx_srgb_texture_to_lin_rec709", nativeMaterialXColorTransformBinding("mx_srgb_texture_to_lin_rec709"));
    own(module, "Fn", nativeTSLBaseBinding("Fn"));
    own(module, "float", nativeTSLBaseBinding("float"));
    own(module, "vec2", nativeTSLBaseBinding("vec2"));
    own(module, "vec3", nativeTSLBaseBinding("vec3"));
    own(module, "vec4", nativeTSLBaseBinding("vec4"));
    own(module, "int", nativeTSLBaseBinding("int"));
    own(module, "add", nativeTSLBaseBinding("add"));
    own(module, "sub", nativeTSLBaseBinding("sub"));
    own(module, "mul", nativeTSLBaseBinding("mul"));
    own(module, "div", nativeTSLBaseBinding("div"));
    own(module, "atan", nativeTSLBaseBinding("atan"));
    own(module, "mix", nativeTSLBaseBinding("mix"));
    own(module, "pow", nativeTSLBaseBinding("pow"));
    own(module, "smoothstep", nativeTSLBaseBinding("smoothstep"));
    own(module, "floor", nativeTSLBaseBinding("floor"));
    own(module, "abs", nativeTSLBaseBinding("abs"));
    own(module, "max", nativeTSLBaseBinding("max"));
    own(module, "clamp", nativeTSLBaseBinding("clamp"));
    own(module, "step", nativeTSLBaseBinding("step"));
    own(module, "cross", nativeTSLBaseBinding("cross"));
    own(module, "dot", nativeTSLBaseBinding("dot"));
    own(module, "normalize", nativeTSLBaseBinding("normalize"));
    own(module, "overloadingFn", nativeFunctionOverloadingNodeBinding("overloadingFn"));
    own(module, "uv", nativeUVBinding("uv"));
    own(module, "frameId", nativeTimerBinding("frameId"));
    own(module, "time", nativeTimerBinding("time"));
    own(module, "mx_aastep", withSource(JSValue::function(+[](const JSValue& self, const Args& args, const Args& captures) -> JSValue {
        auto module = captures.at(0);
        auto threshold = arg(args, 0);
        auto value = arg(args, 1);
        threshold = module.get("float").call({}, {threshold});
        value = module.get("float").call({}, {value});
        auto afwidth = invoke(invoke(module.get("vec2").call({}, {invoke(value, "dFdx")({}), invoke(value, "dFdy")({})}), "length")({}), "mul")({JSValue(0.7071067811865476)});
        return module.get("smoothstep").call({}, {invoke(threshold, "sub")({afwidth}), invoke(threshold, "add")({afwidth}), value});
        return JSValue{};
    }, {module}, "mx_aastep", 2), callback1Source));
    own(module, "_ramp", withSource(JSValue::function(+[](const JSValue& self, const Args& args, const Args& captures) -> JSValue {
        auto module = captures.at(0);
        auto a = arg(args, 0);
        auto b = arg(args, 1);
        auto uv = arg(args, 2);
        auto p = arg(args, 3);
        return module.get("mix").call({}, {a, b, invoke(uv.get(p), "clamp")({})});
    }, {module}, "_ramp", 4), callback2Source));
    own(module, "mx_ramplr", withSource(JSValue::function(+[](const JSValue& self, const Args& args, const Args& captures) -> JSValue {
        auto module = captures.at(0);
        auto valuel = arg(args, 0);
        auto valuer = arg(args, 1);
        auto argument1 = arg(args, 2);
        if (argument1.isUndefined()) argument1 = module.get("uv").call({}, {});
        auto texcoord = argument1;
        return module.get("_ramp").call({}, {valuel, valuer, texcoord, JSValue("x")});
    }, {module}, "mx_ramplr", 2), callback3Source));
    own(module, "mx_ramptb", withSource(JSValue::function(+[](const JSValue& self, const Args& args, const Args& captures) -> JSValue {
        auto module = captures.at(0);
        auto valueb = arg(args, 0);
        auto valuet = arg(args, 1);
        auto argument1 = arg(args, 2);
        if (argument1.isUndefined()) argument1 = module.get("uv").call({}, {});
        auto texcoord = argument1;
        return module.get("_ramp").call({}, {valueb, valuet, texcoord, JSValue("y")});
    }, {module}, "mx_ramptb", 2), callback4Source));
    own(module, "mx_ramp4", withSource(JSValue::function(+[](const JSValue& self, const Args& args, const Args& captures) -> JSValue {
        auto module = captures.at(0);
        auto valuetl = arg(args, 0);
        auto valuetr = arg(args, 1);
        auto valuebl = arg(args, 2);
        auto valuebr = arg(args, 3);
        auto argument1 = arg(args, 4);
        if (argument1.isUndefined()) argument1 = module.get("uv").call({}, {});
        auto texcoord = argument1;
        auto u = invoke(texcoord.get("x"), "clamp")({});
        auto v = invoke(texcoord.get("y"), "clamp")({});
        auto top = module.get("mix").call({}, {valuetl, valuetr, u});
        auto bottom = module.get("mix").call({}, {valuebl, valuebr, u});
        return module.get("mix").call({}, {bottom, top, v});
        return JSValue{};
    }, {module}, "mx_ramp4", 4), callback5Source));
    own(module, "_split", withSource(JSValue::function(+[](const JSValue& self, const Args& args, const Args& captures) -> JSValue {
        auto module = captures.at(0);
        auto a = arg(args, 0);
        auto b = arg(args, 1);
        auto center = arg(args, 2);
        auto uv = arg(args, 3);
        auto p = arg(args, 4);
        return module.get("mix").call({}, {a, b, module.get("mx_aastep").call({}, {center, uv.get(p)})});
    }, {module}, "_split", 5), callback6Source));
    own(module, "mx_splitlr", withSource(JSValue::function(+[](const JSValue& self, const Args& args, const Args& captures) -> JSValue {
        auto module = captures.at(0);
        auto valuel = arg(args, 0);
        auto valuer = arg(args, 1);
        auto center = arg(args, 2);
        auto argument1 = arg(args, 3);
        if (argument1.isUndefined()) argument1 = module.get("uv").call({}, {});
        auto texcoord = argument1;
        return module.get("_split").call({}, {valuel, valuer, center, texcoord, JSValue("x")});
    }, {module}, "mx_splitlr", 3), callback7Source));
    own(module, "mx_splittb", withSource(JSValue::function(+[](const JSValue& self, const Args& args, const Args& captures) -> JSValue {
        auto module = captures.at(0);
        auto valueb = arg(args, 0);
        auto valuet = arg(args, 1);
        auto center = arg(args, 2);
        auto argument1 = arg(args, 3);
        if (argument1.isUndefined()) argument1 = module.get("uv").call({}, {});
        auto texcoord = argument1;
        return module.get("_split").call({}, {valueb, valuet, center, texcoord, JSValue("y")});
    }, {module}, "mx_splittb", 3), callback8Source));
    own(module, "mx_transform_uv", withSource(JSValue::function(+[](const JSValue& self, const Args& args, const Args& captures) -> JSValue {
        auto module = captures.at(0);
        auto argument1 = arg(args, 0);
        if (argument1.isUndefined()) argument1 = JSValue(1.0);
        auto uv_scale = argument1;
        auto argument2 = arg(args, 1);
        if (argument2.isUndefined()) argument2 = JSValue(0.0);
        auto uv_offset = argument2;
        auto argument3 = arg(args, 2);
        if (argument3.isUndefined()) argument3 = module.get("uv").call({}, {});
        auto uv_geo = argument3;
        return invoke(invoke(uv_geo, "mul")({uv_scale}), "add")({uv_offset});
    }, {module}, "mx_transform_uv", 0), callback9Source));
    own(module, "mx_safepower", withSource(JSValue::function(+[](const JSValue& self, const Args& args, const Args& captures) -> JSValue {
        auto module = captures.at(0);
        auto in1 = arg(args, 0);
        auto argument1 = arg(args, 1);
        if (argument1.isUndefined()) argument1 = JSValue(1.0);
        auto in2 = argument1;
        in1 = module.get("float").call({}, {in1});
        return invoke(invoke(invoke(in1, "abs")({}), "pow")({in2}), "mul")({invoke(in1, "sign")({})});
        return JSValue{};
    }, {module}, "mx_safepower", 1), callback10Source));
    own(module, "mx_contrast", withSource(JSValue::function(+[](const JSValue& self, const Args& args, const Args& captures) -> JSValue {
        auto module = captures.at(0);
        auto input = arg(args, 0);
        auto argument1 = arg(args, 1);
        if (argument1.isUndefined()) argument1 = JSValue(1.0);
        auto amount = argument1;
        auto argument2 = arg(args, 2);
        if (argument2.isUndefined()) argument2 = JSValue(0.5);
        auto pivot = argument2;
        return invoke(invoke(invoke(module.get("float").call({}, {input}), "sub")({pivot}), "mul")({amount}), "add")({pivot});
    }, {module}, "mx_contrast", 1), callback11Source));
    own(module, "mx_noise_float", withSource(JSValue::function(+[](const JSValue& self, const Args& args, const Args& captures) -> JSValue {
        auto module = captures.at(0);
        auto argument1 = arg(args, 0);
        if (argument1.isUndefined()) argument1 = module.get("uv").call({}, {});
        auto texcoord = argument1;
        auto argument2 = arg(args, 1);
        if (argument2.isUndefined()) argument2 = JSValue(1.0);
        auto amplitude = argument2;
        auto argument3 = arg(args, 2);
        if (argument3.isUndefined()) argument3 = JSValue(0.0);
        auto pivot = argument3;
        return invoke(invoke(module.get("mx_perlin_noise_float").call({}, {invoke(texcoord, "convert")({JSValue("vec2|vec3")})}), "mul")({amplitude}), "add")({pivot});
    }, {module}, "mx_noise_float", 0), callback12Source));
    own(module, "mx_noise_vec3", withSource(JSValue::function(+[](const JSValue& self, const Args& args, const Args& captures) -> JSValue {
        auto module = captures.at(0);
        auto argument1 = arg(args, 0);
        if (argument1.isUndefined()) argument1 = module.get("uv").call({}, {});
        auto texcoord = argument1;
        auto argument2 = arg(args, 1);
        if (argument2.isUndefined()) argument2 = JSValue(1.0);
        auto amplitude = argument2;
        auto argument3 = arg(args, 2);
        if (argument3.isUndefined()) argument3 = JSValue(0.0);
        auto pivot = argument3;
        return invoke(invoke(module.get("mx_perlin_noise_vec3").call({}, {invoke(texcoord, "convert")({JSValue("vec2|vec3")})}), "mul")({amplitude}), "add")({pivot});
    }, {module}, "mx_noise_vec3", 0), callback13Source));
    own(module, "mx_perlin_noise_vec4_0", module.get("Fn").call({}, {withSource(JSValue::function(+[](const JSValue& self, const Args& args, const Args& captures) -> JSValue {
        auto module = captures.at(0);
        auto iterator1 = getJSIterator(arg(args, 0));
        auto p = nextDestructured(iterator1);
        if (!iterator1.done) closeJSIterator(iterator1);
        return module.get("vec4").call({}, {module.get("mx_perlin_noise_vec3").call({}, {p}), module.get("mx_perlin_noise_float").call({}, {invoke(p, "add")({module.get("vec2").call({}, {JSValue(19.0), JSValue(73.0)})})})});
    }, {module}, "", 1), callback14Source), ([&]() { auto object1 = JSValue::object(); own(object1, "p", JSValue("vec2")); own(object1, "return", JSValue("vec4")); return object1; }())}));
    own(module, "mx_perlin_noise_vec4_1", module.get("Fn").call({}, {withSource(JSValue::function(+[](const JSValue& self, const Args& args, const Args& captures) -> JSValue {
        auto module = captures.at(0);
        auto iterator1 = getJSIterator(arg(args, 0));
        auto p = nextDestructured(iterator1);
        if (!iterator1.done) closeJSIterator(iterator1);
        return module.get("vec4").call({}, {module.get("mx_perlin_noise_vec3").call({}, {p}), module.get("mx_perlin_noise_float").call({}, {invoke(p, "add")({module.get("vec3").call({}, {JSValue(19.0), JSValue(73.0), JSValue(29.0)})})})});
    }, {module}, "", 1), callback15Source), ([&]() { auto object1 = JSValue::object(); own(object1, "p", JSValue("vec3")); own(object1, "return", JSValue("vec4")); return object1; }())}));
    own(module, "mx_perlin_noise_vec4", module.get("overloadingFn").call({}, {array({module.get("mx_perlin_noise_vec4_0"), module.get("mx_perlin_noise_vec4_1")})}));
    own(module, "mx_noise_vec4", withSource(JSValue::function(+[](const JSValue& self, const Args& args, const Args& captures) -> JSValue {
        auto module = captures.at(0);
        auto argument1 = arg(args, 0);
        if (argument1.isUndefined()) argument1 = module.get("uv").call({}, {});
        auto texcoord = argument1;
        auto argument2 = arg(args, 1);
        if (argument2.isUndefined()) argument2 = JSValue(1.0);
        auto amplitude = argument2;
        auto argument3 = arg(args, 2);
        if (argument3.isUndefined()) argument3 = JSValue(0.0);
        auto pivot = argument3;
        return invoke(invoke(module.get("mx_perlin_noise_vec4").call({}, {invoke(texcoord, "convert")({JSValue("vec2|vec3")})}), "mul")({amplitude}), "add")({pivot});
    }, {module}, "mx_noise_vec4", 0), callback16Source));
    own(module, "mx_smoothstep", withSource(JSValue::function(+[](const JSValue& self, const Args& args, const Args& captures) -> JSValue {
        auto module = captures.at(0);
        auto inNode = arg(args, 0);
        auto argument1 = arg(args, 1);
        if (argument1.isUndefined()) argument1 = JSValue(0.0);
        auto low = argument1;
        auto argument2 = arg(args, 2);
        if (argument2.isUndefined()) argument2 = JSValue(1.0);
        auto high = argument2;
        auto range = module.get("sub").call({}, {high, low});
        auto safeRange = module.get("max").call({}, {module.get("abs").call({}, {range}), module.get("float").call({}, {JSValue(0.000001)})});
        auto t = module.get("clamp").call({}, {module.get("div").call({}, {module.get("sub").call({}, {inNode, low}), safeRange}), module.get("float").call({}, {JSValue(0.0)}), module.get("float").call({}, {JSValue(1.0)})});
        auto hermite = module.get("mul").call({}, {module.get("mul").call({}, {t, t}), module.get("sub").call({}, {module.get("float").call({}, {JSValue(3.0)}), module.get("mul").call({}, {module.get("float").call({}, {JSValue(2.0)}), t})})});
        auto fallback = module.get("step").call({}, {high, inNode});
        auto useFallback = module.get("step").call({}, {high, low});
        return module.get("mix").call({}, {hermite, fallback, useFallback});
        return JSValue{};
    }, {module}, "mx_smoothstep", 1), callback17Source));
    own(module, "mx_cell_noise_vec3", withSource(JSValue::function(+[](const JSValue& self, const Args& args, const Args& captures) -> JSValue {
        auto module = captures.at(0);
        auto argument1 = arg(args, 0);
        if (argument1.isUndefined()) argument1 = module.get("uv").call({}, {});
        auto texcoord = argument1;
        return module.get("cell_noise_vec3").call({}, {invoke(texcoord, "convert")({JSValue("vec2|vec3")})});
    }, {module}, "mx_cell_noise_vec3", 0), callback18Source));
    own(module, "mx_worley_noise_float_2d", withSource(JSValue::function(+[](const JSValue& self, const Args& args, const Args& captures) -> JSValue {
        auto module = captures.at(0);
        auto argument1 = arg(args, 0);
        if (argument1.isUndefined()) argument1 = module.get("uv").call({}, {});
        auto texcoord = argument1;
        auto argument2 = arg(args, 1);
        if (argument2.isUndefined()) argument2 = JSValue(1.0);
        auto jitter = argument2;
        auto argument3 = arg(args, 2);
        if (argument3.isUndefined()) argument3 = JSValue(0.0);
        auto style = argument3;
        return module.get("worley_noise_float_2d").call({}, {texcoord, jitter, style});
    }, {module}, "mx_worley_noise_float_2d", 0), callback19Source));
    own(module, "mx_worley_noise_float_3d", withSource(JSValue::function(+[](const JSValue& self, const Args& args, const Args& captures) -> JSValue {
        auto module = captures.at(0);
        auto argument1 = arg(args, 0);
        if (argument1.isUndefined()) argument1 = module.get("uv").call({}, {});
        auto texcoord = argument1;
        auto argument2 = arg(args, 1);
        if (argument2.isUndefined()) argument2 = JSValue(1.0);
        auto jitter = argument2;
        auto argument3 = arg(args, 2);
        if (argument3.isUndefined()) argument3 = JSValue(0.0);
        auto style = argument3;
        return module.get("worley_noise_float_3d").call({}, {texcoord, jitter, style});
    }, {module}, "mx_worley_noise_float_3d", 0), callback20Source));
    own(module, "mx_unifiednoise2d", withSource(JSValue::function(+[](const JSValue& self, const Args& args, const Args& captures) -> JSValue {
        auto module = captures.at(0);
        auto noiseType = arg(args, 0);
        auto argument1 = arg(args, 1);
        if (argument1.isUndefined()) argument1 = module.get("uv").call({}, {});
        auto texcoord = argument1;
        auto argument2 = arg(args, 2);
        if (argument2.isUndefined()) argument2 = module.get("vec2").call({}, {JSValue(1.0), JSValue(1.0)});
        auto freq = argument2;
        auto argument3 = arg(args, 3);
        if (argument3.isUndefined()) argument3 = module.get("vec2").call({}, {JSValue(0.0), JSValue(0.0)});
        auto offset = argument3;
        auto argument4 = arg(args, 4);
        if (argument4.isUndefined()) argument4 = JSValue(1.0);
        auto jitter = argument4;
        auto argument5 = arg(args, 5);
        if (argument5.isUndefined()) argument5 = JSValue(0.0);
        auto outmin = argument5;
        auto argument6 = arg(args, 6);
        if (argument6.isUndefined()) argument6 = JSValue(1.0);
        auto outmax = argument6;
        auto argument7 = arg(args, 7);
        if (argument7.isUndefined()) argument7 = JSValue(false);
        auto clampoutput = argument7;
        auto argument8 = arg(args, 8);
        if (argument8.isUndefined()) argument8 = JSValue(1.0);
        auto octaves = argument8;
        auto argument9 = arg(args, 9);
        if (argument9.isUndefined()) argument9 = JSValue(2.0);
        auto lacunarity = argument9;
        auto argument10 = arg(args, 10);
        if (argument10.isUndefined()) argument10 = JSValue(0.5);
        auto diminish = argument10;
        auto argument11 = arg(args, 11);
        if (argument11.isUndefined()) argument11 = JSValue(0.0);
        auto style = argument11;
        return module.get("unifiednoise2d").call({}, {noiseType, texcoord, freq, offset, jitter, outmin, outmax, clampoutput, octaves, lacunarity, diminish, style});
    }, {module}, "mx_unifiednoise2d", 1), callback21Source));
    own(module, "mx_unifiednoise3d", withSource(JSValue::function(+[](const JSValue& self, const Args& args, const Args& captures) -> JSValue {
        auto module = captures.at(0);
        auto noiseType = arg(args, 0);
        auto argument1 = arg(args, 1);
        if (argument1.isUndefined()) argument1 = module.get("uv").call({}, {});
        auto texcoord = argument1;
        auto argument2 = arg(args, 2);
        if (argument2.isUndefined()) argument2 = module.get("vec3").call({}, {JSValue(1.0), JSValue(1.0), JSValue(1.0)});
        auto freq = argument2;
        auto argument3 = arg(args, 3);
        if (argument3.isUndefined()) argument3 = module.get("vec3").call({}, {JSValue(0.0), JSValue(0.0), JSValue(0.0)});
        auto offset = argument3;
        auto argument4 = arg(args, 4);
        if (argument4.isUndefined()) argument4 = JSValue(1.0);
        auto jitter = argument4;
        auto argument5 = arg(args, 5);
        if (argument5.isUndefined()) argument5 = JSValue(0.0);
        auto outmin = argument5;
        auto argument6 = arg(args, 6);
        if (argument6.isUndefined()) argument6 = JSValue(1.0);
        auto outmax = argument6;
        auto argument7 = arg(args, 7);
        if (argument7.isUndefined()) argument7 = JSValue(false);
        auto clampoutput = argument7;
        auto argument8 = arg(args, 8);
        if (argument8.isUndefined()) argument8 = JSValue(1.0);
        auto octaves = argument8;
        auto argument9 = arg(args, 9);
        if (argument9.isUndefined()) argument9 = JSValue(2.0);
        auto lacunarity = argument9;
        auto argument10 = arg(args, 10);
        if (argument10.isUndefined()) argument10 = JSValue(0.5);
        auto diminish = argument10;
        auto argument11 = arg(args, 11);
        if (argument11.isUndefined()) argument11 = JSValue(0.0);
        auto style = argument11;
        return module.get("unifiednoise3d").call({}, {noiseType, texcoord, freq, offset, jitter, outmin, outmax, clampoutput, octaves, lacunarity, diminish, style});
    }, {module}, "mx_unifiednoise3d", 1), callback22Source));
    own(module, "mx_worley_noise_float", withSource(JSValue::function(+[](const JSValue& self, const Args& args, const Args& captures) -> JSValue {
        auto module = captures.at(0);
        auto argument1 = arg(args, 0);
        if (argument1.isUndefined()) argument1 = module.get("uv").call({}, {});
        auto texcoord = argument1;
        auto argument2 = arg(args, 1);
        if (argument2.isUndefined()) argument2 = JSValue(1.0);
        auto jitter = argument2;
        auto argument3 = arg(args, 2);
        if (argument3.isUndefined()) argument3 = JSValue(0.0);
        auto style = argument3;
        return module.get("mx_worley_noise_float_3d").call({}, {invoke(texcoord, "convert")({JSValue("vec2|vec3")}), jitter, style});
    }, {module}, "mx_worley_noise_float", 0), callback23Source));
    own(module, "mx_worley_noise_vec2", withSource(JSValue::function(+[](const JSValue& self, const Args& args, const Args& captures) -> JSValue {
        auto module = captures.at(0);
        auto argument1 = arg(args, 0);
        if (argument1.isUndefined()) argument1 = module.get("uv").call({}, {});
        auto texcoord = argument1;
        auto argument2 = arg(args, 1);
        if (argument2.isUndefined()) argument2 = JSValue(1.0);
        auto jitter = argument2;
        return module.get("worley_noise_vec2").call({}, {invoke(texcoord, "convert")({JSValue("vec2|vec3")}), jitter, module.get("int").call({}, {JSValue(1.0)})});
    }, {module}, "mx_worley_noise_vec2", 0), callback24Source));
    own(module, "mx_worley_noise_vec3", withSource(JSValue::function(+[](const JSValue& self, const Args& args, const Args& captures) -> JSValue {
        auto module = captures.at(0);
        auto argument1 = arg(args, 0);
        if (argument1.isUndefined()) argument1 = module.get("uv").call({}, {});
        auto texcoord = argument1;
        auto argument2 = arg(args, 1);
        if (argument2.isUndefined()) argument2 = JSValue(1.0);
        auto jitter = argument2;
        auto argument3 = arg(args, 2);
        if (argument3.isUndefined()) argument3 = JSValue(1.0);
        auto metric = argument3;
        return module.get("worley_noise_vec3").call({}, {invoke(texcoord, "convert")({JSValue("vec2|vec3")}), jitter, module.get("int").call({}, {metric})});
    }, {module}, "mx_worley_noise_vec3", 0), callback25Source));
    own(module, "mx_worley_noise_vec3_style", withSource(JSValue::function(+[](const JSValue& self, const Args& args, const Args& captures) -> JSValue {
        auto module = captures.at(0);
        auto argument1 = arg(args, 0);
        if (argument1.isUndefined()) argument1 = module.get("uv").call({}, {});
        auto texcoord = argument1;
        auto argument2 = arg(args, 1);
        if (argument2.isUndefined()) argument2 = JSValue(1.0);
        auto jitter = argument2;
        auto argument3 = arg(args, 2);
        if (argument3.isUndefined()) argument3 = JSValue(0.0);
        auto style = argument3;
        auto argument4 = arg(args, 3);
        if (argument4.isUndefined()) argument4 = JSValue(0.0);
        auto metric = argument4;
        return module.get("worley_noise_vec3_style").call({}, {invoke(texcoord, "convert")({JSValue("vec2|vec3")}), jitter, module.get("int").call({}, {style}), module.get("int").call({}, {metric})});
    }, {module}, "mx_worley_noise_vec3_style", 0), callback26Source));
    own(module, "mx_cell_noise_float", withSource(JSValue::function(+[](const JSValue& self, const Args& args, const Args& captures) -> JSValue {
        auto module = captures.at(0);
        auto argument1 = arg(args, 0);
        if (argument1.isUndefined()) argument1 = module.get("uv").call({}, {});
        auto texcoord = argument1;
        return module.get("cell_noise_float").call({}, {invoke(texcoord, "convert")({JSValue("vec2|vec3")})});
    }, {module}, "mx_cell_noise_float", 0), callback27Source));
    own(module, "mx_fractal_noise_float_2d", withSource(JSValue::function(+[](const JSValue& self, const Args& args, const Args& captures) -> JSValue {
        auto module = captures.at(0);
        auto argument1 = arg(args, 0);
        if (argument1.isUndefined()) argument1 = module.get("uv").call({}, {});
        auto texcoord = argument1;
        auto argument2 = arg(args, 1);
        if (argument2.isUndefined()) argument2 = JSValue(3.0);
        auto octaves = argument2;
        auto argument3 = arg(args, 2);
        if (argument3.isUndefined()) argument3 = JSValue(2.0);
        auto lacunarity = argument3;
        auto argument4 = arg(args, 3);
        if (argument4.isUndefined()) argument4 = JSValue(0.5);
        auto diminish = argument4;
        auto argument5 = arg(args, 4);
        if (argument5.isUndefined()) argument5 = JSValue(1.0);
        auto amplitude = argument5;
        return invoke(module.get("fractal_noise_float_2d").call({}, {texcoord, module.get("int").call({}, {octaves}), lacunarity, diminish}), "mul")({amplitude});
    }, {module}, "mx_fractal_noise_float_2d", 0), callback28Source));
    own(module, "mx_fractal_noise_float", withSource(JSValue::function(+[](const JSValue& self, const Args& args, const Args& captures) -> JSValue {
        auto module = captures.at(0);
        auto argument1 = arg(args, 0);
        if (argument1.isUndefined()) argument1 = module.get("uv").call({}, {});
        auto position = argument1;
        auto argument2 = arg(args, 1);
        if (argument2.isUndefined()) argument2 = JSValue(3.0);
        auto octaves = argument2;
        auto argument3 = arg(args, 2);
        if (argument3.isUndefined()) argument3 = JSValue(2.0);
        auto lacunarity = argument3;
        auto argument4 = arg(args, 3);
        if (argument4.isUndefined()) argument4 = JSValue(0.5);
        auto diminish = argument4;
        auto argument5 = arg(args, 4);
        if (argument5.isUndefined()) argument5 = JSValue(1.0);
        auto amplitude = argument5;
        return invoke(module.get("fractal_noise_float").call({}, {position, module.get("int").call({}, {octaves}), lacunarity, diminish}), "mul")({amplitude});
    }, {module}, "mx_fractal_noise_float", 0), callback29Source));
    own(module, "mx_fractal_noise_vec2", withSource(JSValue::function(+[](const JSValue& self, const Args& args, const Args& captures) -> JSValue {
        auto module = captures.at(0);
        auto argument1 = arg(args, 0);
        if (argument1.isUndefined()) argument1 = module.get("uv").call({}, {});
        auto position = argument1;
        auto argument2 = arg(args, 1);
        if (argument2.isUndefined()) argument2 = JSValue(3.0);
        auto octaves = argument2;
        auto argument3 = arg(args, 2);
        if (argument3.isUndefined()) argument3 = JSValue(2.0);
        auto lacunarity = argument3;
        auto argument4 = arg(args, 3);
        if (argument4.isUndefined()) argument4 = JSValue(0.5);
        auto diminish = argument4;
        auto argument5 = arg(args, 4);
        if (argument5.isUndefined()) argument5 = JSValue(1.0);
        auto amplitude = argument5;
        return invoke(module.get("fractal_noise_vec2").call({}, {position, module.get("int").call({}, {octaves}), lacunarity, diminish}), "mul")({amplitude});
    }, {module}, "mx_fractal_noise_vec2", 0), callback30Source));
    own(module, "mx_fractal_noise_vec3", withSource(JSValue::function(+[](const JSValue& self, const Args& args, const Args& captures) -> JSValue {
        auto module = captures.at(0);
        auto argument1 = arg(args, 0);
        if (argument1.isUndefined()) argument1 = module.get("uv").call({}, {});
        auto position = argument1;
        auto argument2 = arg(args, 1);
        if (argument2.isUndefined()) argument2 = JSValue(3.0);
        auto octaves = argument2;
        auto argument3 = arg(args, 2);
        if (argument3.isUndefined()) argument3 = JSValue(2.0);
        auto lacunarity = argument3;
        auto argument4 = arg(args, 3);
        if (argument4.isUndefined()) argument4 = JSValue(0.5);
        auto diminish = argument4;
        auto argument5 = arg(args, 4);
        if (argument5.isUndefined()) argument5 = JSValue(1.0);
        auto amplitude = argument5;
        return invoke(module.get("fractal_noise_vec3").call({}, {position, module.get("int").call({}, {octaves}), lacunarity, diminish}), "mul")({amplitude});
    }, {module}, "mx_fractal_noise_vec3", 0), callback31Source));
    own(module, "mx_fractal_noise_vec4", withSource(JSValue::function(+[](const JSValue& self, const Args& args, const Args& captures) -> JSValue {
        auto module = captures.at(0);
        auto argument1 = arg(args, 0);
        if (argument1.isUndefined()) argument1 = module.get("uv").call({}, {});
        auto position = argument1;
        auto argument2 = arg(args, 1);
        if (argument2.isUndefined()) argument2 = JSValue(3.0);
        auto octaves = argument2;
        auto argument3 = arg(args, 2);
        if (argument3.isUndefined()) argument3 = JSValue(2.0);
        auto lacunarity = argument3;
        auto argument4 = arg(args, 3);
        if (argument4.isUndefined()) argument4 = JSValue(0.5);
        auto diminish = argument4;
        auto argument5 = arg(args, 4);
        if (argument5.isUndefined()) argument5 = JSValue(1.0);
        auto amplitude = argument5;
        return invoke(module.get("fractal_noise_vec4").call({}, {position, module.get("int").call({}, {octaves}), lacunarity, diminish}), "mul")({amplitude});
    }, {module}, "mx_fractal_noise_vec4", 0), callback32Source));
    own(module, "mx_add", withSource(JSValue::function(+[](const JSValue& self, const Args& args, const Args& captures) -> JSValue {
        auto module = captures.at(0);
        auto in1 = arg(args, 0);
        auto argument1 = arg(args, 1);
        if (argument1.isUndefined()) argument1 = module.get("float").call({}, {JSValue(0.0)});
        auto in2 = argument1;
        return module.get("add").call({}, {in1, in2});
    }, {module}, "mx_add", 1), callback33Source));
    own(module, "mx_subtract", withSource(JSValue::function(+[](const JSValue& self, const Args& args, const Args& captures) -> JSValue {
        auto module = captures.at(0);
        auto in1 = arg(args, 0);
        auto argument1 = arg(args, 1);
        if (argument1.isUndefined()) argument1 = module.get("float").call({}, {JSValue(0.0)});
        auto in2 = argument1;
        return module.get("sub").call({}, {in1, in2});
    }, {module}, "mx_subtract", 1), callback34Source));
    own(module, "mx_multiply", withSource(JSValue::function(+[](const JSValue& self, const Args& args, const Args& captures) -> JSValue {
        auto module = captures.at(0);
        auto in1 = arg(args, 0);
        auto argument1 = arg(args, 1);
        if (argument1.isUndefined()) argument1 = module.get("float").call({}, {JSValue(1.0)});
        auto in2 = argument1;
        return module.get("mul").call({}, {in1, in2});
    }, {module}, "mx_multiply", 1), callback35Source));
    own(module, "mx_divide", withSource(JSValue::function(+[](const JSValue& self, const Args& args, const Args& captures) -> JSValue {
        auto module = captures.at(0);
        auto in1 = arg(args, 0);
        auto argument1 = arg(args, 1);
        if (argument1.isUndefined()) argument1 = module.get("float").call({}, {JSValue(1.0)});
        auto in2 = argument1;
        return module.get("div").call({}, {in1, in2});
    }, {module}, "mx_divide", 1), callback36Source));
    own(module, "mx_modulo", withSource(JSValue::function(+[](const JSValue& self, const Args& args, const Args& captures) -> JSValue {
        auto module = captures.at(0);
        auto in1 = arg(args, 0);
        auto argument1 = arg(args, 1);
        if (argument1.isUndefined()) argument1 = module.get("float").call({}, {JSValue(1.0)});
        auto in2 = argument1;
        return module.get("sub").call({}, {in1, module.get("mul").call({}, {in2, module.get("floor").call({}, {module.get("div").call({}, {in1, in2})})})});
    }, {module}, "mx_modulo", 1), callback37Source));
    own(module, "mx_power", withSource(JSValue::function(+[](const JSValue& self, const Args& args, const Args& captures) -> JSValue {
        auto module = captures.at(0);
        auto in1 = arg(args, 0);
        auto argument1 = arg(args, 1);
        if (argument1.isUndefined()) argument1 = module.get("float").call({}, {JSValue(1.0)});
        auto in2 = argument1;
        return module.get("pow").call({}, {in1, in2});
    }, {module}, "mx_power", 1), callback38Source));
    own(module, "mx_atan2", withSource(JSValue::function(+[](const JSValue& self, const Args& args, const Args& captures) -> JSValue {
        auto module = captures.at(0);
        auto argument1 = arg(args, 0);
        if (argument1.isUndefined()) argument1 = module.get("float").call({}, {JSValue(0.0)});
        auto in1 = argument1;
        auto argument2 = arg(args, 1);
        if (argument2.isUndefined()) argument2 = module.get("float").call({}, {JSValue(1.0)});
        auto in2 = argument2;
        return module.get("atan").call({}, {in1, in2});
    }, {module}, "mx_atan2", 0), callback39Source));
    own(module, "mx_timer", withSource(JSValue::function(+[](const JSValue& self, const Args& args, const Args& captures) -> JSValue {
        auto module = captures.at(0);
        return module.get("time");
    }, {module}, "mx_timer", 0), callback40Source));
    own(module, "mx_frame", withSource(JSValue::function(+[](const JSValue& self, const Args& args, const Args& captures) -> JSValue {
        auto module = captures.at(0);
        return module.get("frameId");
    }, {module}, "mx_frame", 0), callback41Source));
    own(module, "mx_invert", withSource(JSValue::function(+[](const JSValue& self, const Args& args, const Args& captures) -> JSValue {
        auto module = captures.at(0);
        auto in1 = arg(args, 0);
        auto argument1 = arg(args, 1);
        if (argument1.isUndefined()) argument1 = module.get("float").call({}, {JSValue(1.0)});
        auto amount = argument1;
        return module.get("sub").call({}, {amount, in1});
    }, {module}, "mx_invert", 1), callback42Source));
    own(module, "mx_ifgreater", withSource(JSValue::function(+[](const JSValue& self, const Args& args, const Args& captures) -> JSValue {
        auto module = captures.at(0);
        auto value1 = arg(args, 0);
        auto value2 = arg(args, 1);
        auto in1 = arg(args, 2);
        auto in2 = arg(args, 3);
        return invoke(invoke(value1, "greaterThan")({value2}), "mix")({in2, in1});
    }, {module}, "mx_ifgreater", 4), callback43Source));
    own(module, "mx_ifgreatereq", withSource(JSValue::function(+[](const JSValue& self, const Args& args, const Args& captures) -> JSValue {
        auto module = captures.at(0);
        auto value1 = arg(args, 0);
        auto value2 = arg(args, 1);
        auto in1 = arg(args, 2);
        auto in2 = arg(args, 3);
        return invoke(invoke(value1, "greaterThanEqual")({value2}), "mix")({in2, in1});
    }, {module}, "mx_ifgreatereq", 4), callback44Source));
    own(module, "mx_ifequal", withSource(JSValue::function(+[](const JSValue& self, const Args& args, const Args& captures) -> JSValue {
        auto module = captures.at(0);
        auto value1 = arg(args, 0);
        auto value2 = arg(args, 1);
        auto in1 = arg(args, 2);
        auto in2 = arg(args, 3);
        return invoke(invoke(value1, "equal")({value2}), "mix")({in2, in1});
    }, {module}, "mx_ifequal", 4), callback45Source));
    own(module, "mx_separate", withSource(JSValue::function(+[](const JSValue& self, const Args& args, const Args& captures) -> JSValue {
        auto module = captures.at(0);
        auto in1 = arg(args, 0);
        auto argument1 = arg(args, 1);
        if (argument1.isUndefined()) argument1 = JSValue(nullptr);
        auto channelOrOut = argument1;
        if (JSValue(typeOf(channelOrOut).strictEquals(JSValue("string"))).toBoolean()) {
            auto map = ([&]() { auto object2 = JSValue::object(); own(object2, "x", JSValue(0.0)); own(object2, "r", JSValue(0.0)); own(object2, "y", JSValue(1.0)); own(object2, "g", JSValue(1.0)); own(object2, "z", JSValue(2.0)); own(object2, "b", JSValue(2.0)); own(object2, "w", JSValue(3.0)); own(object2, "a", JSValue(3.0)); return object2; }());
            auto c = invoke(invoke(channelOrOut, "replace")({createNativeRegExp("^out", ""), JSValue("")}), "toLowerCase")({});
            if (JSValue(!map.get(c).strictEquals(JSValue{})).toBoolean()) {
                return invoke(in1, "element")({map.get(c)});
            }
        }
        if (JSValue(typeOf(channelOrOut).strictEquals(JSValue("number"))).toBoolean()) {
            return invoke(in1, "element")({channelOrOut});
        }
        if (([&]() -> JSValue { const auto left = JSValue(typeOf(channelOrOut).strictEquals(JSValue("string"))); return left.toBoolean() ? JSValue(channelOrOut.get("length").strictEquals(JSValue(1.0))) : left; }()).toBoolean()) {
            auto map = ([&]() { auto object3 = JSValue::object(); own(object3, "x", JSValue(0.0)); own(object3, "r", JSValue(0.0)); own(object3, "y", JSValue(1.0)); own(object3, "g", JSValue(1.0)); own(object3, "z", JSValue(2.0)); own(object3, "b", JSValue(2.0)); own(object3, "w", JSValue(3.0)); own(object3, "a", JSValue(3.0)); return object3; }());
            if (JSValue(!map.get(channelOrOut).strictEquals(JSValue{})).toBoolean()) {
                return invoke(in1, "element")({map.get(channelOrOut)});
            }
        }
        return in1;
        return JSValue{};
    }, {module}, "mx_separate", 1), callback46Source));
    own(module, "mx_place2d", withSource(JSValue::function(+[](const JSValue& self, const Args& args, const Args& captures) -> JSValue {
        auto module = captures.at(0);
        auto texcoord = arg(args, 0);
        auto argument1 = arg(args, 1);
        if (argument1.isUndefined()) argument1 = module.get("vec2").call({}, {JSValue(0.0), JSValue(0.0)});
        auto pivot = argument1;
        auto argument2 = arg(args, 2);
        if (argument2.isUndefined()) argument2 = module.get("vec2").call({}, {JSValue(1.0), JSValue(1.0)});
        auto scale = argument2;
        auto argument3 = arg(args, 3);
        if (argument3.isUndefined()) argument3 = module.get("float").call({}, {JSValue(0.0)});
        auto rotate = argument3;
        auto argument4 = arg(args, 4);
        if (argument4.isUndefined()) argument4 = module.get("vec2").call({}, {JSValue(0.0), JSValue(0.0)});
        auto offset = argument4;
        auto argument5 = arg(args, 5);
        if (argument5.isUndefined()) argument5 = module.get("int").call({}, {JSValue(0.0)});
        auto operationorder = argument5;
        auto centered = module.get("sub").call({}, {texcoord, pivot});
        auto srt = module.get("add").call({}, {module.get("sub").call({}, {module.get("mx_rotate2d").call({}, {module.get("div").call({}, {centered, scale}), rotate}), offset}), pivot});
        auto trs = module.get("add").call({}, {module.get("div").call({}, {module.get("mx_rotate2d").call({}, {module.get("sub").call({}, {centered, offset}), rotate}), scale}), pivot});
        if (JSValue(typeOf(operationorder).strictEquals(JSValue("number"))).toBoolean()) {
            const auto absoluteOrder = invoke(nativeGlobalBinding("Math"), "abs")({operationorder});
            const auto epsilon = nativeGlobalBinding("Number").get("EPSILON");
            return greater(absoluteOrder, epsilon) ? trs : srt;
        }
        return module.get("mix").call({}, {srt, trs, module.get("step").call({}, {JSValue(0.5), module.get("float").call({}, {operationorder})})});
        return JSValue{};
    }, {module}, "mx_place2d", 1), callback47Source));
    own(module, "mx_heighttonormal", withSource(JSValue::function(+[](const JSValue& self, const Args& args, const Args& captures) -> JSValue {
        auto module = captures.at(0);
        auto input = arg(args, 0);
        auto argument1 = arg(args, 1);
        if (argument1.isUndefined()) argument1 = JSValue(1.0);
        auto scale = argument1;
        auto argument2 = arg(args, 2);
        if (argument2.isUndefined()) argument2 = module.get("uv").call({}, {});
        auto texcoord = argument2;
        auto sobelScale = module.get("float").call({}, {JSValue(JSValue(1.0).toNumber() / JSValue(16.0).toNumber())});
        auto height = module.get("float").call({}, {input});
        auto uvNode = module.get("vec2").call({}, {texcoord});
        auto dHdS = invoke(invoke(module.get("vec2").call({}, {invoke(height, "dFdx")({}), invoke(height, "dFdy")({})}), "mul")({module.get("float").call({}, {scale})}), "mul")({sobelScale});
        auto dUdS = module.get("vec2").call({}, {invoke(uvNode.get("x"), "dFdx")({}), invoke(uvNode.get("x"), "dFdy")({})});
        auto dVdS = module.get("vec2").call({}, {invoke(uvNode.get("y"), "dFdx")({}), invoke(uvNode.get("y"), "dFdy")({})});
        auto tangent = module.get("vec3").call({}, {dUdS.get("x"), dVdS.get("x"), dHdS.get("x")});
        auto bitangent = module.get("vec3").call({}, {dUdS.get("y"), dVdS.get("y"), dHdS.get("y")});
        auto n = module.get("cross").call({}, {tangent, bitangent});
        auto invalid = invoke(module.get("dot").call({}, {n, n}), "lessThan")({module.get("float").call({}, {JSValue(1e-12)})});
        n = invoke(invalid, "mix")({n, module.get("vec3").call({}, {JSValue(0.0), JSValue(0.0), JSValue(1.0)})});
        auto mirrored = invoke(n.get("z"), "lessThan")({module.get("float").call({}, {JSValue(0.0)})});
        n = invoke(mirrored, "mix")({n, invoke(n, "mul")({JSValue(-JSValue(1.0).toNumber())})});
        return invoke(invoke(module.get("normalize").call({}, {n}), "mul")({JSValue(0.5)}), "add")({JSValue(0.5)});
        return JSValue{};
    }, {module}, "mx_heighttonormal", 1), callback48Source));
    module.retainForHeapLifetime();lexical()=JSWeakValue(module);auto facade=JSValue::object();
    own(facade, "mx_aastep", module.get("mx_aastep"));
    own(facade, "mx_add", module.get("mx_add"));
    own(facade, "mx_atan2", module.get("mx_atan2"));
    own(facade, "mx_cell_noise_float", module.get("mx_cell_noise_float"));
    own(facade, "mx_cell_noise_vec3", module.get("mx_cell_noise_vec3"));
    own(facade, "mx_contrast", module.get("mx_contrast"));
    own(facade, "mx_divide", module.get("mx_divide"));
    own(facade, "mx_fractal_noise_float", module.get("mx_fractal_noise_float"));
    own(facade, "mx_fractal_noise_float_2d", module.get("mx_fractal_noise_float_2d"));
    own(facade, "mx_fractal_noise_vec2", module.get("mx_fractal_noise_vec2"));
    own(facade, "mx_fractal_noise_vec3", module.get("mx_fractal_noise_vec3"));
    own(facade, "mx_fractal_noise_vec4", module.get("mx_fractal_noise_vec4"));
    own(facade, "mx_frame", module.get("mx_frame"));
    own(facade, "mx_heighttonormal", module.get("mx_heighttonormal"));
    own(facade, "mx_hsvtorgb", module.get("mx_hsvtorgb"));
    own(facade, "mx_ifequal", module.get("mx_ifequal"));
    own(facade, "mx_ifgreater", module.get("mx_ifgreater"));
    own(facade, "mx_ifgreatereq", module.get("mx_ifgreatereq"));
    own(facade, "mx_invert", module.get("mx_invert"));
    own(facade, "mx_modulo", module.get("mx_modulo"));
    own(facade, "mx_multiply", module.get("mx_multiply"));
    own(facade, "mx_noise_float", module.get("mx_noise_float"));
    own(facade, "mx_noise_vec3", module.get("mx_noise_vec3"));
    own(facade, "mx_noise_vec4", module.get("mx_noise_vec4"));
    own(facade, "mx_place2d", module.get("mx_place2d"));
    own(facade, "mx_power", module.get("mx_power"));
    own(facade, "mx_ramp4", module.get("mx_ramp4"));
    own(facade, "mx_ramplr", module.get("mx_ramplr"));
    own(facade, "mx_ramptb", module.get("mx_ramptb"));
    own(facade, "mx_rgbtohsv", module.get("mx_rgbtohsv"));
    own(facade, "mx_rotate2d", module.get("mx_rotate2d"));
    own(facade, "mx_rotate3d", module.get("mx_rotate3d"));
    own(facade, "mx_safepower", module.get("mx_safepower"));
    own(facade, "mx_separate", module.get("mx_separate"));
    own(facade, "mx_smoothstep", module.get("mx_smoothstep"));
    own(facade, "mx_splitlr", module.get("mx_splitlr"));
    own(facade, "mx_splittb", module.get("mx_splittb"));
    own(facade, "mx_srgb_texture_to_lin_rec709", module.get("mx_srgb_texture_to_lin_rec709"));
    own(facade, "mx_subtract", module.get("mx_subtract"));
    own(facade, "mx_timer", module.get("mx_timer"));
    own(facade, "mx_transform_uv", module.get("mx_transform_uv"));
    own(facade, "mx_unifiednoise2d", module.get("mx_unifiednoise2d"));
    own(facade, "mx_unifiednoise3d", module.get("mx_unifiednoise3d"));
    own(facade, "mx_worley_noise_float", module.get("mx_worley_noise_float"));
    own(facade, "mx_worley_noise_float_2d", module.get("mx_worley_noise_float_2d"));
    own(facade, "mx_worley_noise_float_3d", module.get("mx_worley_noise_float_3d"));
    own(facade, "mx_worley_noise_vec2", module.get("mx_worley_noise_vec2"));
    own(facade, "mx_worley_noise_vec3", module.get("mx_worley_noise_vec3"));
    own(facade, "mx_worley_noise_vec3_style", module.get("mx_worley_noise_vec3_style"));
    facade.retainForHeapLifetime();weak=JSWeakValue(facade);return facade;
}
JSValue nativeMaterialXNodesBinding(const std::string& name){nativeMaterialXNodesModule();return lexical().lock()->get(name);}
JSValue nativeMaterialXNodesPrivate(const std::string& name){return nativeMaterialXNodesBinding(name);}
