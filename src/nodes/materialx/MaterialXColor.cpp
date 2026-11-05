// Complete native TSL port of pinned MaterialXColor.js (11738a). No script evaluator.
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
#include "MaterialXColorSources.inc"
JSWeakValue& lexical(){static thread_local JSWeakValue value;return value;}
JSValue withSource(JSValue value,const char* source){value.setFunctionSourceText(source);return value;}
JSValue nextDestructured(JSIteratorRecord& iterator){if(iterator.done)return {};const auto value=stepJSIterator(iterator);return value?*value:JSValue{};}
JSValue array(const Args& values){auto out=JSValue::array();for(std::size_t i=0;i<values.size();++i)own(out,std::to_string(i).c_str(),values[i]);return out;}
JSValue typeOf(const JSValue& value){if(value.isUndefined())return "undefined";if(value.isCallable())return "function";if(value.isSymbol())return "symbol";switch(value.type()){case JSValue::Type::Boolean:return "boolean";case JSValue::Type::Number:return "number";case JSValue::Type::String:return "string";default:return "object";}}
JSValue leftShift(const JSValue& a,const JSValue& b){const auto x=a.toNumber();const auto y=b.toNumber();return static_cast<std::int32_t>(static_cast<std::uint32_t>(x) << (static_cast<std::uint32_t>(y)&31));}
}

