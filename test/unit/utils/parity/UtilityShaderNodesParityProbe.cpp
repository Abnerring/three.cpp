#include "animation/AnimationUtils.h"
#include "core/JSIterator.h"
#include "nodes/core/Node.h"
#include "nodes/core/NodeBuilder.h"
#include "nodes/core/StackNode.h"
#include "nodes/code/ExpressionNode.h"
#include "nodes/utils/Discard.h"
#include "nodes/utils/Remap.h"
#include "nodes/utils/DebugNode.h"
#include "nodes/tsl/TSLCore.h"
#include "utils.h"
#include <cmath>
#include <iostream>
#include <iterator>
#include <sstream>

namespace {
using Args=std::vector<JSValue>;
bool relative=false;double baseline=0;
JSValue arg(const Args& a,std::size_t i){return i<a.size()?a[i]:JSValue{};}
JSValue coalesce(const JSValue& value,const JSValue& fallback){return value.isUndefined()||value.isNull()?fallback:value;}
void own(JSValue object,const std::string& key,const JSValue& value){JSPropertyDescriptor d;d.value=value;d.writable=d.enumerable=d.configurable=true;object.defineProperty(key,d);}
JSValue array(const Args& values={}){auto out=JSValue::array();for(std::size_t i=0;i<values.size();++i)own(out,std::to_string(i),values[i]);return out;}
JSValue call(const JSValue& object,const char* key,const Args& args={}){return object.get(key).call(object,args);}
void push(const JSValue& array,const JSValue& value){call(array,"push",{value});}
Args values(const JSValue& array){Args out;for(unsigned i=0;i<array.length();++i)out.push_back(array.get(std::to_string(i)));return out;}
JSValue normal(const JSValue& value,Args* seen=nullptr){
 if(value.isUndefined())return "#undefined";if(value.isCallable())return "#function";if(value.isSymbol())return "#symbol";
 if(value.type()==JSValue::Type::Number){const auto n=value.number();if(std::isnan(n))return "#NaN";if(!std::isfinite(n))return n>0?"#Infinity":"#-Infinity";if(n==0&&std::signbit(n))return "#-0";}
 if(!value.isObject())return value;if(value.get("fixtureTag").toBoolean())return "@"+value.get("fixtureTag").toString();Args local;if(!seen)seen=&local;
 for(std::size_t i=0;i<seen->size();++i)if(value.sameValue((*seen)[i]))return array({"#ref",static_cast<double>(i)});const auto index=static_cast<double>(seen->size());seen->push_back(value);
 auto out=value.type()==JSValue::Type::Array?JSValue::array(value.length()):JSValue::object();for(const auto& key:value.keys()){
  const auto field=value.get(key);own(out,key,relative&&key=="id"&&value.get("isNode").strictEquals(true)?(field.toNumber()>baseline?array({"#local",field.toNumber()-baseline}):array({"#existing",index})):normal(field,seen));
 }return out;
}
template<class F>JSValue attempt(F body){try{return normal(body());}catch(const JSTypeError&){return "#TypeError";}catch(const JSRangeError&){return "#RangeError";}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return "#Error";}}
void event(const JSValue& state,const Args& args){push(state.get("events"),array(args));}
void observe(const JSValue& state,JSValue object,const char* key,const std::string& label){const auto value=object.get(key);JSPropertyDescriptor d;d.configurable=d.enumerable=true;d.getter=JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue{event(c[0],{"get",c[2]});if(c[0].get("r").get("throwAt").sameValue(c[2]))throw JSTypeError("getter");return c[1];},{state,value,label});object.defineProperty(key,d);}
JSValue primitive(const JSValue& state,const char* label,const JSValue& value){auto out=JSValue::object();out.setPrimitiveConverter(JSValue::function(+[](const JSValue&,const Args& args,const Args& c)->JSValue{event(c[0],{"primitive",c[1],arg(args,0)});if(c[0].get("r").get("throwAt").strictEquals("primitive."+c[1].toString()))throw JSTypeError("primitive");return c[2];},{state,label,value}));return out;}
JSValue module(const JSValue& name){if(name.strictEquals("expression"))return nativeExpressionNodeModule();if(name.strictEquals("debug"))return nativeDebugNodeModule();if(name.strictEquals("discard"))return nativeDiscardModule();return nativeRemapModule();}
JSValue metadata(const JSValue& request){const auto m=module(request.get("module")),c=m.get("default"),p=c.get("prototype");auto n=c.construct(request.get("module").strictEquals("debug")?Args{1}:Args{});n.set("id",100);auto out=JSValue::object(),exports=array(),fields=array(),methods=array();
 for(const auto& key:m.keys())push(exports,key);
 for(const auto& key:n.ownPropertyNames()){const auto d=n.getOwnPropertyDescriptor(key).value();push(fields,array({key,normal(n.get(key)),d.writable.value_or(false),d.enumerable.value_or(false),d.configurable.value_or(false)}));}
 for(const auto& key:p.ownPropertyNames()){const auto d=p.getOwnPropertyDescriptor(key).value();const auto v=d.value.value_or(JSValue{});push(methods,array({key,normal(v),v.isCallable()?v.get("name"):JSValue(nullptr),v.isCallable()?v.get("length"):JSValue(nullptr),d.writable.value_or(false),d.enumerable.value_or(false),d.configurable.value_or(false)}));}
 own(out,"exports",exports);own(out,"name",c.get("name"));own(out,"length",c.get("length"));own(out,"type",c.get("type"));own(out,"parent",c.getPrototype().get("name"));own(out,"fields",fields);own(out,"methods",methods);return out;
}
JSValue callback(const JSValue& state,const char* name){return JSValue::function(+[](const JSValue& self,const Args& args,const Args& c)->JSValue{
 const auto r=c[0].get("r"),name=c[1];Args row{"call",name,self.isUndefined()};for(const auto& value:args)row.push_back(normal(value));event(c[0],row);
 if(r.get("throwAt").strictEquals(name.toString()+"()"))throw JSTypeError("call");if(name.strictEquals("child.getNodeType"))return r.get("type");if(name.strictEquals("child.build"))return r.get("objects").toBoolean()?primitive(c[0],"snippet","SNIP"):JSValue("SNIP");if(name.strictEquals("format"))return array(args);return {};
 },{state,name});}
