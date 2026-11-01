#include "nodes/core/Node.h"
#include "core/JSGlobal.h"
#include "core/JSPromise.h"
#include <exception>
#include "animation/AnimationUtils.h"
#include <cmath>
#include <iostream>
#include <iterator>
#include <limits>
namespace {
using Args=std::vector<JSValue>;
JSValue arg(const Args& values,unsigned index){return index<values.size()?values[index]:JSValue{};}
JSValue array(const Args& values={}){auto out=JSValue::array();for(const auto& value:values)out.get("push").call(out,{value});return out;}
void push(JSValue out,const JSValue& value){out.get("push").call(out,{value});}
JSValue encode(const JSValue& value){if(value.isUndefined())return array({"undefined"});if(value.type()==JSValue::Type::Number){const auto number=value.number();if(std::isnan(number))return array({"number","NaN"});if(!std::isfinite(number))return array({"number",number>0?"Infinity":"-Infinity"});if(number==0&&std::signbit(number))return array({"number","-0"});}return value;}
JSValue run(const JSValue& request){const auto constructor=nativeNodeConstructor();auto instance=constructor.construct(),receiver=instance;
 const auto requested=request.get("receiver");if(requested.strictEquals("host-null")){receiver=JSValue::object();receiver.set("_uuid",nullptr);}else if(requested.strictEquals("host-undefined"))receiver=JSValue::object();else if(requested.strictEquals("number"))receiver=17;else if(requested.strictEquals("null"))receiver=nullptr;
 if(request.get("cached").toBoolean()){const auto cached=request.get("cached");instance.set("_uuid",cached.strictEquals("undefined")?JSValue{}:cached.strictEquals("zero")?JSValue(0):cached.strictEquals("false")?JSValue(false):JSValue("existing"));}
 auto state=JSValue::object();state.set("request",request);state.set("events",array());state.set("calls",0);state.set("cache",nullptr);auto values=array();
 if(request.get("cacheGetter").toBoolean()){JSPropertyDescriptor descriptor;descriptor.enumerable=descriptor.configurable=true;descriptor.getter=JSValue::function(+[](const JSValue&,const Args&,const Args& cap)->JSValue{push(cap[0].get("events"),"get:_uuid");if(cap[0].get("request").get("cacheGetThrows").toBoolean())throw JSTypeError("cache get");return cap[0].get("cache");},{state},"get",0);descriptor.setter=JSValue::function(+[](const JSValue&,const Args& args,const Args& cap)->JSValue{push(cap[0].get("events"),"set:_uuid");if(cap[0].get("request").get("cacheSetThrows").toBoolean())throw JSTypeError("cache set");auto state=cap[0];state.set("cache",arg(args,0));return {};},{state},"set",1);instance.defineProperty("_uuid",descriptor);}
 auto math=nativeGlobalBinding("Math");const auto original=math.get("random");JSValue callback;
 if(request.get("notCallable").toBoolean())callback=17;else callback=JSValue::function(+[](const JSValue&,const Args&,const Args& cap)->JSValue{auto state=cap[0];const auto count=state.get("calls").number()+1;state.set("calls",count);push(state.get("events"),"random:"+JSValue(count).toString());const auto request=state.get("request");if(request.get("throwAt").strictEquals(count))throw JSTypeError("random");const auto tag=request.get("randomTag");if(tag.strictEquals("NaN"))return std::numeric_limits<double>::quiet_NaN();if(tag.strictEquals("Infinity"))return std::numeric_limits<double>::infinity();if(tag.strictEquals("-0"))return -0.;return request.has("random")?request.get("random"):JSValue(std::fmod(count,257.)/257.);},{state},"random",0);
 math.set("random",callback);try{const auto getter=*constructor.get("prototype").getOwnPropertyDescriptor("uuid")->getter;const auto reads=request.get("reads").isUndefined()?1:request.get("reads").toNumber();for(unsigned index=0;index<reads;++index){if(index&&request.get("reset").toBoolean())receiver.set("_uuid",nullptr);auto value=JSValue::object();try{value.set("value",encode(getter.call(receiver)));}catch(...){const auto error=nativeExceptionValue(std::current_exception());value.set("error",error.get("constructor").get("name"));}push(values,value);}}catch(...){math.set("random",original);throw;}math.set("random",original);
 auto out=JSValue::object();out.set("name",request.get("name"));out.set("id",instance.get("id"));out.set("values",values);out.set("randomCalls",state.get("calls"));out.set("events",state.get("events"));return out;
}
}
int main(){try{const std::string input((std::istreambuf_iterator<char>(std::cin)),{});const auto requests=Animation_detail::parseJSON(input);auto out=array();for(unsigned index=0;index<requests.length();++index)push(out,run(requests.get(JSValue(index))));std::cout<<out.toJSON()->dump();}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}}
