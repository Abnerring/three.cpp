#include "animation/AnimationUtils.h"
#include "nodes/core/Node.h"
#include "nodes/core/NodeBuilder.h"
#include "nodes/display/ColorSpaceNode.h"
#include "nodes/tsl/TSLCore.h"
#include "math/ColorManagement.h"
#include <cmath>
#include <iostream>
#include <iterator>

namespace {
using Args=std::vector<JSValue>;
bool relative=false;double baseline=0;
JSValue arg(const Args& a,std::size_t i){return i<a.size()?a[i]:JSValue{};}
void own(JSValue object,const std::string& key,const JSValue& value){JSPropertyDescriptor d;d.value=value;d.writable=d.enumerable=d.configurable=true;object.defineProperty(key,d);}
JSValue array(const Args& values={}){auto out=JSValue::array();for(std::size_t i=0;i<values.size();++i)own(out,std::to_string(i),values[i]);return out;}
JSValue call(const JSValue& object,const char* key,const Args& args={}){return object.get(key).call(object,args);}
void push(const JSValue& target,const JSValue& value){call(target,"push",{value});}
Args values(const JSValue& value){Args out;for(unsigned i=0;i<value.length();++i)out.push_back(value.get(std::to_string(i)));return out;}
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
void observe(const JSValue& state,JSValue object,const char* key,const std::string& label){const auto value=object.get(key);JSPropertyDescriptor d;d.configurable=d.enumerable=true;d.getter=JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue{event(c[0],{"get",c[2]});if(c[0].get("r").get("throwAt").sameValue(c[2]))throw JSTypeError("get");return c[1];},{state,value,label});object.defineProperty(key,d);}
JSValue metadata(){const auto module=nativeColorSpaceNodeModule(),c=module.get("default"),p=c.get("prototype");auto n=c.construct({1,2,3});n.set("id",100);auto out=JSValue::object(),exports=array(),fields=array(),methods=array();
 for(const auto& key:module.keys())push(exports,key);
 for(const auto& key:n.ownPropertyNames()){const auto d=n.getOwnPropertyDescriptor(key).value();push(fields,array({key,normal(n.get(key)),d.writable.value_or(false),d.enumerable.value_or(false),d.configurable.value_or(false)}));}
 for(const auto& key:p.ownPropertyNames()){const auto d=p.getOwnPropertyDescriptor(key).value();const auto v=d.value.value_or(JSValue{});push(methods,array({key,normal(v),v.isCallable()?v.get("name"):JSValue(nullptr),v.isCallable()?v.get("length"):JSValue(nullptr),d.writable.value_or(false),d.enumerable.value_or(false),d.configurable.value_or(false)}));}
 own(out,"exports",exports);own(out,"name",c.get("name"));own(out,"length",c.get("length"));own(out,"type",c.get("type"));own(out,"parent",c.getPrototype().get("name"));own(out,"fields",fields);own(out,"methods",methods);return out;
}
JSValue callback(const JSValue& state,const JSValue& management,const char* name){return JSValue::function(+[](const JSValue& self,const Args& args,const Args& c)->JSValue{
 const auto r=c[0].get("r"),name=c[2];Args row{"call",name,self.sameValue(c[1])};for(const auto& value:args)row.push_back(normal(value));event(c[0],row);
 if(r.get("throwAt").strictEquals(name.toString()+"()"))throw JSTypeError("call");
 if(name.strictEquals("getTransfer"))return arg(args,0).strictEquals("source")?(r.get("sourceTransfer").toBoolean()?JSValue("srgb"):JSValue("linear")):(r.get("targetTransfer").toBoolean()?JSValue("srgb"):JSValue("linear"));
 if(name.strictEquals("getPrimaries"))return r.get("primariesDiffer").toBoolean()&&arg(args,0).strictEquals("target")?JSValue("B"):JSValue("A");
 if(name.strictEquals("_getMatrix"))return call(arg(args,0),"set",{1,2,3,4,5,6,7,8,9});return {};
 },{state,management,name});}
