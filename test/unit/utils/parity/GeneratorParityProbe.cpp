#include "animation/AnimationUtils.h"
#include "core/JSGenerator.h"
#include "core/JSPromise.h"
#include "core/JSFunction.h"
#include "core/JSIterator.h"
#include "core/JSProxy.h"
#include <cmath>
#include <iostream>
#include <iterator>
#include <limits>
#include <string>

namespace {
using Args=std::vector<JSValue>;
JSValue argument(const Args& args,std::size_t i){return i<args.size()?args[i]:JSValue{};}
JSPropertyDescriptor descriptor(JSValue value,bool writable=true,bool enumerable=true,bool configurable=true){JSPropertyDescriptor d;d.value=std::move(value);d.writable=writable;d.enumerable=enumerable;d.configurable=configurable;return d;}
void own(JSValue object,const std::string& key,const JSValue& value){object.defineProperty(key,descriptor(value));}
JSValue array(const Args& values={}){auto result=JSValue::array();for(const auto& value:values)result.defineProperty(std::to_string(result.length()),descriptor(value));return result;}
void push(JSValue array,const JSValue& value){array.defineProperty(std::to_string(array.length()),descriptor(value));}
JSValue record(std::initializer_list<std::pair<std::string,JSValue>> values){auto result=JSValue::object();for(const auto& [key,value]:values)own(result,key,value);return result;}
JSValue normal(const JSValue& value){if(value.isUndefined())return "#undefined";if(value.isSymbol())return "#symbol";if(value.isCallable())return "#function";if(value.isObject())return "#object";if(value.type()==JSValue::Type::Number){if(std::isnan(value.number()))return "#NaN";if(value.number()==0&&std::signbit(value.number()))return "#-0";}return value;}
JSValue decode(const JSValue& input){if(!input.isObject())return input;const auto tag=input.get("tag");if(tag.strictEquals("undefined"))return {};if(tag.strictEquals("object"))return record({{"marker",7}});if(tag.strictEquals("symbol"))return JSValue::symbol("fixture");if(tag.strictEquals("NaN"))return std::numeric_limits<double>::quiet_NaN();if(tag.strictEquals("-0"))return -0.0;return input;}
JSValue ownMetadata(const JSValue& object){auto out=array();for(const auto& key:object.ownPropertyKeys()){const auto d=*object.getOwnPropertyDescriptor(key);push(out,array({key.isSymbol()?JSValue("Symbol("+key.symbolDescription().value_or("")+")"):key,normal(d.value.value_or(JSValue{})),d.writable.value_or(false),d.enumerable.value_or(false),d.configurable.value_or(false)}));}return out;}
JSValue resultMetadata(const JSValue& result){return record({{"kind","result"},{"value",normal(result.get("value"))},{"done",result.get("done")},{"own",ownMetadata(result)}});}
template<class F>JSValue attempt(F fn){try{return fn();}catch(const JSGeneratorThrown& e){return record({{"kind","throw"},{"value",normal(e.value())}});}catch(const JSTypeError&){return record({{"kind","throw"},{"value","#TypeError"}});}catch(const JSRangeError&){return record({{"kind","throw"},{"value","#RangeError"}});}}
JSGeneratorStep passAbrupt(const JSGeneratorCompletion& c){return c.kind==JSGeneratorCompletionKind::Throw?JSGeneratorStep::abrupt(c.value):JSGeneratorStep::complete(c.value);}
unsigned pc(const JSValue& state){const auto value=state.get("pc");return value.isUndefined()?0:static_cast<unsigned>(value.number());}
void pc(JSValue state,unsigned n){own(state,"pc",n);}
JSGeneratorStep components(const JSValue& state,const JSGeneratorCompletion& c,const JSValue& receiver,const Args&,const Args& captures){
 if(c.kind!=JSGeneratorCompletionKind::Normal)return passAbrupt(c);
 const char* fields[]={"_x","_y","_z",captures[1].get("component").strictEquals("Euler")?"_order":"_w"};
 const auto i=pc(state);pc(state,i+1);if(i<4)return JSGeneratorStep::yield(receiver.get(fields[i]));return JSGeneratorStep::complete();
}
JSGeneratorStep linear(const JSValue& state,const JSGeneratorCompletion& c,const JSValue& receiver,const Args& args,const Args& captures){
 if(c.kind!=JSGeneratorCompletionKind::Normal)return passAbrupt(c);
 const auto i=pc(state);pc(state,i+1);
 if(i==0){push(captures[0],"start");return JSGeneratorStep::yield(receiver.get("first"));}
 if(i==1){push(captures[0],array({"resume",normal(c.value)}));return JSGeneratorStep::yield(argument(args,0));}
 return JSGeneratorStep::complete(c.value);
}
JSGeneratorStep controlled(const JSValue& state,const JSGeneratorCompletion& c,const JSValue& receiver,const Args&,const Args& captures){
 const auto i=pc(state);const auto events=captures[0];
 if(i==0){push(events,"start");pc(state,1);return JSGeneratorStep::yield(receiver.get("first"));}
 if(i==1&&c.kind==JSGeneratorCompletionKind::Throw){push(events,array({"catch",normal(c.value)}));pc(state,2);return JSGeneratorStep::yield(c.value);}
 if(i==1||i==2){
  if(i==1&&c.kind==JSGeneratorCompletionKind::Normal)push(events,array({"body",normal(c.value)}));
  own(state,"pendingKind",static_cast<unsigned>(c.kind));own(state,"pendingValue",c.value);
  push(events,"finally");pc(state,3);return JSGeneratorStep::yield(receiver.get("cleanup"));
 }
 if(c.kind!=JSGeneratorCompletionKind::Normal)return passAbrupt(c);
 push(events,"afterFinally");
 const auto kind=static_cast<JSGeneratorCompletionKind>(static_cast<unsigned>(state.get("pendingKind").number()));
 if(kind!=JSGeneratorCompletionKind::Normal)return passAbrupt({kind,state.get("pendingValue")});
 return JSGeneratorStep::complete(receiver.get("last"));
}
void initialize(const JSValue& state,const JSValue&,const Args& args,const Args& captures){
 if(argument(args,0).isUndefined()){
  push(captures[0],"initialize");
  if(captures[1].get("initThrows").toBoolean())throw JSGeneratorThrown(decode(captures[1].get("initValue")));
  own(state,"seed",decode(captures[1].get("initValue")));
 }else own(state,"seed",args[0]);
}
JSGeneratorStep initialized(const JSValue& state,const JSGeneratorCompletion& c,const JSValue&,const Args&,const Args& captures){
 if(c.kind!=JSGeneratorCompletionKind::Normal)return passAbrupt(c);
 const auto i=pc(state);pc(state,i+1);if(!i){push(captures[0],"body");return JSGeneratorStep::yield(state.get("seed"));}return JSGeneratorStep::complete(c.value);
}
JSValue metadata(){
 const auto ctor=nativeGeneratorFunctionConstructor(),fp=nativeGeneratorFunctionPrototype(),gp=nativeGeneratorPrototype();
 const auto f=createNativeGeneratorFunction(linear,{},"sample",2,"function* sample(a,b){yield a;return b;}");
 const auto g=f.call();auto methods=array();for(const auto name:{"next","return","throw"}){const auto method=gp.get(name);push(methods,record({{"name",method.get("name")},{"length",method.get("length")},{"source",method.nativeFunctionSource()},{"constructible",method.isConstructible()},{"own",ownMetadata(method)}}));}
 return record({{"constructor",ownMetadata(ctor)},{"functionPrototype",ownMetadata(fp)},{"generatorPrototype",ownMetadata(gp)},{"function",ownMetadata(f)},{"instancePrototype",ownMetadata(f.get("prototype"))},{"instance",ownMetadata(g)},{"methods",methods},{"constructorSource",ctor.nativeFunctionSource()},{"source",f.nativeFunctionSource()},{"constructorConstructible",ctor.isConstructible()},{"functionConstructible",f.isConstructible()},{"functionPrototypeCallable",fp.isCallable()},{"generatorPrototypeCallable",gp.isCallable()},{"functionParent",f.getPrototype().sameValue(fp)},{"functionPrototypeParent",fp.getPrototype().sameValue(nativeFunctionConstructor().get("prototype"))},{"constructorParent",ctor.getPrototype().sameValue(nativeFunctionConstructor())},{"generatorPrototypeParent",gp.getPrototype().sameValue(nativeIteratorPrototype())},{"instanceParent",g.getPrototype().sameValue(f.get("prototype"))},{"functionPrototypeInstanceParent",f.get("prototype").getPrototype().sameValue(gp)},{"iteratorSelf",g.get(JSValue::wellKnownSymbol("iterator")).call(g).sameValue(g)},{"instanceOf",g.instanceOf(f)},{"functionTag",JSValue::intrinsicObjectConstructor().get("prototype").get("toString").call(f)},{"generatorTag",JSValue::intrinsicObjectConstructor().get("prototype").get("toString").call(g)}});
}
JSValue makeReceiver(const JSValue& fixture,const JSValue& events,JSValue store){
 const auto type=fixture.get("receiver");if(type.strictEquals("undefined"))return {};if(type.strictEquals("null"))return nullptr;if(type.strictEquals("number"))return 7;
 auto receiver=JSValue::object();
 for(const auto key:{"first","cleanup","last","_x","_y","_z","_w","_order"}){
  const auto input=fixture.get("values").get(key);own(store,key,input.isUndefined()?JSValue(key):decode(input));
  JSPropertyDescriptor d;d.configurable=true;d.enumerable=true;
  d.getter=JSValue::function(+[](const JSValue& self,const Args&,const Args& c)->JSValue{
   const auto fixture=c[0],events=c[1],store=c[2],key=c[3];
   if(fixture.get("observe").toBoolean())push(events,array({"get",key}));
   if(key.sameValue(fixture.get("throwKey")))throw JSGeneratorThrown(decode(fixture.get("thrown")));
   if(key.sameValue(fixture.get("reentryKey"))){
    const auto generator=self.get("generator");
    const auto entry=attempt([&]()->JSValue{return resultMetadata(nativeGeneratorPrototype().get(fixture.get("reentryMethod")).call(generator,{123}));});
    push(events,array({"reentry",entry}));
    if(fixture.get("uncaughtReentry").toBoolean())nativeGeneratorPrototype().get(fixture.get("reentryMethod")).call(generator,{123});
   }
   if(fixture.get("gc").toBoolean())store.collectGarbage();
   return store.get(key);
  },{fixture,events,store,key},"get "+std::string(key),0);
  receiver.defineProperty(key,d);
 }
 return receiver;
}
// Fixture adapter for genuine native generator and Promise reaction protocols.
void interopInitialize(const JSValue& locals,const JSValue&,const Args& args,const Args& captures){
 const auto context=captures[0];
 if(argument(args,0).isUndefined()){
  push(context.get("events"),"initialize");
  if(context.get("fixture").get("initThrows").toBoolean())throw JSGeneratorThrown(context.get("reason"));
  own(locals,"seed",41);
 }else own(locals,"seed",args[0]);
}
JSGeneratorStep interopBody(const JSValue& locals,const JSGeneratorCompletion& completion,const JSValue& receiver,const Args& args,const Args& captures){
 if(completion.kind!=JSGeneratorCompletionKind::Normal)return passAbrupt(completion);
 const auto context=captures[0],events=context.get("events"),stage=context.get("fixture").get("stage");
 const auto i=pc(locals);pc(locals,i+1);
 if(i==0){
  const auto seed=locals.hasOwn("seed")?locals.get("seed"):argument(args,0);
  push(events,array({"body",normal(seed)}));
  if(stage.strictEquals("body-abrupt"))return JSGeneratorStep::abrupt(context.get("reason"));
  if(stage.strictEquals("getter-abrupt"))return JSGeneratorStep::complete(receiver.get("reason"));
  return JSGeneratorStep::yield(17);
 }
 push(events,"resumed");return JSGeneratorStep::complete(31);
}
JSValue interopType(const JSValue& value){
 switch(value.type()){
  case JSValue::Type::Undefined:return "undefined";
  case JSValue::Type::Boolean:return "boolean";
  case JSValue::Type::Number:return "number";
  case JSValue::Type::String:return "string";
  default:return value.isSymbol()?JSValue("symbol"):JSValue("object");
 }
}
JSValue promiseInterop(const JSValue& fixture){
 const auto reason=decode(fixture.get("reason"));auto events=array(),setup=array();
 auto context=record({{"fixture",fixture},{"reason",reason},{"events",events},{"caught",false},{"settled",false}});
 auto receiver=JSValue::object();JSPropertyDescriptor getter;getter.configurable=true;
 getter.getter=JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue{push(c[0].get("events"),"getter");throw JSGeneratorThrown(c[0].get("reason"));},{context},"get reason",0);receiver.defineProperty("reason",getter);
 const auto initialized=fixture.get("initialized").toBoolean();
 const auto function=createNativeGeneratorFunction(interopBody,{context},"fixture",initialized?0:1,"",initialized?interopInitialize:nullptr);
 own(context,"function",function);own(context,"receiver",receiver);
 const auto stage=fixture.get("stage");
 if(!stage.strictEquals("initializer-abrupt")){
  auto generator=function.call(receiver,fixture.get("explicit").toBoolean()?Args{77}:Args{});own(context,"generator",generator);
  if(stage.strictEquals("suspended-yield")||stage.strictEquals("completed"))push(setup,resultMetadata(generator.get("next").call(generator)));
  if(stage.strictEquals("completed"))push(setup,resultMetadata(generator.get("next").call(generator)));
 }
 push(events,"sync.before");const auto constructor=nativePromiseConstructor();
 const auto resolved=constructor.get("resolve").call(constructor);
 const auto reaction=JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue{
  const auto context=c[0];push(context.get("events"),"reaction");const auto stage=context.get("fixture").get("stage");
  if(stage.strictEquals("initializer-abrupt")){const auto g=context.get("function").call(context.get("receiver"));own(context,"generator",g);return g;}
  const auto g=context.get("generator");
  if(stage.strictEquals("body-abrupt")||stage.strictEquals("getter-abrupt"))return g.get("next").call(g);
  return g.get("throw").call(g,{context.get("reason")});
 },{context});
 const auto rejected=resolved.get("then").call(resolved,{reaction});
 const auto onRejected=JSValue::function(+[](const JSValue&,const Args& args,const Args& c)->JSValue{
  const auto context=c[0],received=argument(args,0);own(context,"caught",true);own(context,"sameValue",received.sameValue(context.get("reason")));own(context,"received",normal(received));push(context.get("events"),array({"catch",normal(received)}));
  const auto g=context.get("generator");if(g.isObject()){
   own(context,"postNext",resultMetadata(g.get("next").call(g,{99})));
   bool same=false;try{g.get("throw").call(g,{context.get("reason")});}catch(const JSGeneratorThrown& thrown){same=thrown.value().sameValue(context.get("reason"));}own(context,"completedThrowSame",same);
  }
  return "recovered";
 },{context});
 const auto recovered=rejected.get("catch").call(rejected,{onRejected});
 const auto onSettled=JSValue::function(+[](const JSValue&,const Args& args,const Args& c)->JSValue{own(c[0],"settled",true);own(c[0],"settledValue",normal(argument(args,0)));push(c[0].get("events"),array({"settled",normal(argument(args,0))}));return "checkpoint-ready";},{context});
 const auto settled=recovered.get("then").call(recovered,{onSettled});
 const auto sentinelPromise=constructor.get("resolve").call(constructor);
 const auto sentinel=sentinelPromise.get("then").call(sentinelPromise,{JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue{push(c[0].get("events"),"sentinel");if(c[0].get("fixture").get("gc").toBoolean())c[0].collectGarbage();return {};},{context})});
 push(events,"sync.after");auto before=array();for(unsigned i=0;i<events.length();++i)push(before,events.get(std::to_string(i)));const auto caughtBefore=context.get("caught");
 drainMicrotasks();push(events,"checkpoint.after");
 return record({{"events",events},{"setup",setup},{"beforeCheckpoint",before},{"caughtBefore",caughtBefore},{"caught",context.get("caught")},{"sameValue",context.get("sameValue")},{"received",context.hasOwn("received")?context.get("received"):JSValue("#not-caught")},{"reason",normal(reason)},{"reasonType",interopType(reason)},{"postNext",context.hasOwn("postNext")?context.get("postNext"):JSValue("#not-created")},{"completedThrowSame",context.hasOwn("completedThrowSame")?context.get("completedThrowSame"):JSValue("#not-created")},{"settled",context.get("settled")},{"settledValue",context.hasOwn("settledValue")?context.get("settledValue"):JSValue("#not-settled")}});
}

