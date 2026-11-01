#include "animation/AnimationUtils.h"
#include "core/JSGlobal.h"
#include "core/JSIterator.h"
#include "core/JSJobs.h"
#include "core/JSPromise.h"
#include "core/JSProxy.h"
#include "nodes/display/ColorAdjustment.h"
#include "math/ColorManagement.h"
#include "nodes/core/NodeBuilder.h"
#include "nodes/core/StackNode.h"
#include "nodes/tsl/TSLBase.h"
#include "utils.h"
#include <cmath>
#include <cstdint>
#include <iostream>
#include <limits>
#include <string_view>

namespace {
using Args=std::vector<JSValue>;
JSValue arg(const Args& a,std::size_t i){return i<a.size()?a[i]:JSValue{};}
void own(JSValue o,const JSValue& key,const JSValue& value){JSPropertyDescriptor d;d.value=value;d.writable=d.enumerable=d.configurable=true;o.defineProperty(key,d);}
JSValue array(const Args& a={}){auto out=JSValue::array();for(const auto& value:a)own(out,JSValue(std::to_string(out.length())),value);return out;}
void push(JSValue a,const JSValue& value){own(a,JSValue(std::to_string(a.length())),value);}
JSValue call(const JSValue& object,const char* key,const Args& args={}){return object.get(key).call(object,args);}
Args values(const JSValue& a){Args out;for(unsigned i=0;i<a.length();++i)out.push_back(a.get(std::to_string(i)));return out;}
JSValue randomState,functions;
void initialize(){
 randomState=JSValue::object();randomState.set("seed",305419896.0);randomState.set("calls",0);
 auto math=nativeGlobalBinding("Math");math.set("random",JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue{
  auto state=c[0];const auto seed=std::uint32_t(state.get("seed").number());const auto next=std::uint32_t(seed*std::uint32_t(1664525)+std::uint32_t(1013904223));state.set("seed",double(next));state.set("calls",state.get("calls").number()+1);return double(next)/4294967296.0;
 },{randomState},"random",0));
 nativeColorAdjustmentModule();nativeTSLBase();nativeNodeBuilderConstructor();nativeStackNodeConstructor();nativeColorManagement();
 functions=JSValue::object();for(const auto& key:nativeColorAdjustmentModule().keys())functions.set(key,nativeColorAdjustmentBinding(key));
}
JSValue function(const std::string& name){return nativeColorAdjustmentBinding(name);}

