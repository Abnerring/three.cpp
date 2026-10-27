#include "animation/AnimationUtils.h"
#include "core/JSIterator.h"
#include "core/JSProxy.h"
#include "nodes/core/Node.h"
#include "nodes/core/NodeBuilder.h"
#include "nodes/tsl/TSLCore.h"
#include "utils.h"
#include <iostream>
#include <cmath>

namespace {
using Args = std::vector<JSValue>;
JSValue arg(const Args& args, std::size_t i) { return i < args.size() ? args[i] : JSValue{}; }
JSValue call(const JSValue& self, const char* name, const Args& args = {}) { return self.get(name).call(self, args); }
JSValue array(const Args& values = {}) { auto out = JSValue::array(); for (const auto& value : values) call(out, "push", {value}); return out; }
JSValue normal(const JSValue& value, const JSValue& state, unsigned depth = 0) {
    if (value.isUndefined()) return "#undefined";
    if (value.isCallable()) return "#function";
    if (value.isSymbol()) return "#symbol";
    if (value.type() == JSValue::Type::Number) {
        if (std::isnan(value.number())) return "#NaN";
        if (std::isinf(value.number())) return value.number() > 0 ? "#Infinity" : "#-Infinity";
        if (value.number() == 0 && std::signbit(value.number())) return "#-0";
    }
    if (!value.isObject()) return value;
    for (const auto* name : {"builder", "output", "shader", "call", "function", "stack"})
        if (value.sameValue(state.get(name))) return std::string("#") + name;
    if (depth > 5) return "#depth";
    auto out = value.type() == JSValue::Type::Array ? JSValue::array(value.length()) : JSValue::object();
    if (value.get("isNode").strictEquals(true)) {
        out.set("kind", value.get("constructor").get("name"));
        for (const auto* name : {"nodeType", "value", "rawInputs", "isFn", "isShaderCallNodeInternal", "layout", "once", "subBuilds"})
            if (value.hasOwn(name)) out.set(name, normal(value.get(name), state, depth + 1));
        return out;
    }
    for (const auto& key : value.keys()) out.set(key, normal(value.get(key), state, depth + 1));
    return out;
}
void event(const JSValue& state, const Args& values) {
    auto row = JSValue::array(); for (const auto& value : values) call(row, "push", {normal(value, state)}); call(state.get("events"), "push", {row});
}
JSValue decode(const JSValue& spec, const JSValue& state) {
    if (!spec.isObject()) return spec;
    if (spec.type() == JSValue::Type::Array) { auto out = JSValue::array(); for (unsigned i=0;i<spec.length();++i) call(out,"push",{decode(spec.get(std::to_string(i)),state)}); return out; }
    if (spec.get("tag").strictEquals("undefined")) return {};
    if (spec.get("tag").strictEquals("node")) {
        auto node = nativeNodeConstructor().construct({spec.get("type")}); node.set("fixture", spec.get("name"));
        node.set("build", JSValue::function([](const JSValue& self, const Args& args, const Args& c)->JSValue { event(c[0],{"inputBuild",self.get("fixture"),array(args)});return self.get("fixture"); }, {state}));
        return node;
    }
    if(spec.get("tag").strictEquals("getters")) {
        auto object=JSValue::object();const auto fields=spec.get("fields");
        for(const auto& key:fields.keys()) {
            JSPropertyDescriptor d;d.getter=JSValue::function([](const JSValue& self,const Args&,const Args& c)->JSValue {
                event(c[0],{"getter",c[1],self.sameValue(c[2])});
                if(c[4].get("throwKey").sameValue(c[1]))throw JSTypeError("getter");return c[3];
            },{state,key,object,decode(fields.get(key),state),spec});d.enumerable=d.configurable=true;object.defineProperty(key,d);
        }
        return object;
    }
    auto out=JSValue::object(); for(const auto& key:spec.keys())out.set(key,decode(spec.get(key),state));return out;
}
void getter(JSValue object,const char* key,JSValue value) { JSPropertyDescriptor d;d.getter=value;d.configurable=d.enumerable=true;object.defineProperty(key,d); }
JSValue run(const JSValue& request) {
    auto state=JSValue::object();state.set("events",array());auto out=JSValue::object();const auto module=nativeTSLCore();module.get("setCurrentStack").call({}, {nullptr});
    ThreeUtils::setConsoleFunction(JSValue::function([](const JSValue&,const Args& args,const Args& c)->JSValue{event(c[0],{"console",arg(args,0)});return {};},{state}));
    const auto kind=request.get("kind");
    try {
        auto jsFunc=JSValue::function([](const JSValue&,const Args& args,const Args& c)->JSValue {
            const auto state=c[0],request=c[1];event(state,{"body",double(args.size())});
            const auto inputs=arg(args,0),builder=arg(args,args.size()>1?1:0);
            const auto keys=request.get("reads");if(keys.type()==JSValue::Type::Array)for(unsigned i=0;i<keys.length();++i){const auto key=keys.get(std::to_string(i));event(state,{"read",key,inputs.get(key)});}
            if(request.get("spread").toBoolean()) { auto iterator=getJSIterator(builder);auto values=array();while(auto value=stepJSIterator(iterator))call(values,"push",{*value});event(state,{"builderIterable",values}); }
            if(request.get("generator").toBoolean()) {
                const auto iterable=request.get("builderGenerator").toBoolean()?builder:inputs;
                const auto function=iterable.get(JSValue::wellKnownSymbol("iterator"));const auto iterator=function.call(iterable);
                event(state,{"generatorMeta",function.get("name"),function.get("length"),array()});
                auto keys=array();for(const auto& key:iterator.ownPropertyNames())call(keys,"push",{key});event(state,{"generatorKeys",keys});
                const auto next=iterator.get("next");try{next.call(JSValue::object());event(state,{"borrowed","ok"});}catch(const JSTypeError&){event(state,{"borrowed","TypeError"});}
                event(state,{"next",next.call(iterator)});event(state,{"return",call(iterator,"return",{23})});event(state,{"after",next.call(iterator)});
            }
            if(request.get("throwBody").toBoolean())throw JSTypeError("body");
            return state.get("output");
        },{state,request},"body",request.hasOwn("arity")?static_cast<unsigned>(request.get("arity").toNumber()):0);
        auto output=nativeNodeConstructor().construct({"float"});state.set("output",output);
        const auto layout=request.hasOwn("layout")?decode(request.get("layout"),state):JSValue(nullptr);
        if(kind.strictEquals("exportConstructor")) {
            auto fn=module.get(request.get("name"));const auto original=fn.get("prototype");
            const auto descriptor=fn.getOwnPropertyDescriptor("prototype");auto flags=JSValue::object();
            flags.set("writable",descriptor->writable.value_or(false));flags.set("enumerable",descriptor->enumerable.value_or(false));flags.set("configurable",descriptor->configurable.value_or(false));out.set("descriptor",flags);
            out.set("originalConstructor",original.get("constructor").sameValue(fn));
            auto replacement=request.get("mode").strictEquals("null")?JSValue(nullptr):JSValue::object();if(replacement.isObject())replacement.set("fixture","replacement");
            try {
                fn.set("prototype",replacement);out.set("replaced",fn.get("prototype").sameValue(replacement));auto newTarget=fn;
                if(request.get("mode").strictEquals("newTarget")) {
                    const auto target=JSValue::constructor([](const JSValue&,const Args&,const Args&)->JSValue{return {};},{},"",0,JSValue::ConstructorKind::Callable);auto handler=JSValue::object();
                    handler.set("get",JSValue::function([](const JSValue&,const Args& args,const Args& c)->JSValue {if(arg(args,1).strictEquals("prototype")){event(c[0],{"newTarget.prototype"});return c[1];}return arg(args,0).get(arg(args,1),arg(args,2));},{state,replacement},"get",3));newTarget=createNativeProxy(target,handler);
                }
                const auto value=fn.construct(request.get("name").strictEquals("Stack")?Args{7}:Args{jsFunc},newTarget);
                out.set("callable",value.isCallable());out.set("constructed",value.isObject());out.set("usesReplacement",value.getPrototype().sameValue(replacement));out.set("usesObjectPrototype",value.getPrototype().sameValue(JSValue::intrinsicObjectConstructor().get("prototype")));out.set("isNode",value.get("isNode").strictEquals(true));
            } catch(...) {fn.set("prototype",original);throw;}
            fn.set("prototype",original);
        } else if(kind.strictEquals("metadata")||kind.strictEquals("fn")) {
            const auto fn=module.get("Fn").call({}, {jsFunc,layout});state.set("function",fn);
            auto shader=fn.get("shaderNode");state.set("shader",shader);
            out.set("callable",fn.isCallable());out.set("isNode",fn.get("isNode"));out.set("isFn",fn.get("isFn"));
            out.set("keys",array());for(const auto& key:fn.ownPropertyNames())call(out.get("keys"),"push",{key});
            out.set("nodeType",normal(shader.get("nodeType"),state));out.set("layout",normal(shader.get("layout"),state));
            out.set("cacheable",call(fn,"isCacheable"));out.set("shaderCacheable",call(shader,"isCacheable"));
            if(request.hasOwn("newLayout")) { out.set("setLayoutSelf",call(fn,"setLayout",{decode(request.get("newLayout"),state)}).sameValue(fn));out.set("newLayout",normal(shader.get("layout"),state)); }
            if(request.get("once").toBoolean()) { out.set("onceSelf",call(fn,"once",{decode(request.get("subBuilds"),state)}).sameValue(fn));out.set("once",shader.get("once"));out.set("subBuilds",normal(shader.get("subBuilds"),state)); }
            if(kind.strictEquals("fn")) { Args args;const auto values=request.get("params");for(unsigned i=0;i<values.length();++i)args.push_back(decode(values.get(std::to_string(i)),state));const auto node=fn.call({},args);state.set("call",node);out.set("rawInputs",normal(node.get("rawInputs"),state));out.set("sameShader",node.get("shaderNode").sameValue(shader));out.set("brand",node.get("isShaderCallNodeInternal")); }
        } else if(kind.strictEquals("shader")||kind.strictEquals("call")||kind.strictEquals("layoutCall")) {
            auto shader=module.get("ShaderNode").call({}, {jsFunc,request.hasOwn("type")?decode(request.get("type"),state):JSValue{}});state.set("shader",shader);
            out.set("nodeType",normal(shader.get("nodeType"),state));out.set("global",shader.get("global"));out.set("once",shader.get("once"));out.set("layout",shader.get("layout"));
            const auto raw=request.hasOwn("params")?decode(request.get("params"),state):JSValue(nullptr);
            const auto node=call(shader,"call",{raw});state.set("call",node);
            out.set("rawSame",node.get("rawInputs").sameValue(raw));out.set("sameShader",node.get("shaderNode").sameValue(shader));
            if(!kind.strictEquals("shader")) {
                auto properties=JSValue::object(),builder=JSValue::object();state.set("builder",builder);builder.set("subBuildFn","oldSub");builder.set("fnCall","oldCall");builder.set("fixture",7);
                builder.set("getNodeProperties",JSValue::function([](const JSValue&,const Args& args,const Args& c)->JSValue{event(c[0],{"getNodeProperties",arg(args,0)});return c[1];},{state,properties}));
                builder.set("getClosestSubBuild",JSValue::function([](const JSValue&,const Args& args,const Args& c)->JSValue{event(c[0],{"getClosestSubBuild",arg(args,0)});return c[1].get("subBuild");},{state,request}));
                if(request.hasOwn("cached"))properties.set(request.get("subBuild").toBoolean()?request.get("subBuild"):JSValue("default"),decode(request.get("cached"),state));
                if(request.get("once").toBoolean())shader.set("once",true);
                if(kind.strictEquals("layoutCall")) {
                    call(shader,"setLayout",{layout});
                    auto func=JSValue::object();func.set("call",JSValue::function([](const JSValue&,const Args& args,const Args& c)->JSValue{event(c[0],{"functionCall",array(args)});return c[0].get("output");},{state}));state.set("function",func);
                    builder.set("buildFunctionNode",JSValue::function([](const JSValue&,const Args& args,const Args& c)->JSValue{event(c[0],{"buildFunctionNode",arg(args,0)});return c[0].get("function");},{state}));
                    builder.set("addInclude",JSValue::function([](const JSValue&,const Args& args,const Args& c)->JSValue{event(c[0],{"addInclude",arg(args,0)});return {};},{state}));
                }
                auto results=array();out.set("results",results);for(int i=0;i<(request.hasOwn("calls")?request.get("calls").toNumber():1);++i)call(results,"push",{normal(call(node,"call",{builder}),state)});
                out.set("subBuildFn",builder.get("subBuildFn"));out.set("fnCall",normal(builder.get("fnCall"),state));out.set("properties",normal(properties,state));
            }
        } else if(kind.strictEquals("integration")) {
            state.set("output",module.get("float").call({}, {7}));
            auto renderer=JSValue::object(),debug=JSValue::object(),diagnostics=JSValue::object();diagnostics.set("keywords",false);debug.set("diagnostics",diagnostics);renderer.set("debug",debug);
            auto builder=NodeBuilder(nullptr,renderer,nullptr).object();builder.set("shaderStage","fragment");state.set("builder",builder);
            const auto fn=module.get("Fn").call({}, {jsFunc,layout});if(request.get("once").toBoolean())call(fn,"once",{request.get("subBuilds")});
            const auto node=fn.call();state.set("call",node);auto values=array();
            for(const auto* stage:{"setup","analyze","generate"}){builder.set("buildStage",stage);call(values,"push",{normal(call(node,"build",{builder,"float"}),state)});}
            out.set("values",values);out.set("flow",normal(builder.get("flow"),state));out.set("stackRestored",module.get("getCurrentStack").call().isNull());
        } else if(kind.strictEquals("stack")) {
            auto stack=JSValue::object();state.set("stack",stack);for(const auto* key:{"addToStack","If","Switch"})stack.set(key,JSValue::function([](const JSValue& self,const Args& args,const Args& c)->JSValue{event(c[0],{c[1],self,array(args)});return c[2];},{state,key,"result"}));
            module.get("setCurrentStack").call({}, {request.get("nullStack").toBoolean()?JSValue(nullptr):stack});out.set("sameStack",module.get("getCurrentStack").call().sameValue(stack));
            for(const auto* key:{"Stack","If","Switch"}) {const auto val=module.get(key).call({}, {output,7});out.set(key,normal(val,state));}
        }
    }catch(const JSTypeError& error){std::cerr<<"TSL shader fixture "<<kind.toString()<<": "<<error.what()<<'\n';out.set("error","TypeError");}catch(const JSRangeError&){out.set("error","RangeError");}catch(const std::exception& error){out.set("error","Error");out.set("nativeError",error.what());}
    if(out.hasOwn("error")&&state.get("builder").isObject()){out.set("partialSubBuildFn",normal(state.get("builder").get("subBuildFn"),state));out.set("partialFnCall",normal(state.get("builder").get("fnCall"),state));}
    out.set("events",state.get("events"));module.get("setCurrentStack").call({}, {nullptr});ThreeUtils::setConsoleFunction({});return out;
}
}
int main(int argc,char** argv){try{auto realm=JSValue::object();if(argc>1&&std::string(argv[1])=="--eager-builder-import")nativeNodeBuilderConstructor();const std::string input(std::istreambuf_iterator<char>(std::cin),{});const auto requests=Animation_detail::parseJSON(input);auto out=array();for(unsigned i=0;i<requests.length();++i)call(out,"push",{run(requests.get(std::to_string(i)))});std::cout<<out.toJSON()->dump();return 0;}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}}