JSValue run(const JSValue& fixture){
 const auto kind=fixture.get("kind");if(kind.strictEquals("promiseInterop"))return promiseInterop(fixture);if(kind.strictEquals("metadata"))return metadata();
 if(kind.strictEquals("componentMetadata")){const auto f=createNativeGeneratorFunction(components,{array(),fixture},"[Symbol.iterator]",0,fixture.get("source").string());return record({{"source",f.nativeFunctionSource()},{"name",f.get("name")},{"length",f.get("length")},{"constructible",f.isConstructible()},{"own",ownMetadata(f)},{"functionPrototype",f.getPrototype().sameValue(nativeGeneratorFunctionPrototype())},{"prototypeParent",f.get("prototype").getPrototype().sameValue(nativeGeneratorPrototype())}});}
 if(kind.strictEquals("identity")){const auto method=fixture.get("method");auto events=array();auto f=createNativeGeneratorFunction(linear,{events});auto receiver=record({{"first",7}});auto g=f.call(receiver);if(fixture.get("started").toBoolean())resumeNativeGenerator(g);const auto value=decode(fixture.get("value"));bool same=false;JSValue done;try{const auto result=nativeGeneratorPrototype().get(method).call(g,{value});same=result.get("value").strictEquals(value);done=result.get("done");}catch(const JSGeneratorThrown& e){same=e.value().strictEquals(value);done="throw";}return record({{"same",same},{"done",done}});}

 if(kind.strictEquals("emptyNewTarget")){
  auto events=array();const auto ctor=fixture.get("generator").toBoolean()?nativeGeneratorFunctionConstructor():nativeFunctionConstructor();
  auto target=JSValue::constructor(+[](const JSValue&,const Args&,const Args&)->JSValue{return {};},{},"Derived",0,JSValue::ConstructorKind::Callable);const auto custom=JSValue::object(nullptr);const auto mode=fixture.get("mode");auto slot=JSValue::object();
  if(mode.strictEquals("object"))target.set("prototype",custom);else if(mode.strictEquals("null"))target.set("prototype",nullptr);else if(mode.strictEquals("number"))target.set("prototype",3);else if(mode.strictEquals("bound"))target=target.bind({});else if(mode.strictEquals("nonConstructor"))target=JSValue::object();
  else {auto handler=JSValue::object();own(handler,"get",JSValue::function(+[](const JSValue&,const Args& args,const Args& c)->JSValue{if(args[1].strictEquals("prototype")){push(c[0],"prototype.get");if(c[1].get("throws").toBoolean())throw JSTypeError("prototype getter");if(c[1].get("revoke").toBoolean())revokeNativeProxy(c[3].get("proxy"));return c[1].get("objectPrototype").toBoolean()?c[2]:JSValue(nullptr);}return args[0].get(args[1],args[2]);},{events,fixture,custom,slot},"get",3));target=createNativeProxy(target,handler);own(slot,"proxy",target);}
  JSValue f;const auto result=attempt([&]()->JSValue{f=ctor.construct({},target);return "created";});
  return record({{"result",result},{"custom",f.isObject()&&f.getPrototype().sameValue(custom)},{"fallback",f.isObject()&&f.getPrototype().sameValue(fixture.get("generator").toBoolean()?nativeGeneratorFunctionPrototype():nativeFunctionConstructor().get("prototype"))},{"events",events}});
 }
 if(kind.strictEquals("resultSetters")){
  auto events=array();auto op=JSValue::intrinsicObjectConstructor().get("prototype");const auto value=op.getOwnPropertyDescriptor("value"),done=op.getOwnPropertyDescriptor("done");
  for(const auto key:{"value","done"}){JSPropertyDescriptor d;d.configurable=true;d.setter=JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue{push(c[0],c[1]);return {};},{events,key},"set",1);op.defineProperty(key,d);}
  const auto g=nativeGeneratorFunctionConstructor().call().call();const auto first=resultMetadata(resumeNativeGenerator(g)),last=resultMetadata(resumeNativeGenerator(g,{JSGeneratorCompletionKind::Return,4}));
  if(value)op.defineProperty("value",*value);else op.erase("value");if(done)op.defineProperty("done",*done);else op.erase("done");return record({{"first",first},{"last",last},{"events",events}});
 }
 if(kind.strictEquals("brand")){
  auto gp=nativeGeneratorPrototype();const auto mode=fixture.get("receiver");JSValue receiver;
  if(mode.strictEquals("null"))receiver=nullptr;else if(mode.strictEquals("number"))receiver=3;else if(mode.strictEquals("object"))receiver=JSValue::object();else if(mode.strictEquals("fake"))receiver=JSValue::object(gp);else if(mode.strictEquals("prototype"))receiver=gp;
  else if(mode.strictEquals("functionPrototype"))receiver=nativeGeneratorFunctionPrototype();else if(mode.strictEquals("function"))receiver=createNativeGeneratorFunction(linear);
  else if(mode.strictEquals("proxy")||mode.strictEquals("revoked")){const auto g=createNativeGeneratorFunction(linear).call();auto handler=JSValue::object();for(const auto key:{"get","getPrototypeOf"})own(handler,key,JSValue::function(+[](const JSValue&,const Args&,const Args&)->JSValue{throw JSRangeError("must not invoke brand proxy trap");}));receiver=createNativeProxy(g,handler);if(mode.strictEquals("revoked"))revokeNativeProxy(receiver);}
  return attempt([&]()->JSValue{return resultMetadata(gp.get(fixture.get("method")).call(receiver,{decode(fixture.get("argument"))}));});
 }
 if(kind.strictEquals("empty")){
  const bool generator=fixture.get("generator").toBoolean();const auto ctor=generator?nativeGeneratorFunctionConstructor():nativeFunctionConstructor();const auto f=fixture.get("construct").toBoolean()?ctor.construct():ctor.call();auto returned=f.call();
  return record({{"constructorOwn",ownMetadata(ctor)},{"functionOwn",ownMetadata(f)},{"source",f.nativeFunctionSource()},{"constructible",f.isConstructible()},{"body",generator?resultMetadata(returned.get("next").call(returned)):normal(returned)}});
 }
 if(kind.strictEquals("prototypeMutation")){
  auto f=createNativeGeneratorFunction(linear);const auto mode=fixture.get("mode");auto custom=JSValue::object(nullptr);JSValue replacement;
  if(mode.strictEquals("object"))replacement=custom;else if(mode.strictEquals("null"))replacement=nullptr;else if(mode.strictEquals("number"))replacement=3;else if(mode.strictEquals("undefined"))replacement={};else replacement=JSValue::function(+[](const JSValue&,const Args&,const Args&)->JSValue{return {};});
  f.set("prototype",replacement);const auto g=f.call();
  return record({{"custom",g.getPrototype().sameValue(replacement)},{"fallback",g.getPrototype().sameValue(nativeGeneratorPrototype())},{"brand",isNativeGeneratorObject(g)},{"own",ownMetadata(g)}});
 }
 if(kind.strictEquals("privateIntrinsics")){
  auto gp=nativeGeneratorPrototype(),fp=nativeGeneratorFunctionPrototype(),ctor=nativeGeneratorFunctionConstructor();const auto c=gp.getOwnPropertyDescriptor("constructor"),p=fp.getOwnPropertyDescriptor("prototype");
  gp.defineProperty("constructor",descriptor(nullptr));fp.defineProperty("prototype",descriptor(nullptr));
  auto f=createNativeGeneratorFunction(linear);auto g=f.call();const auto stable=nativeGeneratorPrototype().sameValue(gp)&&nativeGeneratorFunctionPrototype().sameValue(fp)&&nativeGeneratorFunctionConstructor().sameValue(ctor)&&f.get("prototype").getPrototype().sameValue(gp);
  gp.defineProperty("constructor",*c);fp.defineProperty("prototype",*p);return stable;
 }
 auto events=array();auto store=JSValue::object(nullptr);const auto receiver=makeReceiver(fixture,events,store);const auto body=fixture.get("body");
 JSGeneratorBody callback=body.strictEquals("controlled")?controlled:body.strictEquals("components")?components:body.strictEquals("initialized")?initialized:linear;
 auto f=createNativeGeneratorFunction(callback,{events,fixture},body.strictEquals("components")?"[Symbol.iterator]":"fixture",body.strictEquals("components")?0:1,fixture.get("source").isUndefined()?"":fixture.get("source").string(),body.strictEquals("initialized")?initialize:nullptr);
 Args args;const auto list=fixture.get("args");if(list.isObject())for(unsigned i=0;i<list.length();++i)args.push_back(decode(list.get(std::to_string(i))));
 JSValue g;const auto creation=attempt([&]()->JSValue{g=f.call(receiver,args);return "created";});
 if(!g.isObject())return record({{"creation",creation},{"events",events}});
 if(receiver.isObject())own(receiver,"generator",g);
 auto results=array();const auto steps=fixture.get("steps");
 for(unsigned i=0;i<steps.length();++i){const auto step=steps.get(std::to_string(i));const auto changes=step.get("mutate");if(changes.isObject())for(const auto& key:changes.ownPropertyNames())own(store,key,decode(changes.get(key)));
  if(step.get("method").strictEquals("iterator"))push(results,record({{"kind","iterator"},{"same",g.get(JSValue::wellKnownSymbol("iterator")).call(g).sameValue(g)}}));
  else push(results,attempt([&]()->JSValue{return resultMetadata(nativeGeneratorPrototype().get(step.get("method")).call(g,{decode(step.get("argument"))}));}));
 }
 return record({{"creation",creation},{"results",results},{"events",events},{"own",ownMetadata(g)}});
}
JSGeneratorStep keep(const JSValue& state,const JSGeneratorCompletion& c,const JSValue& receiver,const Args& args,const Args& captures){
 if(c.kind!=JSGeneratorCompletionKind::Normal)return passAbrupt(c);const auto i=pc(state);pc(state,i+1);if(!i){own(state,"cycle",state);return JSGeneratorStep::yield(captures[0]);}if(!receiver.isObject()||args.empty())throw std::logic_error("lost generator context");return JSGeneratorStep::complete();
}
void lifetime(){
 for(unsigned realm=0;realm<3;++realm){JSWeakValue wc,wgp,wfp;{
  auto root=JSValue::object();auto ctor=nativeGeneratorFunctionConstructor();wc=JSWeakValue(ctor);wgp=JSWeakValue(nativeGeneratorPrototype());wfp=JSWeakValue(nativeGeneratorFunctionPrototype());
  JSWeakValue wf,wg,wcap,wreceiver,warg;
  {auto capture=JSValue::object(),receiver=JSValue::object(),arg=JSValue::object();auto f=createNativeGeneratorFunction(keep,{capture});auto g=f.call(receiver,{arg});own(capture,"function",f);own(capture,"generator",g);own(g,"self",g);wf=JSWeakValue(f);wg=JSWeakValue(g);wcap=JSWeakValue(capture);wreceiver=JSWeakValue(receiver);warg=JSWeakValue(arg);
   root.collectGarbage();if(wcap.expired()||wreceiver.expired()||warg.expired())throw std::logic_error("live context lost");
   const auto first=resumeNativeGenerator(g);if(!first.get("value").sameValue(capture))throw std::logic_error("capture identity lost");
  }
  root.collectGarbage();if(!wf.expired()||!wg.expired()||!wcap.expired()||!wreceiver.expired()||!warg.expired())throw std::logic_error("generator capture/function/self/locals cycle retained");
  {JSValue g;JSWeakValue wr,wa;{auto receiver=JSValue::object(),arg=JSValue::object();wr=JSWeakValue(receiver);wa=JSWeakValue(arg);g=createNativeGeneratorFunction(keep,{nullptr}).call(receiver,{arg});}root.collectGarbage();if(wr.expired()||wa.expired())throw std::logic_error("suspended-start context released");resumeNativeGenerator(g,{JSGeneratorCompletionKind::Return,77});root.collectGarbage();if(!wr.expired()||!wa.expired())throw std::logic_error("completed context retained");}
 }
 JSValue::nativeSafepoint();if(!wc.expired()||!wgp.expired()||!wfp.expired()||JSValue::hasActiveHeap())throw std::logic_error("generator realm retained");
 }
 std::cout<<"3 generator realm / true-cycle / context-release lifetimes passed\n";
}
}
int main(int argc,char** argv){try{if(argc>1&&std::string(argv[1])=="--lifetime"){lifetime();return 0;}const std::string text(std::istreambuf_iterator<char>(std::cin),{});const auto input=Animation_detail::parseJSON(text);auto output=array();auto execute=JSValue::function(+[](const JSValue&,const Args&,const Args& captures)->JSValue{for(unsigned i=0;i<captures[0].length();++i)push(captures[1],run(captures[0].get(std::to_string(i))));return {};},{input,output});execute.call();std::cout<<output.toJSON()->dump();}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}}
