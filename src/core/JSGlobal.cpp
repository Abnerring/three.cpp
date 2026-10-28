#include "core/JSGlobal.h"
#include "math/JSNumberMath.h"
#include "core/JSString.h"
#include "core/JSStringEncoding.h"
#include "core/JSIterator.h"
#include "core/JSMap.h"
#include "core/JSWeakMap.h"
#include "core/JSSet.h"
#include "core/NativeArray.h"
#include <cmath>
#include <limits>
#include <random>

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
    for(const auto* name:{"sin","cos","ceil","sqrt","round"}){
        auto function=JSValue::function(+[](const JSValue&,const std::vector<JSValue>& args,const std::vector<JSValue>& captures)->JSValue{
            const auto input=(args.empty()?JSValue{}:args[0]).toNumber();
            if(captures[0].strictEquals("sin"))return JSNumberMath::sin(input);
            if(captures[0].strictEquals("cos"))return JSNumberMath::cos(input);
            if(captures[0].strictEquals("sqrt"))return std::sqrt(input);
            if(captures[0].strictEquals("round")){
                if(!std::isfinite(input)||input==0)return input;
                const auto floor=std::floor(input);const auto result=input-floor>=0.5?floor+1:floor;
                return result==0&&input<0?-0.0:result;
            }
            return std::ceil(input);
        },{name},name,1);
        JSPropertyDescriptor d;d.value=function;d.writable=d.configurable=true;math.defineProperty(name,d);
    }
    auto maximum=JSValue::function(+[](const JSValue&,const std::vector<JSValue>& args,const std::vector<JSValue>&)->JSValue{
        // Conversion of later arguments is observable even after an earlier NaN.
        std::vector<double> numbers;numbers.reserve(args.size());for(const auto& value:args)numbers.push_back(value.toNumber());
        double result=-std::numeric_limits<double>::infinity();
        for(const auto number:numbers){if(std::isnan(number))return number;if(number>result||(number==0&&result==0&&!std::signbit(number)))result=number;}
        return result;
    },{},"max",2);
    JSPropertyDescriptor maximumProperty;maximumProperty.value=maximum;maximumProperty.writable=maximumProperty.configurable=true;math.defineProperty("max",maximumProperty);
    auto random=JSValue::function(+[](const JSValue&,const std::vector<JSValue>&,const std::vector<JSValue>&)->JSValue{
        static thread_local std::mt19937_64 engine(std::random_device{}());return std::generate_canonical<double,53>(engine);
    },{},"random",0);
    JSPropertyDescriptor randomProperty;randomProperty.value=random;randomProperty.writable=randomProperty.configurable=true;math.defineProperty("random",randomProperty);
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
    for(const auto& entry:std::initializer_list<std::pair<const char*,JSValue>>{{"Object",JSValue::intrinsicObjectConstructor()},{"WeakMap",nativeWeakMapConstructor()},{"Set",nativeSetConstructor()},{"ArrayBuffer",nativeArrayBufferConstructor()}}){
        JSPropertyDescriptor property;property.value=entry.second;property.writable=property.configurable=true;object.defineProperty(entry.first,property);
    }
    constexpr const char* typedNames[]={"Int8Array","Uint8Array","Int16Array","Uint16Array","Int32Array","Uint32Array","Float32Array","Float64Array","Uint8ClampedArray"};
    for(std::size_t i=0;i<9;++i){JSPropertyDescriptor property;property.value=NativeArray_detail::constructor(i);property.writable=property.configurable=true;object.defineProperty(typedNames[i],property);}
    object.retainForHeapLifetime();weak=JSWeakValue(object);return object;
}
JSValue nativeGlobalBinding(const std::string& name){
    const auto global=nativeGlobalObject();
    if(!global.has(name))throw JSReferenceError(name+" is not defined");
    return global.get(name);
}