JSValue token(const JSValue& value){
 if(value.isUndefined())return array({"undefined"});
 if(value.isCallable())return array({"function",value.get("name"),value.get("length"),value.nativeFunctionSource()});
 if(value.type()==JSValue::Type::Number){const auto n=value.number();if(std::isnan(n))return array({"number","NaN"});if(std::isinf(n))return array({"number",n<0?"-Infinity":"Infinity"});if(n==0&&std::signbit(n))return array({"number","-0"});}
 if(value.isSymbol()){const auto d=value.symbolDescription();return array({"symbol",d?JSValue(*d):JSValue(nullptr)});}return value;
}
struct Graph {
 JSValue objects=JSValue::array();std::vector<JSValue> seen;
 JSValue encode(const JSValue& value){
  if(value.isCallable())return array({"function",token(value.get("name")),token(value.get("length")),value.nativeFunctionSource()});
  if(!value.isObject())return token(value);
  for(std::size_t i=0;i<seen.size();++i)if(value.sameValue(seen[i]))return array({"ref",double(i)});
  const auto index=seen.size();seen.push_back(value);auto row=JSValue::object();const bool isArray=isNativeArray(value);row.set("kind",isArray?"array":"object");row.set("properties",array());push(objects,row);
  if(value.get("isNode").strictEquals(true)){row.set("type",value.get("type"));row.set("id",value.get("id"));row.set("uuid",value.get("uuid"));}
  if(isArray)row.set("length",value.get("length"));
  for(const auto& key:value.ownPropertyKeys()){
   const auto d=value.getOwnPropertyDescriptor(key);if(!d)throw std::logic_error("Snapshot own key lost its descriptor");
   const auto property=d->value?encode(*d->value):array({"accessor",d->getter&&!d->getter->isUndefined(),d->setter&&!d->setter->isUndefined()});
   push(row.get("properties"),array({key.isSymbol()?token(key):key,property,d->enumerable.value_or(false),d->configurable.value_or(false),d->writable?JSValue(*d->writable):JSValue(nullptr)}));
  }
  return array({"ref",double(index)});
 }
 JSValue snapshot(const JSValue& value){auto out=JSValue::object();out.set("root",encode(value));out.set("objects",objects);return out;}
};
JSValue graph(const JSValue& value){return Graph{}.snapshot(value);}
JSValue decode(const JSValue& value){
 if(!value.isObject())return value;
 if(value.hasOwn("$undefined"))return {};
 if(value.hasOwn("$symbol"))return JSValue::symbol(value.get("$symbol").string());
 if(value.hasOwn("$number")){const auto s=value.get("$number").string();if(s=="NaN")return std::numeric_limits<double>::quiet_NaN();if(s=="Infinity")return std::numeric_limits<double>::infinity();if(s=="-Infinity")return -std::numeric_limits<double>::infinity();if(s=="-0")return -0.0;return {};}
 if(value.hasOwn("node")){Args a;for(const auto& v:values(value.get("args")))a.push_back(decode(v));return nativeTSLBaseBinding(value.get("node").string()).call({},a);}
 auto out=isNativeArray(value)?array():JSValue::object();for(const auto& key:value.keys())own(out,key,decode(value.get(key)));return out;
}
JSValue currentError(const JSValue& reason,bool build=false){
 const auto value=nativeExceptionValue(std::current_exception());auto out=JSValue::object();out.set("name",!build&&value.sameValue(reason)?JSValue(nullptr):value.get("name"));out.set("thrownOriginal",value.sameValue(reason));return out;
}
JSValue metadata(){
 auto result=JSValue::object();for(const auto* name:{"grayscale","saturation","vibrance","hue","luminance","cdl","posterize"}){
  const auto fn=function(name),callback=std::string_view(name)=="luminance"?fn:fn.get("shaderNode").get("jsFunc");auto row=JSValue::object();
  row.set("name",token(fn.get("name")));row.set("length",token(fn.get("length")));row.set("source",fn.nativeFunctionSource());
  auto keys=array();for(const auto& key:fn.ownPropertyKeys())push(keys,token(key));row.set("ownKeys",keys);
  auto info=JSValue::object();info.set("name",callback.get("name"));info.set("length",callback.get("length"));info.set("source",callback.nativeFunctionSource());
  bool constructible=false;try{auto dummy=JSValue::constructor(+[](const JSValue& self,const Args&,const Args&)->JSValue{return self;});dummy.construct({},callback);constructible=true;}catch(...){}
  info.set("constructible",constructible);row.set("callback",info);row.set("isFn",token(fn.get("isFn")));row.set("layout",std::string_view(name)=="luminance"?JSValue(nullptr):graph(fn.get("shaderNode").get("layout")));result.set(name,row);
 }return result;
}
JSValue rawInputs(const JSValue& args,const JSValue& kind,const JSValue& events,const JSValue& reason){
 if(!kind.toBoolean())return args;auto state=JSValue::object();state.set("index",0);auto iterator=JSValue::object();
 iterator.set("next",JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue{
  push(c[2],"next");if(c[1].strictEquals("nextThrow"))throw JSThrownValue(c[4]);auto result=JSValue::object();JSPropertyDescriptor done;done.enumerable=done.configurable=true;
  done.getter=JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue{push(c[2],"done");if(c[1].strictEquals("doneThrow"))throw JSThrownValue(c[4]);return c[1].strictEquals("empty")||c[3].get("index").number()>=c[0].length();},c);result.defineProperty("done",done);
  JSPropertyDescriptor value;value.enumerable=value.configurable=true;value.getter=JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue{push(c[2],"value");if(c[1].strictEquals("valueThrow"))throw JSThrownValue(c[4]);auto state=c[3];const auto index=state.get("index").number();state.set("index",index+1);return c[0].get(JSValue(index));},c);result.defineProperty("value",value);return result;
 },{args,kind,events,state,reason},"next",0));
 JSPropertyDescriptor ret;ret.enumerable=ret.configurable=true;ret.getter=JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue{
  push(c[2],"get:return");if(c[1].strictEquals("returnGetterThrow"))throw JSThrownValue(c[4]);return JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue{push(c[2],"return");if(c[1].strictEquals("returnThrow"))throw JSThrownValue(c[4]);return c[1].strictEquals("returnPrimitive")?JSValue(17):JSValue::object();},c,"",0);
 },{args,kind,events,state,reason},"get return",0);iterator.defineProperty("return",ret);
 auto out=JSValue::object();out.set(JSValue::wellKnownSymbol("iterator"),JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue{push(c[2],"iterator");if(c[1].strictEquals("iteratorThrow"))throw JSThrownValue(c[3]);return c[1].strictEquals("primitiveIterator")?JSValue(17):c[0];},{iterator,kind,events,reason},"[Symbol.iterator]",0));return out;
}
JSValue reenter(){return nativeColorAdjustmentBinding("luminance").call({}, {nativeTSLBaseBinding("vec3").call({}, {.12,.23,.34}),nativeTSLBaseBinding("vec3").call({}, {.2,.3,.5})});}
JSValue run(const JSValue& request){
 if(request.get("mode").strictEquals("metadata")){auto out=JSValue::object();out.set("metadata",metadata());out.set("randomCalls",randomState.get("calls"));return out;}
 if(request.get("mode").strictEquals("bindings")){auto out=JSValue::object();auto bindings=array();for(const auto& key:nativeColorAdjustmentModule().keys())push(bindings,array({key,nativeColorAdjustmentBinding(key).sameValue(nativeColorAdjustmentBinding(key))}));out.set("exports",bindings);out.set("randomCalls",randomState.get("calls"));return out;}
 const auto events=array(),messages=array(),previous=nativeTSLBaseBinding("getCurrentStack").call(),stack=nativeStackNodeConstructor().construct();auto reason=JSValue::object();reason.set("reason","original");nativeTSLBaseBinding("setCurrentStack").call({}, {stack});
 ThreeUtils::setConsoleFunction(JSValue::function(+[](const JSValue&,const Args& a,const Args& c)->JSValue{push(c[0],array(a));return {};},{messages},"",0));JSValue result,changedTarget,changedKey;std::optional<JSPropertyDescriptor> savedCM,savedTarget;
 try{
  Args args;for(const auto& value:values(request.get("args")))args.push_back(decode(value));
  if(request.get("cm").toBoolean()){
   auto cm=nativeColorManagement();savedCM=cm.getOwnPropertyDescriptor("getLuminanceCoefficients");const auto method=cm.get("getLuminanceCoefficients");JSPropertyDescriptor d;d.configurable=true;
   d.getter=JSValue::function(+[](const JSValue& self,const Args&,const Args& c)->JSValue{push(c[2],array({"cm:get",self.sameValue(c[0])}));self.collectGarbage();if(c[3].strictEquals("getThrow"))throw JSThrownValue(c[4]);return JSValue::function(+[](const JSValue& self,const Args& a,const Args& c)->JSValue{push(c[2],array({"cm:call",self.sameValue(c[0]),double(a.size()),a.at(0).get("isVector3"),a.at(0).get("x"),a.at(0).get("y"),a.at(0).get("z"),token(arg(a,1))}));self.collectGarbage();if(c[3].strictEquals("callThrow"))throw JSThrownValue(c[4]);if(c[3].strictEquals("reenter"))push(c[2],array({"cm:reenter",reenter().get("isNode")}));return c[1].call(self,a);},c,"",0);},{cm,method,events,request.get("cm"),reason},"get",0);cm.defineProperty("getLuminanceCoefficients",d);
  }
  const auto info=request.get("instrument");if(info.toBoolean()){
   changedTarget=args.at(std::size_t(info.get("argument").number()));changedKey=info.get("key");const auto method=changedTarget.get(changedKey);savedTarget=changedTarget.getOwnPropertyDescriptor(changedKey);JSPropertyDescriptor d;d.configurable=true;
   d.getter=JSValue::function(+[](const JSValue& self,const Args&,const Args& c)->JSValue{push(c[3],array({"get",c[1],self.sameValue(c[0])}));self.collectGarbage();if(c[4].strictEquals("getThrow"))throw JSThrownValue(c[5]);if(c[4].strictEquals("reenter"))push(c[3],array({"reenter",reenter().get("isNode")}));return c[2];},{changedTarget,changedKey,method,events,info.get("mode"),reason},"get",0);changedTarget.defineProperty(changedKey,d);
  }
  const auto fn=function(request.get("fn").string());const auto value=request.get("mode").strictEquals("body")&&!request.get("fn").strictEquals("luminance")?call(fn.get("shaderNode"),"jsFunc",{rawInputs(array(args),request.get("iterator"),events,reason)}):fn.call({},args);
  result=JSValue::object();result.set("value",graph(value));result.set("stack",graph(stack.get("nodes")));
  if(request.get("build").toBoolean()){
   auto diagnostics=JSValue::object();diagnostics.set("keywords",false);auto debug=JSValue::object();debug.set("diagnostics",diagnostics);auto renderer=JSValue::object();renderer.set("debug",debug);
   auto builder=nativeNodeBuilderConstructor().construct({nullptr,renderer,nullptr});builder.set("shaderStage","fragment");auto mutableStack=stack;mutableStack.set("outputNode",value);
   try{call(builder,"flowBuildStage",{stack,"setup"});result.set("nodeType",call(value,"getNodeType",{builder}));result.set("flow",graph(call(builder,"flowStagesNode",{stack})));}catch(...){result.set("buildError",currentError(reason,true));}
   auto info=JSValue::object();info.set("shaderStage",builder.get("shaderStage"));info.set("buildStage",builder.get("buildStage"));info.set("stackIdentity",builder.get("stack").sameValue(stack));result.set("builder",info);
  }
 }catch(...){result=JSValue::object();result.set("error",currentError(reason));result.set("stack",graph(stack.get("nodes")));}
 if(savedCM){auto cm=nativeColorManagement();cm.defineProperty("getLuminanceCoefficients",*savedCM);}if(changedTarget.isObject()){if(savedTarget)changedTarget.defineProperty(changedKey,*savedTarget);else changedTarget.erase(changedKey);}
 nativeTSLBaseBinding("setCurrentStack").call({}, {previous});ThreeUtils::setConsoleFunction({});result.set("events",events);result.set("messages",graph(messages));result.set("randomCalls",randomState.get("calls"));return result;
}
void collect(const JSValue& keeper){for(unsigned i=0;i<8;++i){endJSJob();keeper.collectGarbage();}}
JSValue lifetime(){
 auto keeper=nativeColorAdjustmentModule(),out=JSValue::object(),observations=array();out.set("rounds",3);out.set("observations",observations);
 for(int round=0;round<3;++round)for(const auto* name:{"grayscale","saturation","vibrance","hue","luminance","cdl","posterize"}){
  JSValue held;JSWeakValue weak;{
   const auto previous=nativeTSLBaseBinding("getCurrentStack").call(),stack=nativeStackNodeConstructor().construct();nativeTSLBaseBinding("setCurrentStack").call({}, {stack});
   const auto color=nativeTSLBaseBinding("vec4").call({}, {.371+round*.013,.241,.173,.87}),steps=nativeTSLBaseBinding("float").call({}, {3.179});
   held=std::string_view(name)=="luminance"?function(name).call({}, {color}):call(function(name).get("shaderNode"),"jsFunc",{std::string_view(name)=="posterize"?array({color,steps}):array({color})});weak=JSWeakValue(color);nativeTSLBaseBinding("setCurrentStack").call({}, {previous});
  }
  collect(keeper);if(weak.expired())throw std::logic_error(std::string("Actual ")+name+" graph lost its input");auto alive=JSValue::object();alive.set("round",round);alive.set("name",name);alive.set("phase","held");alive.set("alive",true);push(observations,alive);held={};
  collect(keeper);if(!weak.expired())throw std::logic_error(std::string("Dropped ")+name+" graph retained its input");auto dropped=JSValue::object();dropped.set("round",round);dropped.set("name",name);dropped.set("phase","dropped");dropped.set("gone",true);push(observations,dropped);
 }return out;
}
JsonValue heapLifetime(){for(unsigned i=0;i<3;++i){JSWeakValue weak;{auto keeper=JSValue::object();const auto module=nativeColorAdjustmentModule();weak=JSWeakValue(module);if(!nativeColorAdjustmentBinding("grayscale").isCallable()||!nativeColorAdjustmentBinding("luminance").isCallable())throw std::logic_error("Real color adjustment export missing");keeper.collectGarbage();}endJSJob();JSValue::nativeSafepoint();if(!weak.expired()||JSValue::hasActiveHeap())throw std::logic_error("Color adjustment module retained retired heap");}return JsonValue::Object{{"heaps",3},{"retired",3}};}
}
int main(int argc,char** argv){try{
 if(argc==2&&std::string_view(argv[1])=="--heap-lifetime"){std::cout<<heapLifetime().dump();return 0;}
 initialize();if(argc==2&&std::string_view(argv[1])=="--lifetime"){std::cout<<lifetime().toJSON()->dump();functions={};randomState={};return 0;}
 if(argc!=1)throw std::logic_error("Usage: probe [--lifetime|--heap-lifetime]");const std::string input((std::istreambuf_iterator<char>(std::cin)),{});const auto requests=Animation_detail::parseJSON(input);const auto runJob=JSValue::function(+[](const JSValue&,const Args& args,const Args&)->JSValue{return run(args.at(0));});std::cout<<'[';bool first=true;for(const auto& request:values(requests)){{const auto result=runJob.call({}, {request});if(!first)std::cout<<',';first=false;std::cout<<result.toJSON()->dump();}endJSJob();requests.collectGarbage();}std::cout<<']';functions={};randomState={};
 }catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}}
