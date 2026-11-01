#include "core/JSValue.h"
#include "nodes/tsl/TSLCore.h"
#include "nodes/core/Node.h"
#include "animation/AnimationUtils.h"
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
using Args=std::vector<JSValue>;
const std::vector<std::string> names={"sourceFixtureAlpha","sourceFixtureBeta","sourceFixtureGamma"};
JSValue array(const Args& values={}){auto a=JSValue::array();for(const auto& v:values)a.set(std::to_string(a.length()),v);return a;}
void push(JSValue a,const JSValue& v){a.set(std::to_string(a.length()),v);}
JSValue normal(const JSValue& v,const JSValue& state){if(v.isUndefined())return "#undefined";if(v.sameValue(state.get("node")))return "#receiver";if(v.sameValue(state.get("result")))return "#result";return v;}
void require(bool value,const char* text){if(!value)throw std::logic_error(text);}
JSValue source(const JSValue& fn){return fn.get("toString").call(fn);}
JSValue metadata(const std::string& name,bool assign){
 const auto proto=nativeNodeConstructor().get("prototype"),fn=proto.get(name+(assign?"Assign":""));
 const auto d=proto.getOwnPropertyDescriptor(name+(assign?"Assign":"")).value();
 const auto p=fn.getOwnPropertyDescriptor("prototype").value();auto out=JSValue::object();
 out.set("name",fn.get("name"));out.set("length",fn.get("length"));out.set("source",source(fn));
 auto keys=array();for(const auto& key:fn.ownPropertyNames())push(keys,key);out.set("ownKeys",keys);
 out.set("hasOwnToString",fn.hasOwn("toString"));out.set("prototypeDescriptor",array({p.writable.value_or(false),p.enumerable.value_or(false),p.configurable.value_or(false),fn.get("prototype").get("constructor").sameValue(fn)}));
 out.set("methodDescriptor",array({d.writable.value_or(false),d.enumerable.value_or(false),d.configurable.value_or(false)}));
 out.set("stableIdentity",fn.sameValue(proto.get(name+(assign?"Assign":""))));out.set("distinctAssign",!proto.get(name).sameValue(proto.get(name+"Assign")));return out;
}
JSValue install(){
 auto states=JSValue::object();const auto add=nativeTSLCoreBinding("addMethodChaining");
 for(const auto& name:names){auto state=JSValue::object();states.set(name,state);
  auto element=JSValue::function([](const JSValue& self,const Args& args,const Args& cap)->JSValue{
   auto state=cap[0];state.collectGarbage();Args event{"element",cap[1],self.isUndefined()};for(const auto& v:args)event.push_back(normal(v,state));push(state.get("events"),array(event));
   if(state.get("throwElement").toBoolean())throw JSTypeError("fixture-element");return state.get("result");
  },{state,name},"element",0);add.call({}, {name,element});
 }
 return states;
}
JSValue run(const JSValue& r,JSValue states){
 auto state=states.get(r.get("name"));state.set("events",array());state.set("result",JSValue::object());state.set("throwElement",r.get("throwElement"));
 auto node=nativeNodeConstructor().construct();state.set("node",node);
 JSPropertyDescriptor stack;stack.configurable=true;stack.getter=JSValue::function([](const JSValue&,const Args&,const Args& cap)->JSValue{cap[0].collectGarbage();push(cap[0].get("events"),array({"stack.get"}));return cap[1];},{state,r.get("stack")},"get",0);node.defineProperty("isStackNode",stack);
 for(const auto* key:{"assign","addToStack"}){JSPropertyDescriptor d;d.configurable=true;d.getter=JSValue::function([](const JSValue&,const Args&,const Args& cap)->JSValue{
  cap[0].collectGarbage();push(cap[0].get("events"),array({"get",cap[1]}));return JSValue::function([](const JSValue& self,const Args& args,const Args& cap)->JSValue{cap[0].collectGarbage();Args event{"call",cap[1],self.sameValue(cap[0].get("node"))};for(const auto& v:args)event.push_back(normal(v,cap[0]));push(cap[0].get("events"),array(event));return cap[0].get("result");},{cap[0],cap[1]},"fixture",0);
 },{state,key},"get",0);node.defineProperty(key,d);}
 const auto name=r.get("name").string();const bool assign=r.get("assign").toBoolean();auto out=JSValue::object();out.set("metadata",metadata(name,assign));
 Args args;for(std::uint32_t i=0;i<r.get("args").length();++i)args.push_back(r.get("args").get(std::to_string(i)));
 try{out.set("result",normal(node.get(name+(assign?"Assign":"")).call(node,args),state));}catch(const JSTypeError&){out.set("result","#TypeError");}
 out.set("events",state.get("events"));return out;
}
JSValue summaryMetadata(){auto out=JSValue::object(),ns=array();std::vector<JSValue> methods;const auto proto=nativeNodeConstructor().get("prototype");bool different=true;
 for(const auto& name:names){push(ns,name);for(const auto& key:{name,name+"Assign"}){auto fn=proto.get(key);for(const auto& prior:methods)different=different&&!fn.sameValue(prior);methods.push_back(fn);}}
 out.set("names",ns);out.set("methodCount",6);out.set("allDifferent",different);auto builtins=array();for(const auto* name:{"toFloat","element"}){auto entry=JSValue::object();entry.set("name",name);entry.set("normal",metadata(name,false));entry.set("assign",metadata(name,true));push(builtins,entry);}out.set("builtins",builtins);return out;
}
void lifetime(){
 JsonValue::Array rounds;
 for(unsigned round=0;round<3;++round){JSWeakValue heap,callbackWitness,methodWitness;
  {
   auto add=nativeTSLCoreBinding("addMethodChaining"),proto=nativeNodeConstructor().get("prototype");auto captured=JSValue::object();captured.set("value",37);JSWeakValue capturedWitness(captured);
   auto callback=JSValue::function([](const JSValue&,const Args&,const Args& cap)->JSValue{cap[0].collectGarbage();return cap[0].get("value");},{captured},"captured",0);callbackWitness=JSWeakValue(callback);
   const auto name="lifetimeSourceFixture"+std::to_string(round);add.call({}, {name,callback});auto method=proto.get(name);methodWitness=JSWeakValue(method);heap=JSWeakValue(add);
   callback={};captured={};add.collectGarbage();require(!callbackWitness.expired()&&!capturedWitness.expired(),"Registered real NodeElements/closure lost callback captures");
   proto.erase(name);proto.erase(name+"Assign");add.collectGarbage();require(method.call(JSValue::object()).strictEquals(37),"Detached chaining closure lost captured element");
   require(source(method).toString().find("function ( ...params )")!=std::string::npos,"Detached chaining closure lost exact source");
   method={};add.collectGarbage();require(methodWitness.expired(),"Removed chaining function was retained by source metadata");
   require(!callbackWitness.expired(),"Real NodeElements registry unexpectedly released registered callback");
  }
  JSValue::nativeSafepoint();require(heap.expired()&&callbackWitness.expired(),"Source metadata or module cache retained retired realm");rounds.push_back(JsonValue::Array{true,true,true,true});
 }
 std::cout<<JsonValue(rounds).dump()<<'\n';
}
}
int main(int argc,char** argv){try{if(argc==2&&std::string(argv[1])=="--lifetime"){lifetime();return 0;}require(argc==1,"Unsupported probe arguments");const std::string input((std::istreambuf_iterator<char>(std::cin)),{});auto requests=Animation_detail::parseJSON(input),states=install(),cases=array();for(std::uint32_t i=0;i<requests.length();++i)push(cases,run(requests.get(std::to_string(i)),states));auto out=JSValue::object();out.set("cases",cases);out.set("metadata",summaryMetadata());std::cout<<out.toJSON()->dump()<<'\n';}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
