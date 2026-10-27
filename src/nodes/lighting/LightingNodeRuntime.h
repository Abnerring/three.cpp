#pragma once
#include "nodes/utils/UtilityNodeRuntime.h"
#include <tuple>
namespace LightingNodeRuntime {
using namespace UtilityNodeRuntime;
inline JSValue makeClass(const char* name, unsigned arity, const JSValue& base, JSValue::NativeFunction initialize, const Args& captures, const char* source, const char* typeSource) {
    auto holder=JSValue::object(nullptr); Args values{holder}; values.insert(values.end(),captures.begin(),captures.end());
    auto ctor=JSValue::builtinConstructor(initialize,values,name,arity);holder.set("constructor",ctor);ctor.setPrototype(base);
    auto prototype=ctor.get("prototype");prototype.setPrototype(base.get("prototype"));ctor.setFunctionSourceText(source);
    JSPropertyDescriptor type;type.configurable=true;type.getter=JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue{return c[0];},{name},"get type",0);type.getter->setFunctionSourceText(typeSource);ctor.defineProperty("type",type);return ctor;
}
inline void method(JSValue prototype,const char* name,unsigned arity,JSValue::NativeFunction code,const Args& captures,const char* source){
    JSPropertyDescriptor d;d.value=JSValue::function(code,captures,name,arity);d.writable=d.configurable=true;d.value->setFunctionSourceText(source);prototype.defineProperty(name,d);
}
inline JSValue publish(const JSValue& bindings,JSWeakValue& lexical,JSWeakValue& cached){
    auto module=JSValue::object();for(const auto& key:bindings.ownPropertyNames())own(module,key.c_str(),bindings.get(key));
    bindings.retainForHeapLifetime();lexical=JSWeakValue(bindings);module.retainForHeapLifetime();cached=JSWeakValue(module);return module;
}
}
