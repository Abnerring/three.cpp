#include "animation/AnimationUtils.h"
#include "core/JSIterator.h"
#include "nodes/core/Node.h"
#include "nodes/core/NodeBuilder.h"
#include "nodes/core/StackNode.h"
#include "nodes/display/ColorSpaceFunctions.h"
#include "nodes/display/PremultiplyAlphaFunctions.h"
#include "nodes/functions/material/getAlphaHashThreshold.h"
#include "nodes/tsl/TSLCore.h"
#include <cmath>
#include <iostream>
#include <map>

namespace {
using Args=std::vector<JSValue>;
void own(JSValue o,const std::string& k,const JSValue& v){JSPropertyDescriptor d;d.value=v;d.writable=d.enumerable=d.configurable=true;o.defineProperty(k,d);}
JSValue array(const Args& a={}){auto out=JSValue::array();for(std::size_t i=0;i<a.size();++i)own(out,std::to_string(i),a[i]);return out;}
JSValue call(const JSValue& o,const char* key,const Args& args={}){return o.get(key).call(o,args);}
Args values(const JSValue& a){Args out;for(unsigned i=0;i<a.length();++i)out.push_back(a.get(std::to_string(i)));return out;}
bool structuralNodeIds=false;
JSValue normal(const JSValue& v,std::vector<JSValue>* seen=nullptr,std::map<double,unsigned>* ids=nullptr){
 if(v.isUndefined())return "#undefined";if(v.isCallable())return "#function";if(v.isSymbol())return "#symbol";
 if(v.type()==JSValue::Type::Number){if(std::isnan(v.number()))return "#NaN";if(std::isinf(v.number()))return v.number()>0?"#Infinity":"#-Infinity";if(v.number()==0&&std::signbit(v.number()))return "#-0";}
 if(!v.isObject())return v;if(v.get("fixtureTag").toBoolean())return "@"+v.get("fixtureTag").toString();std::vector<JSValue> local;if(!seen)seen=&local;for(std::size_t i=0;i<seen->size();++i)if(v.sameValue((*seen)[i]))return array({"#ref",static_cast<double>(i)});seen->push_back(v);
 std::map<double,unsigned> localIds;if(!ids)ids=&localIds;auto out=v.type()==JSValue::Type::Array?JSValue::array(v.length()):JSValue::object();for(const auto& key:v.keys()){const auto field=v.get(key);if(key=="id"&&v.get("isNode").strictEquals(true)&&field.type()==JSValue::Type::Number&&structuralNodeIds){const auto [it,inserted]=ids->emplace(field.number(),static_cast<unsigned>(ids->size()));own(out,key,array({"#nodeId",static_cast<double>(it->second)}));}else own(out,key,normal(field,seen,ids));}return out;
}
void event(JSValue s,const Args& a){call(s.get("events"),"push",{array(a)});}
JSValue fixture(JSValue s,const std::string& tag){
 auto n=nativeNodeConstructor().construct({"vec4"});n.set("fixtureTag",tag);
 for(const auto* key:{"x","y","z","a","rgb","xyz","xy"}){JSPropertyDescriptor d;d.configurable=true;d.getter=JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue{const auto label=c[1].string();event(c[0],{"get",label});if(c[0].get("r").get("throwAt").strictEquals(label))throw JSTypeError("fixture");return fixture(c[0],label);},{s,tag+"."+key});n.defineProperty(key,d);}
 for(const auto* key:{"mul","add","sub","pow","lessThanEqual","equal","select","div"}){JSPropertyDescriptor d;d.configurable=true;d.getter=JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue{const auto label=c[1].string();event(c[0],{"get",label});if(c[0].get("r").get("throwAt").strictEquals(label))throw JSTypeError("fixture");return JSValue::function(+[](const JSValue&,const Args& a,const Args& c)->JSValue{Args row{"call",c[1]};for(const auto& v:a)row.push_back(normal(v));event(c[0],row);const auto label=c[1].string()+"()";if(c[0].get("r").get("throwAt").strictEquals(label))throw JSTypeError("fixture");return fixture(c[0],label);},c);},{s,tag+"."+key});n.defineProperty(key,d);}
 return n;
}
JSValue inputs(JSValue s,const JSValue& value){
 if(s.get("r").get("iterator").strictEquals("array"))return array({value});
 auto state=JSValue::object();state.set("index",0);auto iterator=JSValue::object();
 for(const auto* key:{"next","return"}){JSPropertyDescriptor d;d.configurable=true;d.getter=JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue{
  const auto key=c[3].string();event(c[0],{"get","iterator."+key});const auto mode=c[0].get("r").get("iterator");if(key=="return"&&mode.strictEquals("returnGetterThrow"))throw JSTypeError("return getter");
  return JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue{
   const auto mode=c[0].get("r").get("iterator");if(c[3].strictEquals("return")){event(c[0],{"return"});if(mode.strictEquals("returnThrow"))throw JSTypeError("return");if(mode.strictEquals("returnPrimitive"))return 1;return JSValue::object();}
   auto state=c[2];state.set("index",state.get("index").toNumber()+1);event(c[0],{"next",state.get("index")});if(mode.strictEquals("nextThrow"))throw JSTypeError("next");auto result=JSValue::object();
   for(const auto* key:{"done","value"}){JSPropertyDescriptor d;d.configurable=d.enumerable=true;d.getter=JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue{event(c[0],{"get","result."+c[2].string()});const auto mode=c[0].get("r").get("iterator");if(c[2].strictEquals("done"))return mode.strictEquals("empty");if(mode.strictEquals("valueThrow"))throw JSTypeError("value");return c[1];},{c[0],c[1],key});result.defineProperty(key,d);}return result;
  },c);
 },{s,value,state,key});iterator.defineProperty(key,d);}
 auto result=JSValue::object();result.set(JSValue::wellKnownSymbol("iterator"),JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue{event(c[0],{"iterator"});return c[1];},{s,iterator}));return result;
}
JSValue metadata(const JSValue& functions){auto out=array();for(const auto& key:functions.keys()){const auto f=functions.get(key);auto row=JSValue::object();own(row,"key",key);own(row,"name",normal(f.get("name")));own(row,"length",normal(f.get("length")));Args names;for(const auto& name:f.ownPropertyNames())names.emplace_back(name);own(row,"own",array(names));own(row,"isFn",f.get("isFn"));own(row,"shaderType",f.get("shaderNode").get("type"));own(row,"nodeType",f.get("shaderNode").get("nodeType"));own(row,"layout",normal(f.get("shaderNode").get("layout")));call(out,"push",{row});}return out;}
JSValue evaluate(const JSValue& r,const JSValue& functions,const JSValue& tsl){
 structuralNodeIds=r.get("structuralNodeIds").toBoolean();
 if(r.get("kind").strictEquals("metadata"))return metadata(functions);
 auto s=JSValue::object();s.set("r",r);s.set("events",array());const auto previous=call(tsl,"getCurrentStack"),stack=nativeStackNodeConstructor().construct();call(tsl,"setCurrentStack",{stack});auto out=JSValue::object();
 try{const auto value=r.get("fixture").toBoolean()?fixture(s,"color"):tsl.get(r.get("type")).call({},values(r.get("values")));const auto f=functions.get(r.get("name"));const auto result=r.get("kind").strictEquals("call")?f.call({}, {value}):f.get("shaderNode").get("jsFunc").call({}, {inputs(s,value)});out.set("result",normal(result));out.set("stack",normal(stack.get("nodes")));}
 catch(const JSTypeError&){out.set("error","#TypeError");out.set("stack",normal(stack.get("nodes")));}catch(const JSRangeError&){out.set("error","#RangeError");out.set("stack",normal(stack.get("nodes")));}catch(const std::exception& e){out.set("error","#Error");out.set("nativeError",e.what());out.set("stack",normal(stack.get("nodes")));}
 call(tsl,"setCurrentStack",{previous});out.set("events",s.get("events"));return out;
}
}
int main(){try{const std::string text(std::istreambuf_iterator<char>(std::cin),{});const auto cases=Animation_detail::parseJSON(text);const auto builder=nativeNodeBuilderConstructor(),tsl=nativeTSLCore(),colors=nativeColorSpaceFunctions(),alpha=nativePremultiplyAlphaFunctions();auto functions=JSValue::object();for(const auto& module:{colors,alpha})for(const auto& key:module.keys())own(functions,key,module.get(key));own(functions,"getAlphaHashThreshold",nativeAlphaHashThreshold());auto out=array();for(unsigned i=0;i<cases.length();++i)call(out,"push",{evaluate(cases.get(std::to_string(i)),functions,tsl)});std::cout<<out.toJSON()->dump();}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
