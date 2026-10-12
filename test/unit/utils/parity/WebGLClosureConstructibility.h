#pragma once
#include "renderers/webgl/WebGLInfo.h"
#include "renderers/webgl/WebGLBufferRenderer.h"
#include "renderers/webgl/WebGLIndexedBufferRenderer.h"
#include "renderers/webgl/WebGLProperties.h"
#include "core/JSWeakMap.h"

// Shared fixtures exercise the declarations returned by the four fixed upstream
// modules. Native adapter callbacks only record calls; they do not bind a GPU.
namespace WebGLClosureParity {
using Args = std::vector<JSValue>;
using A = JsonValue::Array;
using O = JsonValue::Object;
inline void push(JSValue trace, const Args& args) {
    auto entry = JSValue::array(); entry.get("push").call(entry, args);
    trace.get("push").call(trace, {entry});
}
inline JsonValue encode(const JSValue& v) {
    if(v.isUndefined()) return "undefined";
    if(v.isCallable()) return "function";
    if(v.isNull()) return nullptr;
    if(v.type()==JSValue::Type::Boolean) return v.toBoolean();
    if(v.type()==JSValue::Type::Number) {
        const auto n=v.number(); if(std::isnan(n))return "NaN";
        if(!std::isfinite(n))return n<0?"-Infinity":"Infinity";
        if(n==0 && std::signbit(n))return "-0"; return n;
    }
    if(v.type()==JSValue::Type::String)return v.string();
    if(v.type()==JSValue::Type::Array){A a;for(std::uint32_t i=0;i<v.length();++i)a.push_back(encode(v.get(std::to_string(i))));return a;}
    O o;for(const auto& k:v.keys())o.emplace(k,encode(v.get(k)));return o;
}
inline A names(const std::vector<std::string>& keys){A out;for(const auto& k:keys)out.emplace_back(k);return out;}
inline A flags(const JSValue& v,const char* key){const auto d=v.getOwnPropertyDescriptor(key);return A{d.has_value(),d?d->writable.value_or(false):false,d?d->enumerable.value_or(false):false,d?d->configurable.value_or(false):false};}
inline JsonValue metadata(const JSValue& fn){
    const auto p=fn.get("prototype");
    return O{{"name",encode(fn.get("name"))},{"length",encode(fn.get("length"))},
        {"own",names(fn.ownPropertyNames())},{"nameFlags",flags(fn,"name")},{"lengthFlags",flags(fn,"length")},
        {"prototypeFlags",flags(fn,"prototype")},{"prototypeOwn",p.isObject()?JsonValue(names(p.ownPropertyNames())):JsonValue(nullptr)},
        {"constructorFlags",p.isObject()?JsonValue(flags(p,"constructor")):JsonValue(nullptr)},
        {"constructorIdentity",p.isObject()&&p.get("constructor").sameValue(fn)},
        {"functionPrototype",fn.getPrototype().sameValue(JSValue::function(+[](const JSValue&,const Args&,const Args&)->JSValue{return {};}).getPrototype())}};
}
inline JSValue index(JSValue type=5123,JSValue bytes=2){auto v=JSValue::object();v.set("type",type);v.set("bytesPerElement",bytes);return v;}
inline JSValue list(const Args& values){auto v=JSValue::array();v.get("push").call(v,values);return v;}
struct Fixture {
    JSValue trace=JSValue::array(),gl=JSValue::object(),extension=JSValue::object(),extensions=JSValue::object(),info=JSValue::object();
    JSValue object,key=JSValue::object(),cached;
    explicit Fixture(const std::string& group){
        gl.set("TRIANGLES",4);gl.set("LINES",1);gl.set("LINE_STRIP",3);gl.set("LINE_LOOP",2);gl.set("POINTS",0);
        if(group=="info") {object=WebGLInfo(gl).object();object.get("render").set("calls",7);object.get("render").set("triangles",5);return;}
        if(group=="properties"){object=WebGLProperties().object();cached=object.get("get").call(object,{key});cached.set("before",1);return;}
        for(const auto* n:{"drawArrays","drawArraysInstanced","drawElements","drawElementsInstanced"})gl.set(n,record(n,gl));
        for(const auto* n:{"multiDrawArraysWEBGL","multiDrawElementsWEBGL"})extension.set(n,record(n,extension));
        extensions.set("get",JSValue::function(+[](const JSValue& self,const Args& args,const Args& c)->JSValue{push(c[0],{"get",self.sameValue(c[1]),args[0]});return c[2];},{trace,extensions,extension}));
        info.set("update",record("info",info));
        object=group=="indexed"?WebGLIndexedBufferRenderer(gl,extensions,info).object():WebGLBufferRenderer(gl,extensions,info).object();
        object.get("setMode").call(object,{4});if(group=="indexed")object.get("setIndex").call(object,{index()});
    }
    JSValue record(const char* name,JSValue receiver){return JSValue::function(+[](const JSValue& self,const Args& args,const Args& c)->JSValue{Args line{c[1],self.sameValue(c[2])};line.insert(line.end(),args.begin(),args.end());push(c[0],line);return {};},{trace,name,receiver});}
    Args arguments(const std::string& group,const std::string& method){
        if(group=="info")return method=="reset"?Args{}:Args{12,4,2};
        if(group=="properties")return method=="dispose"?Args{}:method=="update"?Args{key,"x",42}:Args{key};
        if(method=="setMode")return {1};if(method=="setIndex")return {index(5125,4)};
        if(method=="render")return {2,6};if(method=="renderInstances")return {2,6,3};
        return {list({2,4}),list({6,9}),2};
    }
    JsonValue state(const std::string& group){
        if(group=="info")return encode(object.get("render"));
        if(group=="properties")return O{{"old",encode(cached)},{"has",encode(object.get("has").call(object,{key}))}};
        return nullptr;
    }
};
struct ConsoleScope{JSValue previous=WebGLInfo::getConsoleFunction();ConsoleScope(){WebGLInfo::setConsoleFunction(JSValue::function(+[](const JSValue&,const Args&,const Args&)->JSValue{return {};}));}~ConsoleScope(){WebGLInfo::setConsoleFunction(previous);}};
inline JsonValue result(const JSValue& value,const JSValue& fn,const JSValue& target,const JSValue& custom,const JSValue& cached){
    return O{{"object",value.isObject()},{"cached",value.isObject()&&value.sameValue(cached)},
        {"functionPrototype",value.isObject()&&value.getPrototype().sameValue(fn.get("prototype"))},
        {"targetPrototype",value.isObject()&&target.isObject()&&value.getPrototype().sameValue(target.get("prototype"))},
        {"customPrototype",value.isObject()&&value.getPrototype().sameValue(custom)},
        {"objectPrototype",value.isObject()&&value.getPrototype().sameValue(JSValue::object().getPrototype())},
        {"own",value.isObject()?JsonValue(names(value.ownPropertyNames())):encode(value)}};
}
inline JsonValue caseValue(const std::string& group,const std::string& method,int scenario){
    ConsoleScope scope;Fixture f(group);auto fn=f.object.get(method);const auto meta=metadata(fn);
    auto calls=JSValue::array(),custom=JSValue::object();custom.set("marker",17);
    auto target=JSValue::constructor(+[](const JSValue&,const Args&,const Args& c)->JSValue{push(c[0],{"targetBody"});return {};},{calls},"Target",0,JSValue::ConstructorKind::Callable);
    auto args=f.arguments(group,method);JSValue newTarget=fn;
    if(scenario==1||scenario==9){target.set("prototype",custom);if(scenario==9)custom.preventExtensions();newTarget=target;}
    if(scenario==2||scenario==10){target.set("prototype",scenario==2?JSValue(17):JSValue(nullptr));newTarget=target;}
    if(scenario==3)fn.set("prototype",17);
    if(scenario==4)fn.set("prototype",custom);
    if(scenario==5)newTarget=JSValue::function(+[](const JSValue&,const Args&,const Args&)->JSValue{return {};});
    if(scenario==6)newTarget=JSValue::object();
    if(scenario==8)args.clear();
    JsonValue observed;
    try {const auto value=scenario==7?fn.call(nullptr,args):fn.construct(args,newTarget);observed=result(value,fn,newTarget,custom,f.cached);}
    catch(const JSTypeError&){observed=O{{"error","TypeError"}};}catch(const JSRangeError&){observed=O{{"error","RangeError"}};}
    // Subsequent ordinary draws reveal the original closure's mode/index.
    if((group=="buffer"||group=="indexed")&&(method=="setMode"||method=="setIndex"))f.object.get("render").call(f.object,{2,6});
    return O{{"kind","closure-construction"},{"group",group},{"method",method},{"scenario",scenario},{"metadata",meta},
        {"result",observed},{"state",f.state(group)},{"trace",encode(f.trace)},{"targetCalls",encode(calls)}};
}
struct WeakPrototypeScope{
    JSValue prototype=nativeWeakMapConstructor().get("prototype");std::string key;JSPropertyDescriptor old;
    explicit WeakPrototypeScope(std::string name):key(std::move(name)),old(*prototype.getOwnPropertyDescriptor(key)){}
    ~WeakPrototypeScope(){prototype.defineProperty(key,old);}
};
inline JsonValue returnCase(const std::string& method,int scenario){
    Fixture f("properties");auto fn=f.object.get(method);WeakPrototypeScope restore(method);
    JSValue returned=scenario==0?JSValue(true):scenario==1?JSValue::object():scenario==2?JSValue::array():scenario==3?JSValue::constructor(+[](const JSValue&,const Args&,const Args&)->JSValue{return {};},{},"Returned",0,JSValue::ConstructorKind::Callable):scenario==4?JSValue(nullptr):scenario==5?JSValue(17):scenario==6?JSValue("text"):JSValue();
    auto callback=JSValue::function(+[](const JSValue& self,const Args&,const Args& c)->JSValue{push(c[0],{"override",isNativeWeakMap(self)});return c[1];},{f.trace,returned});
    JSPropertyDescriptor d;d.value=callback;restore.prototype.defineProperty(method,d);
    auto custom=JSValue::object(),target=JSValue::constructor(+[](const JSValue&,const Args&,const Args&)->JSValue{return {};},{},"Target",0,JSValue::ConstructorKind::Callable);target.set("prototype",custom);
    const auto value=fn.construct({f.key},target);
    return O{{"kind","closure-return"},{"method",method},{"scenario",scenario},{"returnedIdentity",value.sameValue(returned)},
        {"object",value.isObject()},{"customPrototype",value.getPrototype().sameValue(custom)},
        {"originalCache",value.sameValue(f.cached)},{"own",names(value.ownPropertyNames())},{"trace",encode(f.trace)}};
}
inline JsonValue exceptionCase(const std::string& group,int scenario){
    ConsoleScope scope;Fixture f(group);std::string method;Args args;auto events=f.trace;
    if(group=="info"){
        method=scenario==0?"update":"reset";args={12,4,2};
        if(scenario==0){auto bad=JSValue::object();bad.setPrimitiveConverter(JSValue::function(+[](const JSValue&,const Args& args,const Args& c)->JSValue{push(c[0],{"convert",args[0]});throw JSRangeError("conversion");},{events}));args[0]=bad;}
        else {JSPropertyDescriptor d;d.setter=JSValue::function(+[](const JSValue&,const Args& args,const Args& c)->JSValue{push(c[0],{"setter",args[0]});throw JSRangeError("setter");},{events});d.configurable=true;f.object.get("render").defineProperty("triangles",d);}
    }else if(group=="properties"){
        method=scenario==0?"get":"update";args=scenario==0?Args{17}:Args{f.key,"x",42};
        if(scenario==1){f.object.get("remove").call(f.object,{f.key});auto bad=JSValue::object();bad.setPrimitiveConverter(JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue{push(c[0],{"unexpected-key"});return "x";},{events}));args[1]=bad;}
    }else{
        method=scenario==0?"render":"setIndex";args=scenario==0?Args{2,6}:Args{index(5125,4)};
        if(scenario==0){f.gl.set(group=="indexed"?"drawElements":"drawArrays",JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue{push(c[0],{"draw-throw"});throw JSRangeError("draw");},{events}));}
        else {JSPropertyDescriptor d;d.getter=JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue{push(c[0],{"index-get-throw"});throw JSRangeError("index");},{events});d.configurable=true;args[0].defineProperty("bytesPerElement",d);}
    }
    JsonValue observed;try{observed=encode(f.object.get(method).construct(args));}catch(const JSTypeError&){observed=O{{"error","TypeError"}};}catch(const JSRangeError&){observed=O{{"error","RangeError"}};}
    if(group=="indexed"&&scenario==1)f.object.get("render").call(f.object,{2,6});
    return O{{"kind","closure-exception"},{"group",group},{"scenario",scenario},{"result",observed},{"state",f.state(group)},{"trace",encode(f.trace)}};
}
inline std::vector<JsonValue> cases(const std::string& group){
    std::vector<std::string> methods=group=="info"?std::vector<std::string>{"reset","update"}:group=="properties"?std::vector<std::string>{"has","get","remove","update","dispose"}:group=="indexed"?std::vector<std::string>{"setMode","setIndex","render","renderInstances","renderMultiDraw"}:std::vector<std::string>{"setMode","render","renderInstances","renderMultiDraw"};
    std::vector<JsonValue> out;for(const auto& m:methods)for(int i=0;i<11;++i)out.push_back(caseValue(group,m,i));
    for(int i=0;i<(group=="buffer"?1:2);++i)out.push_back(exceptionCase(group,i));
    if(group=="properties")for(const auto* m:{"has","get"})for(int i=0;i<8;++i)out.push_back(returnCase(m,i));
    return out;
}
}