JSValue nativeMaterialXColorModule(){
    static thread_local JSWeakValue weak;if(const auto cached=weak.lock();cached&&cached->isObject())return *cached;
    static thread_local bool initializing=false;JSModuleInitializationScope scope(initializing,"MaterialXColor module");
    auto module=JSValue::object(nullptr);
    own(module, "int", nativeTSLBaseBinding("int"));
    own(module, "float", nativeTSLBaseBinding("float"));
    own(module, "vec3", nativeTSLBaseBinding("vec3"));
    own(module, "If", nativeTSLBaseBinding("If"));
    own(module, "Fn", nativeTSLBaseBinding("Fn"));
    own(module, "add", nativeOperatorNodeBinding("add"));
    own(module, "floor", nativeMathNodeBinding("floor"));
    own(module, "trunc", nativeMathNodeBinding("trunc"));
    own(module, "max", nativeMathNodeBinding("max"));
    own(module, "min", nativeMathNodeBinding("min"));
    own(module, "mx_hsvtorgb", module.get("Fn").call({}, {withSource(JSValue::function(+[](const JSValue& self, const Args& args, const Args& captures) -> JSValue {
        auto module = captures.at(0);
        auto iterator1 = getJSIterator(arg(args, 0));
        auto hsv = nextDestructured(iterator1);
        if (!iterator1.done) closeJSIterator(iterator1);
        auto s = hsv.get("y");
        auto v = hsv.get("z");
        auto result = invoke(module.get("vec3").call({}, {}), "toVar")({});
        invoke(module.get("If").call({}, {invoke(s, "lessThan")({JSValue(0.0001)}), withSource(JSValue::function(+[](const JSValue& self, const Args& args, const Args& captures) -> JSValue {
            auto module = captures.at(0);
            auto v = captures.at(1);
            auto result = captures.at(2);
            invoke(result, "assign")({module.get("vec3").call({}, {v, v, v})});
            return JSValue{};
        }, {module, v, result}, "", 0), callback2Source)}), "Else")({withSource(JSValue::function(+[](const JSValue& self, const Args& args, const Args& captures) -> JSValue {
            auto module = captures.at(0);
            auto hsv = captures.at(1);
            auto s = captures.at(2);
            auto v = captures.at(3);
            auto result = captures.at(4);
            auto h = hsv.get("x");
            h = invoke(invoke(invoke(h, "sub")({module.get("floor").call({}, {h})}), "mul")({JSValue(6.0)}), "toVar")({});
            auto hi = module.get("int").call({}, {module.get("trunc").call({}, {h})});
            auto f = invoke(h, "sub")({module.get("float").call({}, {hi})});
            auto p = invoke(v, "mul")({invoke(s, "oneMinus")({})});
            auto q = invoke(v, "mul")({invoke(invoke(s, "mul")({f}), "oneMinus")({})});
            auto t = invoke(v, "mul")({invoke(invoke(s, "mul")({invoke(f, "oneMinus")({})}), "oneMinus")({})});
            invoke(invoke(invoke(invoke(invoke(module.get("If").call({}, {invoke(hi, "equal")({module.get("int").call({}, {JSValue(0.0)})}), withSource(JSValue::function(+[](const JSValue& self, const Args& args, const Args& captures) -> JSValue {
                auto module = captures.at(0);
                auto v = captures.at(1);
                auto result = captures.at(2);
                auto p = captures.at(3);
                auto t = captures.at(4);
                invoke(result, "assign")({module.get("vec3").call({}, {v, t, p})});
                return JSValue{};
            }, {module, v, result, p, t}, "", 0), callback4Source)}), "ElseIf")({invoke(hi, "equal")({module.get("int").call({}, {JSValue(1.0)})}), withSource(JSValue::function(+[](const JSValue& self, const Args& args, const Args& captures) -> JSValue {
                auto module = captures.at(0);
                auto v = captures.at(1);
                auto result = captures.at(2);
                auto p = captures.at(3);
                auto q = captures.at(4);
                invoke(result, "assign")({module.get("vec3").call({}, {q, v, p})});
                return JSValue{};
            }, {module, v, result, p, q}, "", 0), callback5Source)}), "ElseIf")({invoke(hi, "equal")({module.get("int").call({}, {JSValue(2.0)})}), withSource(JSValue::function(+[](const JSValue& self, const Args& args, const Args& captures) -> JSValue {
                auto module = captures.at(0);
                auto v = captures.at(1);
                auto result = captures.at(2);
                auto p = captures.at(3);
                auto t = captures.at(4);
                invoke(result, "assign")({module.get("vec3").call({}, {p, v, t})});
                return JSValue{};
            }, {module, v, result, p, t}, "", 0), callback6Source)}), "ElseIf")({invoke(hi, "equal")({module.get("int").call({}, {JSValue(3.0)})}), withSource(JSValue::function(+[](const JSValue& self, const Args& args, const Args& captures) -> JSValue {
                auto module = captures.at(0);
                auto v = captures.at(1);
                auto result = captures.at(2);
                auto p = captures.at(3);
                auto q = captures.at(4);
                invoke(result, "assign")({module.get("vec3").call({}, {p, q, v})});
                return JSValue{};
            }, {module, v, result, p, q}, "", 0), callback7Source)}), "ElseIf")({invoke(hi, "equal")({module.get("int").call({}, {JSValue(4.0)})}), withSource(JSValue::function(+[](const JSValue& self, const Args& args, const Args& captures) -> JSValue {
                auto module = captures.at(0);
                auto v = captures.at(1);
                auto result = captures.at(2);
                auto p = captures.at(3);
                auto t = captures.at(4);
                invoke(result, "assign")({module.get("vec3").call({}, {t, p, v})});
                return JSValue{};
            }, {module, v, result, p, t}, "", 0), callback8Source)}), "Else")({withSource(JSValue::function(+[](const JSValue& self, const Args& args, const Args& captures) -> JSValue {
                auto module = captures.at(0);
                auto v = captures.at(1);
                auto result = captures.at(2);
                auto p = captures.at(3);
                auto q = captures.at(4);
                invoke(result, "assign")({module.get("vec3").call({}, {v, p, q})});
                return JSValue{};
            }, {module, v, result, p, q}, "", 0), callback9Source)});
            return JSValue{};
        }, {module, hsv, s, v, result}, "", 0), callback3Source)});
        return result;
        return JSValue{};
    }, {module}, "", 1), callback1Source), ([&]() { auto object1 = JSValue::object(); own(object1, "hsv", JSValue("vec3")); own(object1, "return", JSValue("vec3")); return object1; }())}));
    own(module, "mx_rgbtohsv", module.get("Fn").call({}, {withSource(JSValue::function(+[](const JSValue& self, const Args& args, const Args& captures) -> JSValue {
        auto module = captures.at(0);
        auto iterator1 = getJSIterator(arg(args, 0));
        auto c_immutable = nextDestructured(iterator1);
        if (!iterator1.done) closeJSIterator(iterator1);
        auto c = invoke(module.get("vec3").call({}, {c_immutable}), "toVar")({});
        auto r = invoke(module.get("float").call({}, {c.get("x")}), "toVar")({});
        auto g = invoke(module.get("float").call({}, {c.get("y")}), "toVar")({});
        auto b = invoke(module.get("float").call({}, {c.get("z")}), "toVar")({});
        auto mincomp = invoke(module.get("float").call({}, {module.get("min").call({}, {r, module.get("min").call({}, {g, b})})}), "toVar")({});
        auto maxcomp = invoke(module.get("float").call({}, {module.get("max").call({}, {r, module.get("max").call({}, {g, b})})}), "toVar")({});
        auto delta = invoke(module.get("float").call({}, {invoke(maxcomp, "sub")({mincomp})}), "toVar")({});
        auto h = invoke(module.get("float").call({}, {}), "toVar")({});
        auto s = invoke(module.get("float").call({}, {}), "toVar")({});
        auto v = invoke(module.get("float").call({}, {}), "toVar")({});
        invoke(v, "assign")({maxcomp});
        invoke(module.get("If").call({}, {invoke(maxcomp, "greaterThan")({JSValue(0.0)}), withSource(JSValue::function(+[](const JSValue& self, const Args& args, const Args& captures) -> JSValue {
            auto module = captures.at(0);
            auto maxcomp = captures.at(1);
            auto delta = captures.at(2);
            auto s = captures.at(3);
            invoke(s, "assign")({invoke(delta, "div")({maxcomp})});
            return JSValue{};
        }, {module, maxcomp, delta, s}, "", 0), callback11Source)}), "Else")({withSource(JSValue::function(+[](const JSValue& self, const Args& args, const Args& captures) -> JSValue {
            auto module = captures.at(0);
            auto s = captures.at(1);
            invoke(s, "assign")({JSValue(0.0)});
            return JSValue{};
        }, {module, s}, "", 0), callback12Source)});
        invoke(module.get("If").call({}, {invoke(s, "lessThanEqual")({JSValue(0.0)}), withSource(JSValue::function(+[](const JSValue& self, const Args& args, const Args& captures) -> JSValue {
            auto module = captures.at(0);
            auto h = captures.at(1);
            invoke(h, "assign")({JSValue(0.0)});
            return JSValue{};
        }, {module, h}, "", 0), callback13Source)}), "Else")({withSource(JSValue::function(+[](const JSValue& self, const Args& args, const Args& captures) -> JSValue {
            auto module = captures.at(0);
            auto r = captures.at(1);
            auto g = captures.at(2);
            auto b = captures.at(3);
            auto maxcomp = captures.at(4);
            auto delta = captures.at(5);
            auto h = captures.at(6);
            invoke(invoke(module.get("If").call({}, {invoke(r, "greaterThanEqual")({maxcomp}), withSource(JSValue::function(+[](const JSValue& self, const Args& args, const Args& captures) -> JSValue {
                auto module = captures.at(0);
                auto g = captures.at(1);
                auto b = captures.at(2);
                auto delta = captures.at(3);
                auto h = captures.at(4);
                invoke(h, "assign")({invoke(invoke(g, "sub")({b}), "div")({delta})});
                return JSValue{};
            }, {module, g, b, delta, h}, "", 0), callback15Source)}), "ElseIf")({invoke(g, "greaterThanEqual")({maxcomp}), withSource(JSValue::function(+[](const JSValue& self, const Args& args, const Args& captures) -> JSValue {
                auto module = captures.at(0);
                auto r = captures.at(1);
                auto b = captures.at(2);
                auto delta = captures.at(3);
                auto h = captures.at(4);
                invoke(h, "assign")({module.get("add").call({}, {JSValue(2.0), invoke(invoke(b, "sub")({r}), "div")({delta})})});
                return JSValue{};
            }, {module, r, b, delta, h}, "", 0), callback16Source)}), "Else")({withSource(JSValue::function(+[](const JSValue& self, const Args& args, const Args& captures) -> JSValue {
                auto module = captures.at(0);
                auto r = captures.at(1);
                auto g = captures.at(2);
                auto delta = captures.at(3);
                auto h = captures.at(4);
                invoke(h, "assign")({module.get("add").call({}, {JSValue(4.0), invoke(invoke(r, "sub")({g}), "div")({delta})})});
                return JSValue{};
            }, {module, r, g, delta, h}, "", 0), callback17Source)});
            invoke(h, "mulAssign")({JSValue(JSValue(1.0).toNumber() / JSValue(6.0).toNumber())});
            module.get("If").call({}, {invoke(h, "lessThan")({JSValue(0.0)}), withSource(JSValue::function(+[](const JSValue& self, const Args& args, const Args& captures) -> JSValue {
                auto module = captures.at(0);
                auto h = captures.at(1);
                invoke(h, "addAssign")({JSValue(1.0)});
                return JSValue{};
            }, {module, h}, "", 0), callback18Source)});
            return JSValue{};
        }, {module, r, g, b, maxcomp, delta, h}, "", 0), callback14Source)});
        return module.get("vec3").call({}, {h, s, v});
        return JSValue{};
    }, {module}, "", 1), callback10Source), ([&]() { auto object1 = JSValue::object(); own(object1, "c", JSValue("vec3")); own(object1, "return", JSValue("vec3")); return object1; }())}));
    module.retainForHeapLifetime();lexical()=JSWeakValue(module);auto facade=JSValue::object();
    own(facade, "mx_hsvtorgb", module.get("mx_hsvtorgb"));
    own(facade, "mx_rgbtohsv", module.get("mx_rgbtohsv"));
    facade.retainForHeapLifetime();weak=JSWeakValue(facade);return facade;
}
JSValue nativeMaterialXColorBinding(const std::string& name){nativeMaterialXColorModule();return lexical().lock()->get(name);}
JSValue nativeMaterialXColorPrivate(const std::string& name){return nativeMaterialXColorBinding(name);}
