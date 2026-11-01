#include "renderers/shaders/UniformsUtils.h"
#include "core/JSSymbol.h"
#include "core/JSGlobal.h"
#include "core/JSPromise.h"
#include "core/JSError.h"
#include "core/JSJobs.h"
#include "animation/AnimationUtils.h"
#include <cmath>
#include <exception>
#include <iostream>
#include <iterator>

namespace {
using Args=std::vector<JSValue>;
JSValue array(const Args& values={}){auto out=JSValue::array();for(unsigned i=0;i<values.size();++i)out.set(std::to_string(i),values[i]);return out;}
void push(const JSValue& receiver,const JSValue& value){receiver.get("push").call(receiver,{value});}
JSValue uniform(const JSValue& value){auto out=JSValue::object();out.set("value",value);return out;}
JSValue typeofValue(const JSValue& value){switch(value.type()){case JSValue::Type::Undefined:return "undefined";case JSValue::Type::Boolean:return "boolean";case JSValue::Type::Number:return "number";case JSValue::Type::String:return "string";case JSValue::Type::Symbol:return "symbol";case JSValue::Type::Function:return "function";default:return "object";}}
JSValue encode(const JSValue& value){
 if(value.isUndefined())return array({"undefined"});
 if(value.type()==JSValue::Type::Number){const auto n=value.number();if(std::isnan(n))return array({"number","NaN"});if(std::isinf(n))return array({"number",n>0?"Infinity":"-Infinity"});if(n==0&&std::signbit(n))return array({"number","-0"});}
 if(value.isSymbol()){const auto description=value.symbolDescription();return array({"symbol",encode(description?JSValue(*description):JSValue{}),encode(nativeSymbolKeyFor(value))});}
 if(!value.isObject())return value;
 auto out=JSValue::object(),keys=array(),values=array();for(const auto& key:value.ownPropertyKeys()){push(keys,encode(key));push(values,encode(value.get(key)));}
 out.set("kind",value.type()==JSValue::Type::Array?"array":"object");out.set("keys",keys);out.set("values",values);return out;
}
template<class F>JSValue attempt(F callback){auto out=JSValue::object();try{out.set("value",encode(callback()));}catch(...){const auto error=nativeExceptionValue(std::current_exception());if(isNativeError(error)){out.set("error",error.get("name"));out.set("nativeError",true);out.set("typeError",error.instanceOf(nativeTypeErrorConstructor()));}else out.set("thrown",encode(error));}return out;}
JSValue getter(const JSValue& self,const Args&,const Args& captures){
 auto state=captures[0];const auto request=state.get("request"),events=state.get("events");const double reads=state.get("reads").toNumber()+1;state.set("reads",reads);
 push(events,array({"get",state.get("key"),typeofValue(self),self.sameValue(state.get("receiver")),reads}));
 const auto action=request.get("action");
 if(action.strictEquals("throwNumber"))throw JSThrownValue(17);
 if(action.strictEquals("throwTypeError"))throw JSTypeError("uniform getter");
 if(action.strictEquals("reenter")&&reads==1){auto source=JSValue::object();source.set("nested",uniform(request.get("value").toNumber()+100));const auto nested=cloneUniforms(source);push(events,array({"nested",nested.get("nested").get("value")}));}
 const auto value=JSValue(request.get("value").toNumber()+reads);return request.get("level").strictEquals("outer")?uniform(value):value;
}
JSValue aliasGetter(const JSValue&,const Args&,const Args& captures){push(captures[0],array({captures[1]}));throw JSThrownValue(captures[2]);}
JSValue run(const JSValue& request){
 const auto symbol=JSValue::symbol(request.get("name").string()),receiver=request.get("boxed").toBoolean()?symbol.toObject():symbol;
 const bool outer=request.get("level").strictEquals("outer");const std::string key=outer?"uniform":"value";
 auto prototype=nativeSymbolPrototype(),global=nativeGlobalObject(),events=array(),state=JSValue::object();
 const auto savedProperty=prototype.getOwnPropertyDescriptor(key);
 const auto savedConstructor=*prototype.getOwnPropertyDescriptor("constructor"),savedGlobal=*global.getOwnPropertyDescriptor("Symbol");
 auto source=receiver;if(!outer){source=JSValue::object();source.set("uniform",receiver);}
 state.set("request",request);state.set("receiver",receiver);state.set("key",key);state.set("reads",0);state.set("events",events);
 auto restore=[&](){if(savedProperty)prototype.defineProperty(key,*savedProperty);else prototype.erase(key);prototype.defineProperty("constructor",savedConstructor);global.defineProperty("Symbol",savedGlobal);};
 try{
  JSPropertyDescriptor property;property.enumerable=property.configurable=true;
  if(request.get("accessor").toBoolean())property.getter=JSValue::function(getter,{state},"get uniform",0);
  else{property.value=outer?uniform(request.get("value")):request.get("value");property.writable=true;}
  prototype.defineProperty(key,property);
  const auto alias=request.get("alias");
  if(alias.strictEquals("data")){
   const auto fake=JSValue::function(aliasGetter,{events,"global Symbol call",19},"FakeSymbol",0);
   JSPropertyDescriptor descriptor;descriptor.value=fake;descriptor.writable=descriptor.configurable=true;descriptor.enumerable=false;
   global.defineProperty("Symbol",descriptor);prototype.defineProperty("constructor",descriptor);
  }else if(alias.strictEquals("getters")){
   JSPropertyDescriptor descriptor;descriptor.configurable=true;descriptor.enumerable=false;descriptor.getter=JSValue::function(aliasGetter,{events,"global Symbol getter",19},"get Symbol",0);global.defineProperty("Symbol",descriptor);
   descriptor.getter=JSValue::function(aliasGetter,{events,"prototype constructor getter",23},"get constructor",0);prototype.defineProperty("constructor",descriptor);
  }
  auto out=JSValue::object();out.set("result",attempt([&](){return request.get("operation").strictEquals("merge")?nativeMergeUniforms().call({}, {array({source})}):nativeCloneUniforms().call({}, {source});}));out.set("events",encode(events));restore();return out;
 }catch(...){restore();throw;}
}
JSValue execute(const JSValue& requests){if(requests.type()!=JSValue::Type::Array)throw std::invalid_argument("Input array required");auto out=array();for(unsigned i=0;i<requests.length();++i){const auto request=requests.get(std::to_string(i));auto row=JSValue::object();row.set("name",request.get("name"));row.set("value",run(request));push(out,row);endJSJob();requests.collectGarbage();}return out;}
void lifetime(const std::string& input){for(unsigned i=0;i<3;++i){{auto requests=Animation_detail::parseJSON(input);execute(requests);}endJSJob();JSValue::nativeSafepoint();if(JSValue::hasActiveHeap())throw std::logic_error("Uniforms Symbol reentry/descriptor captures retained a retired heap");}std::cout<<"UniformsSymbol lifetime: 3 heaps passed\n";}
}
int main(int argc,char** argv){try{const std::string input(std::istreambuf_iterator<char>(std::cin),{});if(argc==2&&std::string(argv[1])=="--lifetime"){lifetime(input);return 0;}if(argc!=1)throw std::invalid_argument("Only --lifetime or JSON stdin accepted");std::cout<<execute(Animation_detail::parseJSON(input)).toJSON()->dump();}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}}
