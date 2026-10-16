// Ported warning/console protocol from three.js utils.js; see THREEJS-LICENSE.
#include "utils.h"
#include "constants.h"
#include "core/JSIterator.h"
#include "renderers/webgl/WebGLInfo.h"
#include <iostream>

namespace {
using Args=std::vector<JSValue>;
JSValue rest(const Args& args){
    auto array=JSValue::array(static_cast<std::uint32_t>(args.size()));
    for(std::size_t i=0;i<args.size();++i){
        JSPropertyDescriptor d;d.value=args[i];d.writable=d.enumerable=d.configurable=true;array.defineProperty(std::to_string(i),d);
    }
    return array;
}
JSValue warningCache(){
    static thread_local JSWeakValue weak;
    if(const auto cached=weak.lock();cached&&cached->isObject())return *cached;
    auto cache=JSValue::object();cache.retainForHeapLifetime();weak=JSWeakValue(cache);return cache;
}
JSValue plus(const JSValue& left,const JSValue& right){
    const auto a=left.toPrimitive(),b=right.toPrimitive();
    if(a.type()==JSValue::Type::String||b.type()==JSValue::Type::String)return a.toString()+b.toString();
    const auto x=a.toNumber();return x+b.toNumber();
}
Args spread(const JSValue& value){
    auto iterator=getJSIterator(value);Args result;
    while(const auto next=stepJSIterator(iterator))result.push_back(*next);
    return result;
}
void logArray(JSValue params,const char* level){
    const auto first=params.get("0");
    if(first.type()==JSValue::Type::String&&first.get("startsWith").call(first,{"TSL:"}).toBoolean()){
        const auto stack=params.get("1");
        if(stack.toBoolean()&&stack.get("isStackTrace").toBoolean()){
            const auto previous=params.get("0");
            const auto location=stack.get("getLocation").call(stack);
            params.set("0",plus(previous,plus(" ",location)));
        }else params.set("1","Stack trace not available. Enable \"THREE.Node.captureStackTrace\" to capture stack traces.");
    }
    const auto shift=params.get("shift");
    const auto message=plus("THREE.",shift.call(params));
    if(ThreeUtils::getConsoleFunction().toBoolean()){
        const auto callback=ThreeUtils::getConsoleFunction();
        Args args{level,message};const auto remaining=spread(params);args.insert(args.end(),remaining.begin(),remaining.end());
        callback.call({},args);
    }else{
        const auto stack=params.get("0");
        if(stack.toBoolean()&&stack.get("isStackTrace").toBoolean())
            std::cerr<<stack.get("getError").call(stack,{message}).trace().dump()<<'\n';
        else{
            const auto remaining=spread(params);
            std::cerr<<message.toString();for(const auto& value:remaining)std::cerr<<' '<<value.trace().dump();std::cerr<<'\n';
        }
    }
}
}
JSValue ThreeUtils::nativeReversedDepthFuncs(){
    static thread_local JSWeakValue weak;
    static thread_local bool initializing=false;
    if(const auto cached=weak.lock();cached&&cached->isObject())return *cached;
    JSModuleInitializationScope scope(initializing,"ReversedDepthFuncs");
    auto result=JSValue::object();
    for(const auto& [a,b]:std::initializer_list<std::pair<int,int>>{
        {NeverDepth,AlwaysDepth},{LessDepth,GreaterDepth},{EqualDepth,NotEqualDepth},{LessEqualDepth,GreaterEqualDepth},
        {AlwaysDepth,NeverDepth},{GreaterDepth,LessDepth},{NotEqualDepth,EqualDepth},{GreaterEqualDepth,LessEqualDepth}}){
        JSPropertyDescriptor d;d.value=b;d.writable=d.enumerable=d.configurable=true;result.defineProperty(JSValue(a).toString(),d);
    }
    result.retainForHeapLifetime();weak=JSWeakValue(result);return result;
}
void ThreeUtils::setConsoleFunction(const JSValue& callback){static_cast<void>(JSWeakValue(callback));WebGLInfo::setConsoleFunction(callback);}
JSValue ThreeUtils::getConsoleFunction(){return WebGLInfo::getConsoleFunction();}
void ThreeUtils::warn(const Args& params){for(const auto& value:params)static_cast<void>(JSWeakValue(value));logArray(rest(params),"warn");}
void ThreeUtils::error(const Args& params){for(const auto& value:params)static_cast<void>(JSWeakValue(value));logArray(rest(params),"error");}
void ThreeUtils::warnOnce(const Args& params){
    for(const auto& value:params)static_cast<void>(JSWeakValue(value));
    auto values=rest(params);
    const auto message=values.get("join").call(values,{" "});
    auto cache=warningCache();
    if(cache.has(message))return;
    cache.set(message,true);
    // Spread into warn creates another rest array and uses the live iterator.
    warn(spread(values));
}
