#include "nodes/core/StackTrace.h"
#include "core/JSSourceTrace.h"
#include "core/JSError.h"
#include "core/JSGlobal.h"
#include "core/JSString.h"
#include "core/JSIterator.h"
#include "core/JSPromise.h"
#include "core/JSJobs.h"
#include "animation/AnimationUtils.h"
#include "StackTraceProbeLocations.inc"
#include "StackTraceLifetimeInput.inc"
#include <cmath>
#include <iostream>
#include <iterator>
#include <limits>

namespace {
using Args=std::vector<JSValue>;
JSValue array(const Args& values={}){auto out=JSValue::array();for(unsigned i=0;i<values.size();++i)out.set(std::to_string(i),values[i]);return out;}
void push(const JSValue& target,const JSValue& value){target.get("push").call(target,{value});}
JSValue call(const JSValue& self,const char* name,const Args& args={}){return self.get(name).call(self,args);}
JSPropertyDescriptor field(JSValue value){JSPropertyDescriptor d;d.value=value;d.writable=d.configurable=true;return d;}
JSValue arg(const Args& args,unsigned i){return i<args.size()?args[i]:JSValue{};}
JSValue encode(const JSValue& value){
 if(value.isUndefined())return array({"undefined"});
 if(value.type()==JSValue::Type::Number){const auto n=value.number();if(std::isnan(n))return array({"number","NaN"});if(std::isinf(n))return array({"number",n>0?"Infinity":"-Infinity"});if(n==0&&std::signbit(n))return array({"number","-0"});}
 if(!value.isObject())return value;
 auto keys=array(),values=array(),out=JSValue::object();for(const auto& key:value.ownPropertyKeys()){push(keys,key);push(values,encode(value.get(key)));}out.set("kind",value.type()==JSValue::Type::Array?"array":"object");out.set("keys",keys);out.set("values",values);return out;
}
template<class F>JSValue attempt(F callback){JSSourceTrace::FrameScope frame(StackTraceProbeLocations::hostFrame("attempt"));auto out=JSValue::object();try{JSValue value;{JSSourceTrace::PositionScope position(StackTraceProbeLocations::hostPosition("attempt.invoke"));value=callback();}out.set("value",encode(value));}catch(...){const auto e=nativeExceptionValue(std::current_exception());if(isNativeError(e)){out.set("error",e.get("name"));out.set("nativeError",true);out.set("typeError",e.instanceOf(nativeTypeErrorConstructor()));}else out.set("thrown",encode(e));}return out;}
JSValue decode(const JSValue& value){if(!value.isObject()||!value.hasOwn("$tag"))return value;const auto tag=value.get("$tag");if(tag.strictEquals("undefined"))return {};if(tag.strictEquals("-0"))return -0.0;if(tag.strictEquals("NaN"))return std::numeric_limits<double>::quiet_NaN();if(tag.strictEquals("Infinity"))return std::numeric_limits<double>::infinity();return -std::numeric_limits<double>::infinity();}
JSValue inspect(const JSValue& frames){
 auto out=array();for(unsigned i=0;i<frames.length();++i){const auto frame=frames.get(std::to_string(i));auto value=JSValue::object();for(const auto* name:{"getColumnNumber","getEnclosingColumnNumber","getEnclosingLineNumber","getEvalOrigin","getFileName","getFunction","getFunctionName","getLineNumber","getMethodName","getPosition","getPromiseIndex","getScriptNameOrSourceURL","getScriptHash","getThis","getTypeName","isAsync","isConstructor","isEval","isNative","isPromiseAll","isToplevel","toString"})value.set(name,call(frame,name));push(out,value);}return out;
}
// This host namespace represents the actual imported source module. Its brand
// and URL are private data; frame labeling never reads user-controlled getters.
struct ProbeSourceModule { std::string uri; };
std::string uri(const JSValue& module){return std::static_pointer_cast<ProbeSourceModule>(module.hostPayload("probe.SourceModuleNamespace"))->uri;}
JSSourceTrace::Frame mappedFrame(const char* name,const JSValue& self,const JSValue& module){auto frame=StackTraceProbeLocations::frame(name,uri(module));if(self.isHostObject("probe.SourceModuleNamespace"))frame.typeName="Module";return frame;}
JSValue leaf(const JSValue&,const Args& args,const Args& captures){
 const auto module=captures[0],request=arg(args,0),operation=request.get("operation");const auto source=uri(module);JSSourceTrace::FrameScope frame(StackTraceProbeLocations::frame("leaf",source));
 if(operation.strictEquals("throw"))throw JSThrownValue(17);
 if(operation.strictEquals("capture-middle")||operation.strictEquals("capture-absent")){
  auto target=JSValue::object();const bool middle=operation.strictEquals("capture-middle");JSSourceTrace::PositionScope position(StackTraceProbeLocations::position(middle?"capture-middle":"capture-absent",source));
  call(nativeErrorConstructor(),"captureStackTrace",{target,module.get(middle?"middle":"absent")});return target.get("stack");
 }
 if(operation.strictEquals("lazy")){JSSourceTrace::PositionScope position(StackTraceProbeLocations::position("error-lazy",source));return nativeErrorConstructor().construct({"lazy"});}
 if(operation.strictEquals("default-filtered")){JSSourceTrace::PositionScope position(StackTraceProbeLocations::position("default",source));return nativeStackTraceConstructor().construct().get("stack");}
 JSValue error;{JSSourceTrace::PositionScope position(StackTraceProbeLocations::position("error-boom",source));error=nativeErrorConstructor().construct({"boom"});}JSSourceTrace::PositionScope position(StackTraceProbeLocations::position("read-boom-stack",source));return error.get("stack");
}
JSValue middle(const JSValue&,const Args& args,const Args& captures){const auto module=captures[0];const auto source=uri(module);JSSourceTrace::FrameScope frame(StackTraceProbeLocations::frame("middle",source));JSSourceTrace::PositionScope position(StackTraceProbeLocations::position("call-leaf",source));return module.get("leaf").call({},args);}
JSValue outer(const JSValue& self,const Args& args,const Args& captures){const auto module=captures[0];const auto source=uri(module);JSSourceTrace::FrameScope frame(mappedFrame("outer",self,module));JSSourceTrace::PositionScope position(StackTraceProbeLocations::position("call-middle",source));return module.get("middle").call({},args);}
JSValue absent(const JSValue&,const Args&,const Args& captures){JSSourceTrace::FrameScope frame(StackTraceProbeLocations::frame("absent",uri(captures[0])));return {};}
JSValue formatter(const JSValue& self,const Args& args,const Args& captures){
 const auto source=uri(captures[0]);auto metadata=mappedFrame("formatter",self,captures[0]);if(self.sameValue(nativeErrorConstructor())){metadata.typeName="Error";const auto property=self.getOwnPropertyDescriptor("prepareStackTrace");if(property&&property->value&&property->value->sameValue(captures[0].get("formatter")))metadata.methodName="prepareStackTrace";}JSSourceTrace::FrameScope frame(std::move(metadata));JSValue nested;
 {JSSourceTrace::PositionScope position(StackTraceProbeLocations::position("error-nested",source));nested=nativeErrorConstructor().construct({"nested"}).get("stack");}
 auto names=array();const auto frames=arg(args,1);for(unsigned i=0;i<frames.length();++i)push(names,call(frames.get(std::to_string(i)),"getFunctionName"));auto out=JSValue::object();out.set("names",names);out.set("nested",nativeStackTraceConstructor().construct({nested}).get("stack"));return out;
}
JSValue resume(const JSValue&,const Args&,const Args& captures){const auto source=uri(captures[0]);JSSourceTrace::FrameScope frame(StackTraceProbeLocations::frame("resume",source));JSSourceTrace::PositionScope position(StackTraceProbeLocations::position("error-resumed",source));return nativeErrorConstructor().construct({"resumed"}).get("stack");}
JSValue schedule(const JSValue&,const Args&,const Args& captures){const auto module=captures[0];JSSourceTrace::FrameScope frame(StackTraceProbeLocations::frame("schedule",uri(module)));return call(resolveNativePromise(),"then",{module.get("resume")});}
struct SourceAwait {
 JSValue state,target;std::optional<JSPropertyDescriptor> savedPrepare;JSPropertyDescriptor savedLimit;bool restored=false;
 void restore(){if(restored)return;restored=true;if(savedPrepare)target.defineProperty("prepareStackTrace",*savedPrepare);else target.erase("prepareStackTrace");target.defineProperty("stackTraceLimit",savedLimit);}
 ~SourceAwait(){try{restore();}catch(...){}}
};
JSValue source(const JSValue& request,std::shared_ptr<SourceAwait>& pending){
 JSSourceTrace::FrameScope hostFrame(StackTraceProbeLocations::hostFrame("source"));const auto entryDepth=JSSourceTrace::capture(std::numeric_limits<std::size_t>::max()).size();
 const auto error=nativeErrorConstructor();const auto savedPrepare=error.getOwnPropertyDescriptor("prepareStackTrace");const auto savedLimit=*error.getOwnPropertyDescriptor("stackTraceLimit");auto target=error;target.set("stackTraceLimit",3);target.erase("prepareStackTrace");
 auto owner=std::make_shared<ProbeSourceModule>();owner->uri=request.get("sourceURI").string();auto module=JSValue::hostObject(owner,owner.get(),"probe.SourceModuleNamespace");module.setPrototype(nullptr);
 for(const auto& entry:std::initializer_list<std::pair<const char*,JSValue::NativeFunction>>{{"leaf",leaf},{"middle",middle},{"outer",outer},{"absent",absent},{"formatter",formatter},{"resume",resume},{"schedule",schedule}}){auto descriptor=field(JSValue::function(entry.second,{module},entry.first,std::string(entry.first)=="formatter"?2:std::string(entry.first)=="absent"||std::string(entry.first)=="resume"||std::string(entry.first)=="schedule"?0:1));descriptor.enumerable=true;descriptor.configurable=false;module.defineProperty(entry.first,descriptor);}JSPropertyDescriptor tag;tag.value="Module";module.defineProperty(JSValue::wellKnownSymbol("toStringTag"),tag);module.preventExtensions();
 const auto restore=[&](){if(savedPrepare)target.defineProperty("prepareStackTrace",*savedPrepare);else target.erase("prepareStackTrace");target.defineProperty("stackTraceLimit",savedLimit);};
 const auto structured=JSValue::function(+[](const JSValue&,const Args& args,const Args&)->JSValue{return inspect(arg(args,1));}, {}, "",2);const auto operation=request.get("operation");
 try{
  if(operation.strictEquals("structured"))target.set("prepareStackTrace",structured);if(operation.strictEquals("reentry"))target.set("prepareStackTrace",module.get("formatter"));
  JSValue out;
  if(operation.strictEquals("lazy")){JSValue e;{JSSourceTrace::PositionScope position(StackTraceProbeLocations::hostPosition("source.lazy.outer"));e=module.get("outer").call(module,{request});}target.set("prepareStackTrace",structured);out=JSValue::object();out.set("value",encode(e.get("stack")));}
  else if(operation.strictEquals("throw-unwind")){auto firstRequest=JSValue::object();firstRequest.set("operation","throw");out=JSValue::object();{JSSourceTrace::PositionScope position(StackTraceProbeLocations::hostPosition("source.throw-first.attempt"));out.set("first",attempt([&](){JSSourceTrace::FrameScope callback(StackTraceProbeLocations::hostFrame("source.throw-first"));JSSourceTrace::PositionScope position(StackTraceProbeLocations::hostPosition("source.throw-first.outer"));return module.get("outer").call(module,{firstRequest});}));}firstRequest.set("operation","frames");{JSSourceTrace::PositionScope position(StackTraceProbeLocations::hostPosition("source.throw-second.attempt"));out.set("second",attempt([&](){JSSourceTrace::FrameScope callback(StackTraceProbeLocations::hostFrame("source.throw-second"));JSSourceTrace::PositionScope position(StackTraceProbeLocations::hostPosition("source.throw-second.outer"));return module.get("outer").call(module,{firstRequest});}));}}
  else if(operation.strictEquals("microtask")){target.set("stackTraceLimit",1);auto state=JSValue::object();JSValue promise;{JSSourceTrace::PositionScope position(StackTraceProbeLocations::hostPosition("source.microtask.schedule"));promise=module.get("schedule").call(module);}const auto settled=JSValue::function(+[](const JSValue&,const Args& args,const Args& captures)->JSValue{auto state=captures[0];state.set("value",arg(args,0));state.set("done",true);return {};},{state},"",1);call(promise,"then",{settled});pending=std::make_shared<SourceAwait>();pending->state=state;pending->target=target;pending->savedPrepare=savedPrepare;pending->savedLimit=savedLimit;}
  else{JSSourceTrace::PositionScope position(StackTraceProbeLocations::hostPosition("source.default.attempt"));out=attempt([&](){JSSourceTrace::FrameScope callback(StackTraceProbeLocations::hostFrame("source.default"));JSSourceTrace::PositionScope position(StackTraceProbeLocations::hostPosition("source.default.outer"));return module.get("outer").call(module,{request});});}
  if(!pending)restore();if(JSSourceTrace::capture(std::numeric_limits<std::size_t>::max()).size()!=entryDepth)throw std::logic_error("Source frame survived completed/abrupt/queued execution");return out;
 }catch(...){restore();throw;}
}
JSValue splitInput(const JSValue& self,const Args& args,const Args& captures){auto state=captures[0];push(state.get("events"),array({"split",self.sameValue(state.get("input")),arg(args,0)}));if(state.get("operation").strictEquals("input-split-throw"))throw JSThrownValue(19);return call(state.get("text"),"split",args);}
JSValue splitGetter(const JSValue&,const Args&,const Args& captures){push(captures[0].get("events"),array({"get split"}));return JSValue::function(splitInput,captures,"",1);}
JSValue integerHook(const JSValue&,const Args& args,const Args& captures){auto state=captures[0];auto events=state.get("events");push(events,array({"parseInt",arg(args,0),arg(args,1)}));if(state.get("operation").strictEquals("parseInt-throw"))throw JSThrownValue(23);if(state.get("operation").strictEquals("parseInt-reentry")&&events.length()==1){const auto nested=nativeStackTraceConstructor().construct({"at nested (nested.js:7:2)"});push(events,array({"nested",call(nested,"getLocation")}));}return state.get("oldParseInt").call({},args);}
JSValue integerGetter(const JSValue&,const Args&,const Args& captures){push(captures[0].get("events"),array({"get parseInt"}));return JSValue::function(integerHook,captures,"",2);}
JSValue matchHook(const JSValue& self,const Args& args,const Args& captures){auto state=captures[0];push(state.get("events"),array({"match",self.type()==JSValue::Type::String?JSValue("string"):JSValue("object"),self.toString()}));return state.get("oldMatch").call(self,args);}
JSValue replacementError(const JSValue& self,const Args&,const Args& captures){push(captures[0].get("events"),array({"new Error"}));auto target=self;target.set("stack","at replacement (replacement.js:29:8)");return {};}
JSValue stack(const JSValue& request){
 JSSourceTrace::FrameScope frame(StackTraceProbeLocations::hostFrame("stack"));
 auto global=nativeGlobalObject(),stringPrototype=nativeStringPrototype(),state=JSValue::object();const auto savedError=*global.getOwnPropertyDescriptor("Error"),savedInteger=*global.getOwnPropertyDescriptor("parseInt"),savedMatch=*stringPrototype.getOwnPropertyDescriptor("match");
 state.set("events",array());state.set("text",request.get("text"));state.set("operation",request.get("operation"));state.set("oldParseInt",*savedInteger.value);state.set("oldMatch",*savedMatch.value);const auto operation=request.get("operation");auto input=request.get("text");
 const auto restore=[&](){global.defineProperty("Error",savedError);global.defineProperty("parseInt",savedInteger);stringPrototype.defineProperty("match",savedMatch);};
 try{
  if(operation.strictEquals("input-split")||operation.strictEquals("input-split-throw")){input=JSValue::object();state.set("input",input);JSPropertyDescriptor property;property.getter=JSValue::function(splitGetter,{state},"get split",0);property.configurable=property.enumerable=true;input.defineProperty("split",property);}
  if(operation.strictEquals("parseInt-reentry")||operation.strictEquals("parseInt-throw"))global.set("parseInt",JSValue::function(integerHook,{state},"",2));
  if(operation.strictEquals("parseInt-getter-order")){JSPropertyDescriptor property;property.getter=JSValue::function(integerGetter,{state},"get",0);property.configurable=true;global.defineProperty("parseInt",property);}
  if(operation.strictEquals("match-protocol"))stringPrototype.defineProperty("match",field(JSValue::function(matchHook,{state},"value",1)));
  if(operation.strictEquals("global-error")||operation.strictEquals("empty-input-error")){global.set("Error",JSValue::constructor(replacementError,{state},"CustomError",0));input=operation.strictEquals("empty-input-error")?JSValue(""):JSValue(nullptr);}
  auto out=JSValue::object();{JSSourceTrace::PositionScope position(StackTraceProbeLocations::hostPosition("stack.result.attempt"));out.set("result",attempt([&](){JSSourceTrace::FrameScope callback(StackTraceProbeLocations::hostFrame("stack.result"));JSValue trace;{JSSourceTrace::PositionScope position(StackTraceProbeLocations::hostPosition("stack.result.construct"));trace=nativeStackTraceConstructor().construct({input});}auto value=JSValue::object();value.set("stack",trace.get("stack"));{JSSourceTrace::PositionScope position(StackTraceProbeLocations::hostPosition("stack.result.getLocation"));value.set("location",call(trace,"getLocation"));}{JSSourceTrace::PositionScope position(StackTraceProbeLocations::hostPosition("stack.result.getError"));value.set("error",call(trace,"getError",{"message"}));}return value;}));}out.set("events",encode(state.get("events")));restore();return out;
 }catch(...){restore();throw;}
}
JSValue coercion(const JSValue&,const Args& args,const Args& captures){auto state=captures[0];const bool string=captures[1].strictEquals("string");push(state.get("events"),array({captures[1],arg(args,0)}));if(state.get("operation").strictEquals(string?"string-throw":"radix-throw"))throw JSThrownValue(string?31:37);return string?JSValue("  -0x1f trailing"):JSValue(16);}
JSValue integer(const JSValue& request){
 JSSourceTrace::FrameScope frame(StackTraceProbeLocations::hostFrame("integer"));
 const auto parse=nativeGlobalBinding("parseInt"),operation=request.get("operation");
 if(operation.strictEquals("metadata")){auto value=JSValue::object();value.set("name",parse.get("name"));value.set("length",parse.get("length"));auto keys=array();for(const auto& key:parse.ownPropertyKeys())push(keys,key);value.set("keys",keys);value.set("constructible",attempt([&](){return parse.construct();}));value.set("global",*nativeGlobalObject().getOwnPropertyDescriptor("parseInt")->enumerable);value.set("number",parse.sameValue(nativeGlobalBinding("Number").get("parseInt")));auto out=JSValue::object();out.set("value",encode(value));return out;}
 if(operation.strictEquals("alias")){auto out=JSValue::object();out.set("value",encode(parse.sameValue(nativeGlobalBinding("Number").get("parseInt"))));return out;}
 auto state=JSValue::object();state.set("events",array());state.set("operation",operation);auto value=decode(request.get("value")),radix=decode(request.get("radix"));
 if(operation.strictEquals("coercion-order")||operation.strictEquals("string-throw")||operation.strictEquals("radix-throw")){value=JSValue::object();value.setPrimitiveConverter(JSValue::function(coercion,{state,"string"},"[Symbol.toPrimitive]",1));radix=JSValue::object();radix.setPrimitiveConverter(JSValue::function(coercion,{state,"radix"},"[Symbol.toPrimitive]",1));}
 if(operation.strictEquals("symbol-input"))value=JSValue::symbol("input");if(operation.strictEquals("symbol-radix")){value="17";radix=JSValue::symbol("radix");}
 auto out=JSValue::object();out.set("result",attempt([&](){return parse.call({}, {value,radix});}));out.set("events",encode(state.get("events")));return out;
}
JSValue executeHost(const JSValue& requests){auto out=array();for(unsigned i=0;i<requests.length();++i){const auto request=requests.get(std::to_string(i));auto row=JSValue::object();const auto kind=request.get("kind");row.set("kind",kind);row.set("operation",request.hasOwn("operation")?request.get("operation"):JSValue(nullptr));JSValue value;std::shared_ptr<SourceAwait> pending;
 {JSSourceTrace::FrameScope moduleFrame(StackTraceProbeLocations::hostFrame("comparator.module"));JSSourceTrace::PositionScope modulePosition(StackTraceProbeLocations::hostPosition("comparator.oracle"));JSSourceTrace::FrameScope oracleFrame(StackTraceProbeLocations::hostFrame("oracle"));JSSourceTrace::PositionScope position(StackTraceProbeLocations::hostPosition(kind.strictEquals("source")?"oracle.source":kind.strictEquals("stack")?"oracle.stack":"oracle.integer"));value=kind.strictEquals("source")?source(request,pending):kind.strictEquals("stack")?stack(request):integer(request);}
 if(pending){if(!JSSourceTrace::capture().empty())throw std::logic_error("Synchronous source frames survived await suspension");drainMicrotasks();if(!pending->state.get("done").toBoolean())throw std::logic_error("Mapped promise continuation did not execute");{JSSourceTrace::FrameScope resumed(StackTraceProbeLocations::hostFrame("source"));value=JSValue::object();value.set("value",encode(pending->state.get("value")));pending->restore();}pending.reset();}
 row.set("value",value);push(out,row);endJSJob();requests.collectGarbage();}return out;}
}
int main(int argc,char** argv){try{if(argc==2&&std::string(argv[1])=="--lifetime"){for(unsigned i=0;i<3;++i){{const auto requests=Animation_detail::parseJSON(stackTraceLifetimeInput());if(requests.length()!=570)throw std::logic_error("Original lifetime suite must contain 570 cases");const auto results=executeHost(requests);if(results.length()!=requests.length())throw std::logic_error("Lifetime execution omitted cases");}if(!JSSourceTrace::capture().empty())throw std::logic_error("Host source frame survived a lifetime cycle");endJSJob();JSValue::nativeSafepoint();if(JSValue::hasActiveHeap())throw std::logic_error("Source trace/CallSite/provider graph retained a retired heap");}std::cout<<"StackTraceSource lifetime: 3 heaps passed\n";return 0;}if(argc!=1)throw std::invalid_argument("JSON stdin or --lifetime required");const std::string input(std::istreambuf_iterator<char>(std::cin),{});std::cout<<executeHost(Animation_detail::parseJSON(input)).toJSON()->dump();return 0;}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}}