JSValue protocol(const JSValue& r,const JSValue& tsl,JSValue management){
 auto state=JSValue::object();own(state,"r",r);own(state,"events",array());const std::vector<std::string> keys{"enabled","workingColorSpace","getTransfer","getPrimaries","_getMatrix"};std::vector<JSPropertyDescriptor> saved;for(const auto& key:keys)saved.push_back(management.getOwnPropertyDescriptor(key).value());
 auto restore=[&]{for(std::size_t i=0;i<keys.size();++i)management.defineProperty(keys[i],saved[i]);};
 try{
  own(management,"enabled",r.getOwnPropertyDescriptor("enabled").has_value()?r.get("enabled"):JSValue(true));own(management,"workingColorSpace","working");for(const auto* name:{"getTransfer","getPrimaries","_getMatrix"})own(management,name,callback(state,management,name));
  auto color=nativeNodeConstructor().construct({"vec4"});color.set("fixtureTag","color");for(const auto* key:{"rgb","a"}){JSPropertyDescriptor d;d.configurable=true;d.getter=JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue{const auto label="color."+c[2].toString();event(c[0],{"get",label});if(c[0].get("r").get("throwAt").strictEquals(label))throw JSTypeError("color");return c[2].strictEquals("rgb")?c[1].get("vec3").call({}, {.1,.2,.3}):c[1].get("float").call({}, {.4});},{state,tsl,key});color.defineProperty(key,d);}
  auto n=nativeColorSpaceNodeConstructor().construct({color,r.getOwnPropertyDescriptor("source").has_value()?r.get("source"):JSValue("source"),r.getOwnPropertyDescriptor("target").has_value()?r.get("target"):JSValue("target")});n.set("fixtureTag","self");
  auto context=JSValue::object(),renderer=JSValue::object(),builder=JSValue::object();own(context,"outputColorSpace",r.get("contextOutput"));own(renderer,"outputColorSpace","renderer");own(builder,"fixtureTag","builder");own(builder,"context",context);own(builder,"renderer",renderer);
  for(const auto& key:keys)observe(state,management,key.c_str(),"management."+key);for(const auto* key:{"colorNode","source","target","resolveColorSpace"})observe(state,n,key,std::string("self.")+key);for(const auto* key:{"context","renderer"})observe(state,builder,key,std::string("builder.")+key);observe(state,context,"outputColorSpace","context.outputColorSpace");observe(state,renderer,"outputColorSpace","renderer.outputColorSpace");
  auto out=JSValue::object();own(out,"result",attempt([&]{return r.get("method").strictEquals("resolveColorSpace")?call(n,"resolveColorSpace",{builder,r.get("space")}):call(n,"setup",{builder});}));own(out,"events",state.get("events"));restore();return out;
 }catch(...){restore();throw;}
}
JSValue color(const JSValue& tsl){return tsl.get("vec4").call({}, {.1,.2,.3,.4});}
JSValue factory(const JSValue& r,const JSValue& tsl){const auto input=color(tsl);Args args=values(r.get("args"));JSValue result;if(r.get("chaining").toBoolean())result=input.get(r.get("name")).call(input,args);else{args.insert(args.begin(),input);result=nativeColorSpaceNodeModule().get(r.get("name")).call({},args);}auto out=JSValue::object();own(out,"result",normal(result));own(out,"sameInput",result.get("colorNode").sameValue(input));return out;}
JSValue graph(const JSValue& r,const JSValue& tsl){auto renderer=JSValue::object();renderer.set("outputColorSpace","srgb");const auto node=nativeColorSpaceNodeConstructor().construct({color(tsl),r.get("source"),r.get("target")}),builder=nativeNodeBuilderConstructor().construct({nullptr,renderer,nullptr});return normal(call(node,"setup",{builder}));}
JSValue integration(const JSValue& r,const JSValue& tsl){
 auto renderer=JSValue::object(),debug=JSValue::object(),diagnostics=JSValue::object();diagnostics.set("keywords",false);debug.set("diagnostics",diagnostics);renderer.set("debug",debug);renderer.set("coordinateSystem",2001);renderer.set("outputColorSpace","srgb");
 auto builder=nativeNodeBuilderConstructor().construct({nullptr,renderer,nullptr});builder.set("shaderStage","fragment");const auto node=nativeColorSpaceNodeConstructor().construct({color(tsl),r.get("source"),r.get("target")});auto results=array();
 for(const auto* stage:{"setup","analyze","generate"}){builder.set("buildStage",stage);push(results,attempt([&]{const auto value=call(node,"build",{builder,"vec4"});return value.isObject()&&value.get("isNode").toBoolean()?JSValue("#"+value.get("type").toString()):value;}));}
 auto out=JSValue::object();own(out,"values",results);own(out,"flow",normal(builder.get("flow")));own(out,"stackRestored",call(tsl,"getCurrentStack").isNull());return out;
}
}
int main(){try{const std::string text(std::istreambuf_iterator<char>(std::cin),{});const auto requests=Animation_detail::parseJSON(text);const auto builder=nativeNodeBuilderConstructor(),module=nativeColorSpaceNodeModule(),tsl=nativeTSLCore(),management=nativeColorManagement();auto out=array();
 for(unsigned i=0;i<requests.length();++i){const auto r=requests.get(std::to_string(i)),kind=r.get("kind");relative=r.get("relativeNodeIds").toBoolean();baseline=nativeNodeConstructor().construct().get("id").toNumber();JSValue result;if(kind.strictEquals("metadata"))result=metadata();else if(kind.strictEquals("protocol"))result=protocol(r,tsl,management);else if(kind.strictEquals("factory"))result=factory(r,tsl);else if(kind.strictEquals("graph"))result=graph(r,tsl);else result=integration(r,tsl);push(out,result);}
 std::cout<<out.toJSON()->dump();return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
