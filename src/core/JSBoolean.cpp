#include "core/JSBoolean.h"
namespace {
using Args=std::vector<JSValue>;
struct BooleanData { bool value; };
JSPropertyDescriptor data(const JSValue& value){JSPropertyDescriptor d;d.value=value;d.writable=d.configurable=true;return d;}
JSValue allocate(const JSValue& primitive){
    if(primitive.type()!=JSValue::Type::Boolean)throw JSTypeError("BooleanData requires a primitive Boolean");
    auto payload=std::make_shared<BooleanData>(BooleanData{primitive.toBoolean()});
    return JSValue::hostObject(payload,payload.get(),"three.Boolean");
}
JSValue valueOf(const JSValue& self,const Args&,const Args&){return nativeBooleanValue(self);}
JSValue toString(const JSValue& self,const Args&,const Args&){return nativeBooleanValue(self).toBoolean()?"true":"false";}
}
bool isNativeBooleanObject(const JSValue& value){static_cast<void>(JSWeakValue(value));return value.isHostObject("three.Boolean");}
JSValue nativeBooleanValue(const JSValue& receiver){
    static_cast<void>(JSWeakValue(receiver));
    if(receiver.type()==JSValue::Type::Boolean)return receiver;
    if(receiver.isHostObject("three.Boolean"))return std::static_pointer_cast<BooleanData>(receiver.hostPayload("three.Boolean"))->value;
    throw JSTypeError("Boolean method called on incompatible receiver");
}
JSValue nativeBooleanConstructor(){
    static thread_local JSWeakValue weak;
    if(const auto value=weak.lock();value&&value->isConstructible())return *value;
    auto prototype=allocate(false);
    auto constructor=JSValue::callableBuiltinConstructor(+[](const JSValue&,const JSValue& target,const Args& args,const Args&)->JSValue{
        const JSValue value=(args.empty()?JSValue{}:args[0]).toBoolean();
        if(target.isUndefined())return value;
        const auto prototype=target.getPrototypeFromConstructor(nativeBooleanPrototype());
        auto result=allocate(value);result.setPrototype(prototype);return result;
    },{},"Boolean",1,prototype);
    prototype.defineProperty("toString",data(JSValue::function(toString,{},"toString",0)));
    prototype.defineProperty("valueOf",data(JSValue::function(valueOf,{},"valueOf",0)));
    constructor.retainForHeapLifetime();weak=JSWeakValue(constructor);return constructor;
}
JSValue nativeBooleanPrototype(){return nativeBooleanConstructor().get("prototype");}
JSValue createNativeBooleanObject(const JSValue& primitive){
    static_cast<void>(JSWeakValue(primitive));auto result=allocate(primitive);result.setPrototype(nativeBooleanPrototype());return result;
}
