// Ported warning/console protocol from three.js utils.js; see THREEJS-LICENSE.
#include "utils.h"
#include "core/JSPromise.h"
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
JSValue ThreeUtils::yieldToMain() {
    const auto capability = createNativePromiseCapability();
    queueHostTask(capability.resolve);
    return capability.promise;
}
JSValue ThreeUtils::getConsoleFunction(){return WebGLInfo::getConsoleFunction();}
void ThreeUtils::log(const Args& params){
    for(const auto& value:params)static_cast<void>(JSWeakValue(value));
    auto values=rest(params);const auto shift=values.get("shift");const auto message=plus("THREE.",shift.call(values));
    if(getConsoleFunction().toBoolean()){
        const auto callback=getConsoleFunction();Args args{"log",message};const auto remaining=spread(values);args.insert(args.end(),remaining.begin(),remaining.end());callback.call({},args);
    }else{
        const auto remaining=spread(values);std::cout<<message.toString();for(const auto& value:remaining)std::cout<<' '<<value.trace().dump();std::cout<<'\n';
    }
}
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

namespace ThreeUtils {
JSValue nativeErrorFunction(){static thread_local JSWeakValue weak;if(const auto existing=weak.lock();existing&&existing->isCallable())return *existing;auto function=JSValue::constructor(+[](const JSValue&,const std::vector<JSValue>& args,const std::vector<JSValue>&)->JSValue{error(args);return {};},{},"error",0,JSValue::ConstructorKind::Callable);function.setFunctionSourceText("function error( ...params ) {\r\n\r\n\tparams = enhanceLogMessage( params );\r\n\r\n\tconst message = 'THREE.' + params.shift();\r\n\r\n\tif ( _setConsoleFunction ) {\r\n\r\n\t\t_setConsoleFunction( 'error', message, ...params );\r\n\r\n\t} else {\r\n\r\n\t\tconst stackTrace = params[ 0 ];\r\n\r\n\t\tif ( stackTrace && stackTrace.isStackTrace ) {\r\n\r\n\t\t\tconsole.error( stackTrace.getError( message ) );\r\n\r\n\t\t} else {\r\n\r\n\t\t\tconsole.error( message, ...params );\r\n\r\n\t\t}\r\n\r\n\t}\r\n\r\n}");function.retainForHeapLifetime();weak=JSWeakValue(function);return function;}
JSValue nativeWarnFunction(){static thread_local JSWeakValue weak;if(const auto existing=weak.lock();existing&&existing->isCallable())return *existing;auto function=JSValue::constructor(+[](const JSValue&,const std::vector<JSValue>& args,const std::vector<JSValue>&)->JSValue{warn(args);return {};},{},"warn",0,JSValue::ConstructorKind::Callable);function.setFunctionSourceText("function warn( ...params ) {\r\n\r\n\tparams = enhanceLogMessage( params );\r\n\r\n\tconst message = 'THREE.' + params.shift();\r\n\r\n\tif ( _setConsoleFunction ) {\r\n\r\n\t\t_setConsoleFunction( 'warn', message, ...params );\r\n\r\n\t} else {\r\n\r\n\t\tconst stackTrace = params[ 0 ];\r\n\r\n\t\tif ( stackTrace && stackTrace.isStackTrace ) {\r\n\r\n\t\t\tconsole.warn( stackTrace.getError( message ) );\r\n\r\n\t\t} else {\r\n\r\n\t\t\tconsole.warn( message, ...params );\r\n\r\n\t\t}\r\n\r\n\t}\r\n\r\n}");function.retainForHeapLifetime();weak=JSWeakValue(function);return function;}
}

#include "core/JSGlobal.h"
#include "math/JSNumberMath.h"
#include "core/JSIterator.h"
#include "core/JSStringEncoding.h"
#include <string_view>
#include <unordered_set>

