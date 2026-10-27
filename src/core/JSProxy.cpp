#include "core/JSProxy.h"
#include <algorithm>
#include <cmath>

namespace {
using Args = std::vector<JSValue>;
struct State { JSValue target, handler; };
State state(const JSValue& proxy) {
    const auto handler = proxy.getHostSlot("Proxy.handler");
    if (handler.isNull()) throw JSTypeError("Cannot perform operation on a revoked Proxy");
    return {proxy.getHostSlot("Proxy.target"), handler};
}
JSValue trap(const State& s, const char* name) {
    const auto method = s.handler.get(name);
    if (method.isUndefined() || method.isNull()) return {};
    if (!method.isCallable()) throw JSTypeError("Proxy trap must be callable");
    return method;
}
JSValue list(const Args& args) {
    auto array = JSValue::array();
    for (std::size_t i=0; i<args.size(); ++i) {
        JSPropertyDescriptor d; d.value=args[i]; d.writable=d.enumerable=d.configurable=true;
        array.defineProperty(std::to_string(i), d);
    }
    return array;
}
bool contains(const Args& keys, const JSValue& key) {
    return std::any_of(keys.begin(), keys.end(), [&](const auto& x){ return x.sameValue(key); });
}
JSValue descriptorObject(const JSPropertyDescriptor& d) {
    auto object=JSValue::object();
    const auto own=[&](const char* key, const JSValue& value) {
        JSPropertyDescriptor field; field.value=value; field.writable=field.enumerable=field.configurable=true; object.defineProperty(key,field);
    };
    if(d.value)own("value",*d.value);
    if(d.writable)own("writable",*d.writable);
    if(d.getter)own("get",*d.getter);
    if(d.setter)own("set",*d.setter);
    if(d.enumerable)own("enumerable",*d.enumerable);
    if(d.configurable)own("configurable",*d.configurable);
    return object;
}
JSPropertyDescriptor toDescriptor(const JSValue& object) {
    JSPropertyDescriptor d;
    if(object.has("enumerable"))d.enumerable=object.get("enumerable").toBoolean();
    if(object.has("configurable"))d.configurable=object.get("configurable").toBoolean();
    if(object.has("value"))d.value=object.get("value");
    if(object.has("writable"))d.writable=object.get("writable").toBoolean();
    for(const auto* name:{"get","set"}) if(object.has(name)) {
        const auto method=object.get(name);
        if(!method.isUndefined()&&!method.isCallable())throw JSTypeError("Invalid proxy property accessor");
        if(name[0]=='g')d.getter=method; else d.setter=method;
    }
    if((d.getter||d.setter)&&(d.value||d.writable))throw JSTypeError("Invalid proxy property descriptor");
    return d;
}
JSPropertyDescriptor complete(JSPropertyDescriptor d) {
    if(d.getter||d.setter) {if(!d.getter)d.getter=JSValue{};if(!d.setter)d.setter=JSValue{};}
    else {if(!d.value)d.value=JSValue{};if(!d.writable)d.writable=false;}
    if(!d.enumerable)d.enumerable=false;if(!d.configurable)d.configurable=false;return d;
}
bool compatible(bool extensible,const JSPropertyDescriptor& d,const std::optional<JSPropertyDescriptor>& old) {
    if(!old)return extensible;
    // ValidateAndApplyPropertyDescriptor on an isolated ordinary object avoids
    // invoking target hooks or changing the target while checking invariants.
    auto scratch=JSValue::object(nullptr);scratch.defineProperty("p",*old);
    return scratch.defineOwnProperty("p",d);
}
}

