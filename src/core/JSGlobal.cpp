#include "core/JSGlobal.h"
#include "math/JSNumberMath.h"
#include "core/JSString.h"
#include "core/JSStringEncoding.h"
#include "core/JSIterator.h"
#include "core/JSMap.h"
#include <cmath>

namespace {
JSValue initialStringConstructor(){
    static thread_local JSWeakValue weak;if(const auto cached=weak.lock();cached&&cached->isConstructible())return *cached;
    const auto prototype=nativeStringPrototype();
    auto constructor=JSValue::callableBuiltinConstructor(+[](const JSValue&,const JSValue& target,const std::vector<JSValue>& args,const std::vector<JSValue>&)->JSValue{
        JSValue text="";
        if(!args.empty()){
            if(target.isUndefined()&&args[0].isSymbol())text="Symbol("+args[0].symbolDescription().value_or("")+")";
            else text=args[0].toString();
        }
        if(target.isUndefined())return text;
        auto proto=target.get("prototype");if(!proto.isObject())proto=nativeStringPrototype();auto result=createNativeStringObject(text);result.setPrototype(proto);return result;
    },{},"String",1,prototype);
    auto fromCharCode=JSValue::function(+[](const JSValue&,const std::vector<JSValue>& args,const std::vector<JSValue>&)->JSValue{
        std::u16string result;result.reserve(args.size());for(const auto& arg:args){const auto number=arg.toNumber();double code=0;if(std::isfinite(number)&&number!=0){code=std::fmod(std::trunc(number),65536.0);if(code<0)code+=65536.0;}result.push_back(static_cast<char16_t>(code));}return jsUTF8(result);
    },{},"fromCharCode",1);
    JSPropertyDescriptor property;property.value=fromCharCode;property.writable=property.configurable=true;constructor.defineProperty("fromCharCode",property);constructor.retainForHeapLifetime();weak=JSWeakValue(constructor);return constructor;
}
JSValue initialMathObject(){
    auto math=JSValue::object();
    for(const auto* name:{"sin","cos"}){
        auto function=JSValue::function(+[](const JSValue&,const std::vector<JSValue>& args,const std::vector<JSValue>& captures)->JSValue{
            const auto input=(args.empty()?JSValue{}:args[0]).toNumber();return captures[0].strictEquals("sin")?JSNumberMath::sin(input):JSNumberMath::cos(input);
        },{name},name,1);
        JSPropertyDescriptor d;d.value=function;d.writable=d.configurable=true;math.defineProperty(name,d);
    }
    JSPropertyDescriptor tag;tag.value="Math";tag.configurable=true;math.defineProperty(JSValue::wellKnownSymbol("toStringTag"),tag);return math;
}
}

JSValue nativeGlobalObject(){
    static thread_local JSWeakValue weak;
    if(const auto object=weak.lock();object&&object->isObject())return *object;
    auto object=JSValue::object(nullptr);JSPropertyDescriptor math;math.value=initialMathObject();math.writable=math.configurable=true;object.defineProperty("Math",math);
    JSPropertyDescriptor string;string.value=initialStringConstructor();string.writable=string.configurable=true;object.defineProperty("String",string);
    JSPropertyDescriptor array;array.value=JSValue::intrinsicArrayConstructor();array.writable=array.configurable=true;object.defineProperty("Array",array);
    JSPropertyDescriptor map;map.value=nativeMapConstructor();map.writable=map.configurable=true;object.defineProperty("Map",map);
    object.retainForHeapLifetime();weak=JSWeakValue(object);return object;
}
JSValue nativeGlobalBinding(const std::string& name){
    const auto global=nativeGlobalObject();
    if(!global.has(name))throw JSReferenceError(name+" is not defined");
    return global.get(name);
}