JSValue protocol(const JSValue& request){
 auto state=JSValue::object();own(state,"r",request);own(state,"events",array());auto flow=JSValue::object(),builder=JSValue::object();own(flow,"code",coalesce(request.get("code"),"\t\tfloat x;\n\treturn x;"));own(builder,"fixtureTag","builder");own(builder,"shaderStage",coalesce(request.get("stage"),"fragment"));own(builder,"flow",flow);JSValue n;
 if(request.get("module").strictEquals("expression")){
  n=nativeExpressionNodeConstructor().construct({request.get("objects").toBoolean()?primitive(state,"snippet","SNIP"):JSValue("SNIP"),request.get("type")});n.set("getNodeType",callback(state,"child.getNodeType"));builder.set("addLineFlowCode",callback(state,"addLineFlowCode"));builder.set("format",callback(state,"format"));
 }else{
  auto child=JSValue::object();own(child,"fixtureTag","child");own(child,"getNodeType",callback(state,"child.getNodeType"));own(child,"build",callback(state,"child.build"));JSValue cb=nullptr;
  if(request.get("callback").strictEquals("function"))cb=callback(state,"callback");else if(request.get("callback").strictEquals("false"))cb=false;else if(request.get("callback").strictEquals("undefined"))cb={};
  n=nativeDebugNodeConstructor().construct({child,cb});if(request.get("objects").toBoolean())builder.set("shaderStage",primitive(state,"stage",coalesce(request.get("stage"),"fragment")));
  for(const auto* key:{"getNodeType","build"})observe(state,child,key,std::string("child.")+key);
 }
 n.set("fixtureTag","self");if(!request.get("observed").strictEquals(false)){
  if(request.get("module").strictEquals("expression")){observe(state,n,"getNodeType","self.getNodeType");observe(state,n,"snippet","self.snippet");}
  else{observe(state,n,"node","self.node");observe(state,n,"callback","self.callback");}
  for(const auto& key:builder.keys())if(key!="fixtureTag")observe(state,builder,key.c_str(),"builder."+key);observe(state,flow,"code","flow.code");
 }
 const auto previous=ThreeUtils::getConsoleFunction();ThreeUtils::setConsoleFunction(JSValue::function(+[](const JSValue&,const Args& args,const Args& c)->JSValue{Args row{"console"};for(const auto& value:args)row.push_back(normal(value));event(c[0],row);if(c[0].get("r").get("throwAt").strictEquals("console"))throw JSTypeError("console");return {};},{state}));
 auto out=JSValue::object();own(out,"result",attempt([&]{return n.get(request.get("method")).call(n,{builder,request.get("output")});}));ThreeUtils::setConsoleFunction(previous);own(out,"events",state.get("events"));return out;
}
JSValue remapFixture(const JSValue& state,const std::string& tag){auto n=nativeNodeConstructor().construct({"float"});n.set("fixtureTag",tag);for(const auto* key:{"sub","div","clamp","mul","add"}){JSPropertyDescriptor d;d.configurable=true;d.getter=JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue{event(c[0],{"get",c[1]});if(c[0].get("r").get("throwAt").sameValue(c[1]))throw JSTypeError("getter");return JSValue::function(+[](const JSValue&,const Args& args,const Args& c)->JSValue{Args row{"call",c[1]};for(const auto& value:args)row.push_back(normal(value));event(c[0],row);const auto label=c[1].toString()+"()";if(c[0].get("r").get("throwAt").strictEquals(label))throw JSTypeError("call");return remapFixture(c[0],label);},c);},{state,tag+"."+key});n.defineProperty(key,d);}return n;}
JSValue remapBody(const JSValue& request,const JSValue& tsl){
 auto state=JSValue::object();own(state,"r",request);own(state,"events",array());Args args;
 if(request.get("fixture").toBoolean()){
  for(const auto* tag:{"node","low","high","outLow","outHigh"})args.push_back(remapFixture(state,tag));const auto clamp=request.get("clamp");args.push_back(clamp.strictEquals("node")?nativeNodeConstructor().construct({"bool"}):clamp.strictEquals("null")?JSValue(nullptr):tsl.get("bool").call({}, {clamp.strictEquals(true)}));
 }else for(const auto& value:values(request.get("args")))args.push_back(value.isNull()?JSValue{}:tsl.get("float").call({}, {value}));
 auto iterable=array(args);
 if(!request.get("iterator").strictEquals("array")){
  auto source=iterable;iterable=JSValue::object();iterable.set(JSValue::wellKnownSymbol("iterator"),JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue{
   event(c[0],{"iterator"});auto state=JSValue::object(),iterator=JSValue::object();state.set("index",0);
   iterator.set("next",JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue{auto state=c[2];const auto i=state.get("index");event(c[0],{"next",i});if(c[0].get("r").get("throwAt").strictEquals("next:"+i.toString()))throw JSTypeError("next");auto result=JSValue::object();const auto done=i.toNumber()>=c[1].length();result.set("done",done);if(!done){result.set("value",c[1].get(i));state.set("index",i.toNumber()+1);}return result;},{c[0],c[1],state}));
   JSPropertyDescriptor d;d.configurable=true;d.getter=JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue{event(c[0],{"getReturn"});if(c[0].get("r").get("throwAt").strictEquals("getReturn"))throw JSTypeError("return");return JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue{event(c[0],{"return"});if(c[0].get("r").get("throwAt").strictEquals("return"))throw JSTypeError("return");return JSValue::object();},c);},{c[0]});iterator.defineProperty("return",d);return iterator;
  },{state,source}));
 }
 auto out=JSValue::object();own(out,"result",attempt([&]{return nativeRemapBinding("remap").get("shaderNode").get("jsFunc").call({}, {iterable});}));own(out,"events",state.get("events"));return out;
}
JSValue factory(const JSValue& request,const JSValue& tsl){
 const auto old=call(tsl,"getCurrentStack"),stack=nativeStackNodeConstructor().construct();call(tsl,"setCurrentStack",{request.get("noStack").toBoolean()?JSValue(nullptr):stack});JSValue result;
 try{
  const auto name=request.get("module");if(name.strictEquals("discard"))result=request.get("name").strictEquals("Return")?nativeDiscardBinding("Return").call():nativeDiscardBinding("Discard").call({}, {request.get("condition")});
  else if(name.strictEquals("debug"))result=nativeDebugNodeBinding("debug").call({}, {tsl.get("float").call({}, {2})});
  else if(name.strictEquals("expression"))result=nativeExpressionNodeBinding("expression").call({},values(request.get("args")));
  else result=nativeRemapBinding(request.get("name").string()).call({},values(request.get("args")));
  auto out=JSValue::object();own(out,"result",normal(result));own(out,"stack",normal(stack.get("nodes")));call(tsl,"setCurrentStack",{old});return out;
 }catch(...){call(tsl,"setCurrentStack",{old});throw;}
}
JSValue integration(const JSValue& request,const JSValue& tsl){
 auto renderer=JSValue::object(),debug=JSValue::object(),diagnostics=JSValue::object();diagnostics.set("keywords",false);debug.set("diagnostics",diagnostics);renderer.set("debug",debug);renderer.set("coordinateSystem",2001);
 auto builder=nativeNodeBuilderConstructor().construct({nullptr,renderer,nullptr});builder.set("shaderStage","fragment");JSValue node;const auto name=request.get("module");
 if(name.strictEquals("expression"))node=nativeExpressionNodeBinding("expression").call({}, {request.get("snippet"),request.get("type")});
 else if(name.strictEquals("debug"))node=nativeDebugNodeConstructor().construct({tsl.get("float").call({}, {2}),JSValue::function(+[](const JSValue&,const Args&,const Args&)->JSValue{return {};})});
 else if(name.strictEquals("remap"))node=nativeRemapBinding(request.get("name").string()).call({},values(request.get("args")));
 else{const auto old=call(tsl,"getCurrentStack"),stack=nativeStackNodeConstructor().construct();call(tsl,"setCurrentStack",{stack});try{nativeDiscardBinding(request.get("name").string()).call({}, {request.get("condition")});node=stack;call(tsl,"setCurrentStack",{old});}catch(...){call(tsl,"setCurrentStack",{old});throw;}}
 auto results=array();for(const auto* stage:{"setup","analyze","generate"}){builder.set("buildStage",stage);push(results,attempt([&]{const auto value=call(node,"build",{builder,request.get("output")});return value.isObject()&&value.get("isNode").toBoolean()?JSValue("#"+value.get("type").toString()):value;}));}
 auto out=JSValue::object();own(out,"values",results);own(out,"flow",normal(builder.get("flow")));own(out,"stackRestored",call(tsl,"getCurrentStack").isNull());return out;
}
JSValue logging(const JSValue& request){auto events=array();const auto old=ThreeUtils::getConsoleFunction();std::ostringstream stream;std::streambuf* original=nullptr;
 if(request.get("fallback").toBoolean()){ThreeUtils::setConsoleFunction(nullptr);original=std::cout.rdbuf(stream.rdbuf());}
 else ThreeUtils::setConsoleFunction(JSValue::function(+[](const JSValue&,const Args& args,const Args& c)->JSValue{Args row;for(const auto& value:args)row.push_back(normal(value));push(c[0],array(row));if(c[1].get("throwAt").strictEquals("console"))throw JSTypeError("console");return {};},{events,request}));
 auto out=JSValue::object();own(out,"result",attempt([&]()->JSValue{ThreeUtils::log(values(request.get("args")));return {};}));own(out,"events",events);own(out,"text",stream.str());if(original)std::cout.rdbuf(original);ThreeUtils::setConsoleFunction(old);return out;
}
}
int main(){try{const std::string text(std::istreambuf_iterator<char>(std::cin),{});const auto requests=Animation_detail::parseJSON(text);const auto builder=nativeNodeBuilderConstructor(),tsl=nativeTSLCore(),expr=nativeExpressionNodeModule(),discard=nativeDiscardModule(),remap=nativeRemapModule(),debug=nativeDebugNodeModule();auto out=array();
 for(unsigned i=0;i<requests.length();++i){const auto r=requests.get(std::to_string(i)),kind=r.get("kind");relative=r.get("relativeNodeIds").toBoolean();baseline=nativeNodeConstructor().construct().get("id").toNumber();JSValue result;if(kind.strictEquals("metadata"))result=metadata(r);else if(kind.strictEquals("protocol"))result=protocol(r);else if(kind.strictEquals("remapBody"))result=remapBody(r,tsl);else if(kind.strictEquals("factory"))result=factory(r,tsl);else if(kind.strictEquals("integration"))result=integration(r,tsl);else result=logging(r);push(out,result);}
 std::cout<<out.toJSON()->dump();return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
