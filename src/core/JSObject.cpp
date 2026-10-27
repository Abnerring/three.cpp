#include "core/JSObject.h"
#include "core/JSString.h"
#include "core/JSNumber.h"
namespace {
JSPropertyDescriptor toPropertyDescriptor(const JSValue& object) {
    if(!object.isObject())throw JSTypeError("Property description must be an object");
    JSPropertyDescriptor descriptor;
    if(object.has("enumerable"))descriptor.enumerable=object.get("enumerable").toBoolean();
    if(object.has("configurable"))descriptor.configurable=object.get("configurable").toBoolean();
    if(object.has("value"))descriptor.value=object.get("value");
    if(object.has("writable"))descriptor.writable=object.get("writable").toBoolean();
    for(const auto* name:{"get","set"})if(object.has(name)) {
        const auto method=object.get(name);
        if(!method.isUndefined()&&!method.isCallable())throw JSTypeError("Property accessor must be callable or undefined");
        if(name[0]=='g')descriptor.getter=method;else descriptor.setter=method;
    }
    if((descriptor.getter||descriptor.setter)&&(descriptor.value||descriptor.writable))
        throw JSTypeError("Property descriptor cannot mix accessors and a value or writable field");
    return descriptor;
}
using Args=std::vector<JSValue>;
JSValue defineProperty(const JSValue&,const Args& args,const Args&) {
    auto target=args.empty()?JSValue{}:args[0];
    if(!target.isObject())throw JSTypeError("Object.defineProperty requires an object");
    const auto key=(args.size()>1?args[1]:JSValue{}).toPropertyKey();
    const auto descriptor=toPropertyDescriptor(args.size()>2?args[2]:JSValue{});
    if(!target.defineOwnProperty(key,descriptor))throw JSTypeError("Cannot define property");
    return target;
}
JSValue assign(const JSValue&,const Args& args,const Args&) {
    auto target=args.empty()?JSValue():args[0];
    if(target.isUndefined()||target.isNull())throw JSTypeError("Cannot convert undefined or null to object");
    if(!target.isObject()) {
        if(target.type()==JSValue::Type::String)target=createNativeStringObject(target);
        else throw JSTypeError("Number, Boolean and Symbol target boxing is not implemented");
    }
    for(std::size_t i=1;i<args.size();++i) {
        auto source=args[i];
        if(source.isUndefined()||source.isNull())continue;
        if(!source.isObject()) {
            if(source.type()==JSValue::Type::String)source=createNativeStringObject(source);
            else continue; // These primitive wrappers have no own enumerable keys.
        }
        const auto keys=source.ownPropertyKeys();
        for(const auto& key:keys) {
            const auto descriptor=source.getOwnPropertyDescriptor(key);
            if(descriptor&&descriptor->enumerable.value_or(false)) {
                const auto value=source.get(key);target.set(key,value);
            }
        }
    }
    return target;
}
JSValue values(const JSValue&,const Args& args,const Args&) {
    auto source=args.empty()?JSValue{}:args[0];
    if(source.isNull()||source.isUndefined())throw JSTypeError("Cannot convert undefined or null to object");
    auto result=JSValue::array();
    if(!source.isObject()) {
        if(source.type()==JSValue::Type::String)source=createNativeStringObject(source);
        else return result; // Other primitive wrappers have no own string keys.
    }
    const auto keys=source.ownPropertyKeys();std::uint32_t index=0;
    for(const auto& key:keys) {
        if(key.isSymbol())continue;
        const auto descriptor=source.getOwnPropertyDescriptor(key);
        if(descriptor&&descriptor->enumerable.value_or(false)) {
            JSPropertyDescriptor d;d.value=source.get(key);d.writable=d.enumerable=d.configurable=true;
            result.defineProperty(std::to_string(index++),d);
        }
    }
    return result;
}
JSValue keys(const JSValue&,const Args& args,const Args&) {
    auto source=args.empty()?JSValue{}:args[0];
    if(source.isNull()||source.isUndefined())throw JSTypeError("Cannot convert undefined or null to object");
    auto result=JSValue::array();
    if(!source.isObject()) {
        if(source.type()==JSValue::Type::String)source=createNativeStringObject(source);
        else return result;
    }
    const auto candidates=source.ownPropertyKeys();std::uint32_t index=0;
    for(const auto& key:candidates) {
        if(key.isSymbol())continue;
        const auto descriptor=source.getOwnPropertyDescriptor(key);
        if(descriptor&&descriptor->enumerable.value_or(false)) {
            JSPropertyDescriptor d;d.value=key;d.writable=d.enumerable=d.configurable=true;
            result.defineProperty(std::to_string(index++),d);
        }
    }
    return result;
}
JSValue getPrototypeOf(const JSValue&,const Args& args,const Args&) {
    auto value=args.empty()?JSValue{}:args[0];
    if(value.isNull()||value.isUndefined())throw JSTypeError("Cannot convert undefined or null to object");
    if(!value.isObject()) {
        if(value.type()==JSValue::Type::String)value=createNativeStringObject(value);
        else if(value.type()==JSValue::Type::Number)value=createNativeNumberObject(value);
        else throw std::logic_error("Object.getPrototypeOf Boolean and Symbol boxing is not implemented");
    }
    return value.getPrototype();
}
JSValue reflectionObject(JSValue value){
    if(value.isNull()||value.isUndefined())throw JSTypeError("Cannot convert undefined or null to object");
    if(value.isObject())return value;
    if(value.type()==JSValue::Type::String)return createNativeStringObject(value);
    if(value.type()==JSValue::Type::Number)return createNativeNumberObject(value);
    // Boolean/Symbol wrapper identity is not exposed by these static methods;
    // their own-key/own-descriptor sets are empty.
    return JSValue::object();
}
void own(JSValue object,const JSValue& key,const JSValue& value){
    JSPropertyDescriptor d;d.value=value;d.writable=d.enumerable=d.configurable=true;object.defineProperty(key,d);
}
JSValue descriptorObject(const std::optional<JSPropertyDescriptor>& descriptor){
    if(!descriptor)return {};
    const auto& d=*descriptor;auto object=JSValue::object();
    if(d.value||d.writable){own(object,"value",d.value.value_or(JSValue{}));own(object,"writable",d.writable.value_or(false));}
    else {own(object,"get",d.getter.value_or(JSValue{}));own(object,"set",d.setter.value_or(JSValue{}));}
    own(object,"enumerable",d.enumerable.value_or(false));own(object,"configurable",d.configurable.value_or(false));return object;
}
JSValue getOwnPropertyNames(const JSValue&,const Args& args,const Args&){
    const auto object=reflectionObject(args.empty()?JSValue{}:args[0]);auto result=JSValue::array();unsigned index=0;
    for(const auto& key:object.ownPropertyKeys())if(!key.isSymbol())own(result,std::to_string(index++),key);
    return result;
}
JSValue getOwnPropertyDescriptor(const JSValue&,const Args& args,const Args&){
    const auto object=reflectionObject(args.empty()?JSValue{}:args[0]);
    const auto key=(args.size()>1?args[1]:JSValue{}).toPropertyKey();
    return descriptorObject(object.getOwnPropertyDescriptor(key));
}
JSValue getOwnPropertyDescriptors(const JSValue&,const Args& args,const Args&){
    const auto object=reflectionObject(args.empty()?JSValue{}:args[0]);auto result=JSValue::object();
    const auto keys=object.ownPropertyKeys();
    for(const auto& key:keys){const auto descriptor=object.getOwnPropertyDescriptor(key);if(descriptor)own(result,key,descriptorObject(descriptor));}
    return result;
}
}
void initializeNativeObjectMethods(const JSValue& constructor) {
    JSPropertyDescriptor d;d.value=JSValue::function(assign,{},"assign",2);
    d.writable=d.configurable=true;d.enumerable=false;
    auto object=constructor;object.defineProperty("assign",d);
    d.value=JSValue::function(values,{},"values",1);object.defineProperty("values",d);
    d.value=JSValue::function(keys,{},"keys",1);object.defineProperty("keys",d);
    d.value=JSValue::function(getPrototypeOf,{},"getPrototypeOf",1);object.defineProperty("getPrototypeOf",d);
    d.value=JSValue::function(defineProperty,{},"defineProperty",3);object.defineProperty("defineProperty",d);
    d.value=JSValue::function(getOwnPropertyNames,{},"getOwnPropertyNames",1);object.defineProperty("getOwnPropertyNames",d);
    d.value=JSValue::function(getOwnPropertyDescriptor,{},"getOwnPropertyDescriptor",2);object.defineProperty("getOwnPropertyDescriptor",d);
    d.value=JSValue::function(getOwnPropertyDescriptors,{},"getOwnPropertyDescriptors",1);object.defineProperty("getOwnPropertyDescriptors",d);
}
