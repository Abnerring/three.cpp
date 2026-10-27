#include "core/JSObject.h"
#include "core/JSString.h"
#include "core/JSNumber.h"
namespace {
using Args=std::vector<JSValue>;
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
}
void initializeNativeObjectMethods(const JSValue& constructor) {
    JSPropertyDescriptor d;d.value=JSValue::function(assign,{},"assign",2);
    d.writable=d.configurable=true;d.enumerable=false;
    auto object=constructor;object.defineProperty("assign",d);
    d.value=JSValue::function(values,{},"values",1);object.defineProperty("values",d);
    d.value=JSValue::function(keys,{},"keys",1);object.defineProperty("keys",d);
    d.value=JSValue::function(getPrototypeOf,{},"getPrototypeOf",1);object.defineProperty("getPrototypeOf",d);
}