void revokeNativeProxy(const JSValue& proxy) {
    if(!proxy.isProxy())throw JSTypeError("Expected Proxy");
    auto self=proxy;self.setHostSlot("Proxy.target",nullptr);self.setHostSlot("Proxy.handler",nullptr);
}
bool isNativeArray(const JSValue& value) {
    if(value.isProxy())return isNativeArray(state(value).target);
    return value.type()==JSValue::Type::Array;
}
namespace JSProxyDetail {
JSValue get(const JSValue& self,const JSValue& key,const JSValue& receiver) {
    const auto s=state(self); const auto method=trap(s,"get");
    if(method.isUndefined())return s.target.get(key,receiver);
    const auto result=method.call(s.handler,{s.target,key,receiver});
    const auto d=s.target.getOwnPropertyDescriptor(key);
    if(d&&!d->configurable.value_or(false)) {
        if(d->value&&!d->writable.value_or(false)&&!result.sameValue(*d->value))throw JSTypeError("Proxy get violates non-writable property");
        if(d->getter&&d->getter->isUndefined()&&!result.isUndefined())throw JSTypeError("Proxy get violates accessor property");
    }
    return result;
}
bool set(const JSValue& self,const JSValue& key,const JSValue& value,const JSValue& receiver) {
    const auto s=state(self); const auto method=trap(s,"set");
    if(method.isUndefined())return s.target.reflectSet(key,value,receiver);
    if(!method.call(s.handler,{s.target,key,value,receiver}).toBoolean())return false;
    const auto d=s.target.getOwnPropertyDescriptor(key);
    if(d&&!d->configurable.value_or(false)) {
        if(d->value&&!d->writable.value_or(false)&&!value.sameValue(*d->value))throw JSTypeError("Proxy set violates non-writable property");
        if(d->setter&&d->setter->isUndefined())throw JSTypeError("Proxy set violates accessor property");
    }
    return true;
}
bool has(const JSValue& self,const JSValue& key) {
    const auto s=state(self); const auto method=trap(s,"has");
    if(method.isUndefined())return s.target.has(key);
    const auto result=method.call(s.handler,{s.target,key}).toBoolean();
    if(!result) {
        const auto d=s.target.getOwnPropertyDescriptor(key);
        if(d&&(!d->configurable.value_or(false)||!s.target.isExtensible()))throw JSTypeError("Proxy has cannot hide target property");
    }
    return result;
}
bool erase(const JSValue& self,const JSValue& key) {
    const auto s=state(self); const auto method=trap(s,"deleteProperty");
    if(method.isUndefined()){auto target=s.target;return target.erase(key);}
    if(!method.call(s.handler,{s.target,key}).toBoolean())return false;
    const auto d=s.target.getOwnPropertyDescriptor(key);
    if(d&&(!d->configurable.value_or(false)||!s.target.isExtensible()))throw JSTypeError("Proxy delete cannot hide target property");
    return true;
}
std::optional<JSPropertyDescriptor> descriptor(const JSValue& self,const JSValue& key) {
    const auto s=state(self); const auto method=trap(s,"getOwnPropertyDescriptor");
    if(method.isUndefined())return s.target.getOwnPropertyDescriptor(key);
    const auto result=method.call(s.handler,{s.target,key});
    if(!result.isObject()&&!result.isUndefined())throw JSTypeError("Proxy descriptor must be an object or undefined");
    const auto old=s.target.getOwnPropertyDescriptor(key);
    if(result.isUndefined()) {
        if(!old)return {};
        if(!old->configurable.value_or(false)||!s.target.isExtensible())throw JSTypeError("Proxy descriptor cannot hide target property");
        return {};
    }
    const auto extensible=s.target.isExtensible();
    const auto d=complete(toDescriptor(result));
    if(!compatible(extensible,d,old))throw JSTypeError("Proxy descriptor is incompatible");
    if(!d.configurable.value_or(false)) {
        if(!old||old->configurable.value_or(false))throw JSTypeError("Proxy descriptor cannot invent non-configurable property");
        if(d.value&&!d.writable.value_or(false)&&old->writable.value_or(false))throw JSTypeError("Proxy descriptor cannot invent non-writable property");
    }
    return d;
}
bool define(const JSValue& self,const JSValue& key,const JSPropertyDescriptor& d) {
    const auto s=state(self); const auto method=trap(s,"defineProperty");
    if(method.isUndefined()){auto target=s.target;return target.defineOwnProperty(key,d);}
    if(!method.call(s.handler,{s.target,key,descriptorObject(d)}).toBoolean())return false;
    const auto old=s.target.getOwnPropertyDescriptor(key);const auto ext=s.target.isExtensible();
    const auto settingConfigFalse=d.configurable&&!*d.configurable;
    if(!old) {if(!ext||settingConfigFalse)throw JSTypeError("Proxy cannot define target property");}
    else {
        if(!compatible(ext,d,old)||(settingConfigFalse&&old->configurable.value_or(false)))throw JSTypeError("Proxy define violates target descriptor");
        if(old->value&&!old->configurable.value_or(false)&&old->writable.value_or(false)&&d.writable&&!*d.writable)throw JSTypeError("Proxy cannot invent non-writable property");
    }
    return true;
}
std::vector<JSValue> keys(const JSValue& self) {
    const auto s=state(self); const auto method=trap(s,"ownKeys");
    if(method.isUndefined())return s.target.ownPropertyKeys();
    const auto result=method.call(s.handler,{s.target});
    if(!result.isObject())throw JSTypeError("Proxy ownKeys result must be an object");
    const auto n=result.get("length").toNumber();
    const double length=std::isnan(n)||n<=0?0:std::min(std::floor(n),9007199254740991.0);
    Args keys;
    for(double i=0;i<length;++i) {
        const auto key=result.get(JSValue(i).toString());
        if(key.type()!=JSValue::Type::String&&!key.isSymbol())throw JSTypeError("Proxy ownKeys key must be String or Symbol");
        keys.push_back(key);
    }
    Args unique;for(const auto& key:keys){if(contains(unique,key))throw JSTypeError("Proxy ownKeys contains duplicate key");unique.push_back(key);}
    const auto ext=s.target.isExtensible();const auto targetKeys=s.target.ownPropertyKeys();
    Args required, configurable;
    for(const auto& key:targetKeys) {
        const auto d=s.target.getOwnPropertyDescriptor(key);
        (d&&!d->configurable.value_or(false)?required:configurable).push_back(key);
    }
    for(const auto& key:required)if(!contains(keys,key))throw JSTypeError("Proxy ownKeys omitted non-configurable key");
    if(!ext) {
        for(const auto& key:configurable)if(!contains(keys,key))throw JSTypeError("Proxy ownKeys omitted target key");
        if(keys.size()!=targetKeys.size())throw JSTypeError("Proxy ownKeys invented key on non-extensible target");
    }
    return keys;
}
JSValue prototype(const JSValue& self) {
    const auto s=state(self); const auto method=trap(s,"getPrototypeOf");
    if(method.isUndefined())return s.target.getPrototype();
    const auto result=method.call(s.handler,{s.target});
    if(!result.isObject()&&!result.isNull())throw JSTypeError("Proxy prototype must be object or null");
    if(!s.target.isExtensible()&&!result.sameValue(s.target.getPrototype()))throw JSTypeError("Proxy prototype invariant");
    return result;
}
bool setPrototype(const JSValue& self,const JSValue& value) {
    const auto s=state(self); const auto method=trap(s,"setPrototypeOf");
    if(method.isUndefined()){auto target=s.target;return target.setPrototype(value);}
    if(!method.call(s.handler,{s.target,value}).toBoolean())return false;
    if(!s.target.isExtensible()&&!value.sameValue(s.target.getPrototype()))throw JSTypeError("Proxy prototype invariant");
    return true;
}
bool extensible(const JSValue& self) {
    const auto s=state(self); const auto method=trap(s,"isExtensible");
    if(method.isUndefined())return s.target.isExtensible();
    const auto result=method.call(s.handler,{s.target}).toBoolean();
    if(result!=s.target.isExtensible())throw JSTypeError("Proxy extensibility invariant");return result;
}
bool preventExtensions(const JSValue& self) {
    const auto s=state(self); const auto method=trap(s,"preventExtensions");
    if(method.isUndefined()){
        if(s.target.isProxy())return preventExtensions(s.target);
        auto target=s.target;target.preventExtensions();return true;
    }
    const auto result=method.call(s.handler,{s.target}).toBoolean();
    if(result&&s.target.isExtensible())throw JSTypeError("Proxy preventExtensions invariant");return result;
}
JSValue call(const JSValue& self,const JSValue& receiver,const Args& args) {
    const auto s=state(self); const auto method=trap(s,"apply");
    if(method.isUndefined())return s.target.call(receiver,args);
    return method.call(s.handler,{s.target,receiver,list(args)});
}
JSValue construct(const JSValue& self,const Args& args,const JSValue& newTarget) {
    const auto s=state(self); const auto method=trap(s,"construct");
    if(method.isUndefined())return s.target.construct(args,newTarget);
    const auto result=method.call(s.handler,{s.target,list(args),newTarget});
    if(!result.isObject())throw JSTypeError("Proxy construct result must be object");return result;
}
}