namespace GeometryArrayUtils {

using Args=std::vector<JSValue>;
JSValue arg(const Args& args,unsigned i){return i<args.size()?args[i]:JSValue{};}
void own(JSValue object,const JSValue& key,const JSValue& value){JSPropertyDescriptor d;d.value=value;d.writable=d.enumerable=d.configurable=true;object.defineProperty(key,d);}
void setReference(JSValue base,const JSValue& key,const JSValue& value){if(base.isNull()||base.isUndefined())throw JSTypeError("Cannot assign property of null/undefined");const auto property=key.toPropertyKey();if(base.isObject()){base.set(property,value);return;}auto object=base.toObject();if(!object.reflectSet(property,value,base))throw JSTypeError("Cannot assign primitive property");}
JSValue deleteReference(JSValue base,const JSValue& key){if(base.isNull()||base.isUndefined())throw JSTypeError("Cannot delete property of null/undefined");const auto property=key.toPropertyKey();auto object=base.toObject();if(!object.erase(property))throw JSTypeError("Cannot delete nonconfigurable property");return true;}
JSValue argumentObject(const Args& args){auto object=JSValue::object();JSPropertyDescriptor length;length.value=args.size();length.writable=length.configurable=true;object.defineProperty("length",length);for(unsigned i=0;i<args.size();++i)own(object,JSValue(i),args[i]);return object;}
JSValue typeOf(const JSValue& value){if(value.isUndefined())return "undefined";if(value.isCallable())return "function";if(value.isSymbol())return "symbol";switch(value.type()){case JSValue::Type::Boolean:return "boolean";case JSValue::Type::Number:return "number";case JSValue::Type::String:return "string";default:return "object";}}
JSValue binary(std::string_view op,const JSValue& left,const JSValue& right){
 if(op=="in")return right.has(left);if(op=="===")return left.strictEquals(right);if(op=="!==")return !left.strictEquals(right);
 if(op=="+"){const auto a=left.toPrimitive();const auto b=right.toPrimitive();if(a.type()==JSValue::Type::String||b.type()==JSValue::Type::String){const auto x=a.toString();const auto y=b.toString();return x+y;}const auto x=a.toNumber();const auto y=b.toNumber();return x+y;}
 if(op=="<"||op==">"||op=="<="||op==">="){const auto a=left.toPrimitive(JSValue::PreferredType::Number);const auto b=right.toPrimitive(JSValue::PreferredType::Number);if(a.type()==JSValue::Type::String&&b.type()==JSValue::Type::String){const auto x=jsUTF16(a.string()),y=jsUTF16(b.string());if(op=="<")return x<y;if(op==">")return x>y;if(op=="<=")return x<=y;return x>=y;}const auto x=a.toNumber();const auto y=b.toNumber();if(op=="<")return x<y;if(op==">")return x>y;if(op=="<=")return x<=y;return x>=y;}
 const auto a=left.toNumber();const auto b=right.toNumber();if(op=="-")return a-b;if(op=="*")return a*b;if(op=="/")return a/b;if(op=="**")return JSNumberMath::pow(a,b);throw std::logic_error("Untranslated arithmetic operator");
}
void appendSpread(Args& values,const JSValue& iterable){auto iterator=getJSIterator(iterable);while(const auto value=stepJSIterator(iterator))values.push_back(*value);}
void copyDataProperties(JSValue target,JSValue source){if(source.isNull()||source.isUndefined())return;if(!source.isObject())source=source.toObject();for(const auto& key:source.ownPropertyKeys()){const auto d=source.getOwnPropertyDescriptor(key);if(d&&d->enumerable.value_or(false)){const auto value=source.get(key);own(target,key,value);}}}
class ForInCursor{JSValue object;std::vector<JSValue> keys;std::unordered_set<std::string> visited;std::size_t index=0;bool loaded=false;public:explicit ForInCursor(JSValue source):object(source.isNull()||source.isUndefined()?JSValue(nullptr):source.toObject()){}std::optional<JSValue> next(){while(!object.isNull()){if(!loaded){keys=object.ownPropertyKeys();index=0;loaded=true;}while(index<keys.size()){const auto key=keys[index++];if(key.isSymbol())continue;const auto name=key.string();if(visited.contains(name))continue;const auto d=object.getOwnPropertyDescriptor(key);if(!d)continue;visited.insert(name);if(d->enumerable.value_or(false))return key;}object=object.getPrototype();loaded=false;}return std::nullopt;}};

JSValue arrayNeedsUint32Body(const JSValue& nativeStatementTemp0,const Args& nativeStatementTemp1,const Args& nativeStatementTemp2){
auto nativeStatementTemp3=nativeStatementTemp2[0];auto nativeStatementTemp4=nativeStatementTemp0;
JSValue array=arg(nativeStatementTemp1,0);
{JSValue i=([&]()->JSValue{const auto nativeStatementTemp6=([&]()->JSValue{const auto nativeStatementTemp8=array;const auto nativeStatementTemp9=JSValue("length");return nativeStatementTemp8.get(nativeStatementTemp9);}());const auto nativeStatementTemp7=JSValue(1);return binary("-",nativeStatementTemp6,nativeStatementTemp7);}());
for(;([&]()->JSValue{const auto nativeStatementTemp10=i;const auto nativeStatementTemp11=JSValue(0);return binary(">=",nativeStatementTemp10,nativeStatementTemp11);}()).toBoolean();([&]()->JSValue{const auto nativeStatementTemp12=i.toNumber();const auto nativeStatementTemp13=nativeStatementTemp12-1;i=nativeStatementTemp13;return nativeStatementTemp13;}())){
if(([&]()->JSValue{const auto nativeStatementTemp14=([&]()->JSValue{const auto nativeStatementTemp16=array;const auto nativeStatementTemp17=i;return nativeStatementTemp16.get(nativeStatementTemp17);}());const auto nativeStatementTemp15=JSValue(65535);return binary(">=",nativeStatementTemp14,nativeStatementTemp15);}()).toBoolean())return JSValue(true);

}
}
return JSValue(false);
return JSValue{};
}
}
namespace ThreeUtils {
JSValue nativeArrayNeedsUint32(){static thread_local JSWeakValue weak;if(const auto cached=weak.lock();cached&&cached->isCallable())return *cached;auto function=JSValue::constructor(GeometryArrayUtils::arrayNeedsUint32Body,{JSValue{}},"arrayNeedsUint32",1,JSValue::ConstructorKind::Callable);function.setFunctionSourceText("function arrayNeedsUint32( array ) {\r\n\r\n\t// assumes larger values usually on last\r\n\r\n\tfor ( let i = array.length - 1; i >= 0; -- i ) {\r\n\r\n\t\tif ( array[ i ] >= 65535 ) return true; // account for PRIMITIVE_RESTART_FIXED_INDEX, #24565\r\n\r\n\t}\r\n\r\n\treturn false;\r\n\r\n}");function.retainForHeapLifetime();weak=JSWeakValue(function);return function;}
}

namespace ThreeUtils {
JSValue nativeWarnOnceFunction(){static thread_local JSWeakValue weak;if(const auto existing=weak.lock();existing&&existing->isCallable())return *existing;const auto cache=warningCache();auto function=JSValue::constructor(+[](const JSValue&,const std::vector<JSValue>& args,const std::vector<JSValue>&)->JSValue{warnOnce(args);return {};},{cache},"warnOnce",0,JSValue::ConstructorKind::Callable);function.setFunctionSourceText("function warnOnce( ...params ) {\r\n\r\n\tconst message = params.join( ' ' );\r\n\r\n\tif ( message in _cache ) return;\r\n\r\n\t_cache[ message ] = true;\r\n\r\n\twarn( ...params );\r\n\r\n}");function.retainForHeapLifetime();weak=JSWeakValue(function);return function;}
}
