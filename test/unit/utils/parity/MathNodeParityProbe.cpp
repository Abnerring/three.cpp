#include "animation/AnimationUtils.h"
#include "nodes/math/MathNode.h"
#include "nodes/core/Node.h"
#include "nodes/core/NodeBuilder.h"
#include "nodes/tsl/TSLCore.h"
#include "core/JSSet.h"
#include "utils.h"
#include <iostream>
#include <cmath>
#include <limits>
namespace {
using Args=std::vector<JSValue>;
JSValue arg(const Args& a,std::size_t i){return i<a.size()?a[i]:JSValue{};}
void own(JSValue o,const std::string& k,const JSValue& v){JSPropertyDescriptor d;d.value=v;d.writable=d.enumerable=d.configurable=true;o.defineProperty(k,d);}
JSValue array(const Args& args={}){auto o=JSValue::array();for(std::size_t i=0;i<args.size();++i)own(o,std::to_string(i),args[i]);return o;}
JSValue call(const JSValue& o,const char* k,const Args& a={}){return o.get(k).call(o,a);}
// Relative integration mode rebases allocations after a real Node sentinel;
// existing cache nodes retain their original ids and remain independently checked.
JSValue normal(const JSValue& v,double baseline=0,std::vector<JSValue>* seen=nullptr,bool relativeAllocated=false){
    if(v.isUndefined())return "#undefined";if(v.isSymbol())return "#symbol";if(v.isCallable())return "#function";
    if(v.type()==JSValue::Type::Number){if(std::isnan(v.number()))return "#NaN";if(std::isinf(v.number()))return v.number()>0?"#Infinity":"#-Infinity";if(v.number()==0&&std::signbit(v.number()))return "#-0";}
    if(!v.isObject())return v;if(v.get("_tag").toBoolean())return "@"+v.get("_tag").toString();
    std::vector<JSValue> local;if(!seen)seen=&local;for(const auto& x:*seen)if(x.sameValue(v))return "#repeat";seen->push_back(v);
    auto out=v.type()==JSValue::Type::Array?JSValue::array(v.length()):JSValue::object();for(const auto& key:v.keys()){
        const auto field=v.get(key);own(out,key,key=="id"&&v.get("isNode").strictEquals(true)?JSValue(relativeAllocated&&field.toNumber()<=baseline?field.toNumber():field.toNumber()-baseline):normal(field,baseline,seen,relativeAllocated));
    }
    seen->pop_back();return out;
}
void event(JSValue s,const Args& args){call(s.get("events"),"push",{array(args)});}
JSValue decode(const JSValue& v){if(!v.isObject())return v;if(v.get("tag").strictEquals("undefined"))return {};if(v.get("tag").strictEquals("NaN"))return std::numeric_limits<double>::quiet_NaN();if(v.get("tag").strictEquals("node")){auto n=nativeNodeConstructor().construct({v.has("type")?v.get("type"):JSValue("float")});n.set("_tag",v.get("name"));return n;}if(v.type()==JSValue::Type::Array){auto a=array();for(unsigned i=0;i<v.length();++i)own(a,std::to_string(i),decode(v.get(std::to_string(i))));return a;}return v;}
Args values(const JSValue& v){Args a;for(unsigned i=0;i<v.length();++i)a.push_back(decode(v.get(std::to_string(i))));return a;}
JSValue callback(const JSValue&,const Args& args,const Args& c){
    auto s=c[0];const auto name=c[1].string();const auto r=s.get("request");Args trace{name};for(const auto& v:args)trace.push_back(normal(v));event(s,trace);if(r.get("throwAt").strictEquals(name))throw JSTypeError(name);
    if(name=="a.getNodeType")return r.get("aType");if(name=="b.getNodeType")return r.get("bType");if(name=="c.getNodeType")return r.get("cType");
    if(name=="a.build"||name=="b.build"||name=="c.build")return name.substr(0,1)+":"+arg(args,1).toString();
    if(name=="isMatrix"){const auto t=arg(args,0);return t.type()==JSValue::Type::String&&t.string().starts_with("mat");}
    if(name=="getTypeLength"){const auto t=arg(args,0);if(t.isNull())return 0;if(r.get("lengths").isObject()&&r.get("lengths").has(t))return r.get("lengths").get(t);const auto text=t.toString();if(text.starts_with("mat"))return 16;if(text.find("vec")!=std::string::npos)return text.back()-'0';return 1;}
    if(name=="getPromotedComponentType")return r.has("component")?r.get("component"):JSValue("float");
    if(name=="changeComponentType")return arg(args,0).toString()+":"+arg(args,1).toString();
    if(name=="getComponentType")return "float";
    if(name=="getNodeProperties")return s.get("properties");if(name=="getNodeType")return "vec3";
    if(name=="getMethod")return "native:"+arg(args,0).toString();
    if(name=="format")return array({"format",arg(args,0),arg(args,1),normal(arg(args,2))});
    if(name=="super.setup")return "super-setup";if(name=="super.generate")return "super-generate";
    if(name=="super.serialize"){auto data=arg(args,0);data.set("superValue",3);return {};}
    if(name=="super.deserialize")return {};
    return {};
}
JSValue cb(JSValue state,const std::string& name){return JSValue::function(callback,{state,name});}
void observe(JSValue s,JSValue o,const std::string& key,const std::string& label){const auto value=o.get(key);JSPropertyDescriptor d;d.enumerable=d.configurable=true;d.getter=JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue{event(c[0],{"get",c[2]});if(c[0].get("request").get("throwAt").sameValue(c[2]))throw JSTypeError("get");return c[1];},{s,value,label},"get",0);o.defineProperty(key,d);}
JSValue protocol(const JSValue& r){
    auto s=JSValue::object();s.set("request",r);s.set("events",array());auto a=JSValue::object(),b=JSValue::object(),c=JSValue::object();
    for(auto* value:{&a,&b,&c}){const auto label=value==&a?"a":value==&b?"b":"c";value->set("_tag",label);value->set("getNodeType",cb(s,std::string(label)+".getNodeType"));value->set("build",cb(s,std::string(label)+".build"));}
    auto node=nativeMathNodeConstructor().construct({r.get("method"),a,r.get("bNull").toBoolean()?JSValue(nullptr):b,r.get("cNull").toBoolean()?JSValue(nullptr):c});node.set("_tag","math");node.set("getNodeType",cb(s,"getNodeType"));node.set("stackTrace","trace");
    auto builder=JSValue::object();builder.set("_tag","builder");for(const auto* name:{"isMatrix","getTypeLength","getPromotedComponentType","changeComponentType","getNodeProperties","getMethod","getComponentType","format"})builder.set(name,cb(s,name));
    auto renderer=JSValue::object();renderer.set("coordinateSystem",r.get("coordinate"));builder.set("renderer",renderer);builder.set("shaderStage",r.get("stage"));
    auto properties=JSValue::object();properties.set("outputNode",r.get("outputNode"));s.set("properties",properties);
    if(r.get("observed").toBoolean()){
        for(const auto* key:{"method","aNode","bNode","cNode","getNodeType","getInputType","stackTrace"})observe(s,node,key,std::string("math.")+key);
        for(const auto& [value,label]:std::initializer_list<std::pair<JSValue,const char*>>{{a,"a"},{b,"b"},{c,"c"}})for(const auto* key:{"build","getNodeType"})observe(s,value,key,std::string(label)+"."+key);
        for(const auto* key:{"isMatrix","getTypeLength","getPromotedComponentType","changeComponentType","getNodeProperties","getMethod","getComponentType","format","renderer","shaderStage"})observe(s,builder,key,std::string("builder.")+key);
        observe(s,renderer,"coordinateSystem","renderer.coordinateSystem");observe(s,properties,"outputNode","properties.outputNode");
    }
    const auto ctor=nativeMathNodeConstructor();auto base=nativeNodeConstructor().get("prototype");auto out=JSValue::object();const auto previousConsole=ThreeUtils::getConsoleFunction();
    ThreeUtils::setConsoleFunction(JSValue::function(+[](const JSValue&,const Args& args,const Args& c)->JSValue{Args trace{"console"};for(const auto& v:args)trace.push_back(normal(v));event(c[0],trace);return {};},{s}));
    const auto operation=r.get("operation").string();std::optional<JSPropertyDescriptor> oldSuper;std::optional<JSPropertyDescriptor> oldStatic;std::optional<JSPropertyDescriptor> oldHas;
    if(operation=="setup"||operation=="generate"||operation=="serialize"||operation=="deserialize"){oldSuper=base.getOwnPropertyDescriptor(operation);base.set(operation,cb(s,"super."+operation));}
    if(r.has("staticKey")){const auto key=r.get("staticKey").string();oldStatic=ctor.getOwnPropertyDescriptor(key);JSPropertyDescriptor d;d.configurable=true;d.getter=JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue{event(c[0],{"static",c[1]});return c[2];},{s,key,r.get("staticValue")});auto mutableCtor=ctor;mutableCtor.defineProperty(key,d);}
    auto setProto=nativeSetConstructor().get("prototype");
    if(r.has("setHas")){oldHas=setProto.getOwnPropertyDescriptor("has");setProto.set("has",JSValue::function(+[](const JSValue& self,const Args& args,const Args& c)->JSValue{event(c[0],{"set.has",isNativeSet(self),normal(arg(args,0))});return c[0].get("request").get("setHas");},{s}));}
    auto data=JSValue::object();data.set("method",r.has("dataMethod")?r.get("dataMethod"):JSValue("restored"));
    try{const auto value=call(node,operation.c_str(),operation=="serialize"||operation=="deserialize"?Args{data}:Args{builder,"out"});out.set("result",normal(value));}
    catch(const JSTypeError&){out.set("result","#TypeError");}catch(const JSRangeError&){out.set("result","#RangeError");}catch(const std::exception& e){out.set("result","#Error");out.set("nativeError",e.what());}
    if(oldSuper)base.defineProperty(operation,*oldSuper);if(oldStatic){auto mutableCtor=ctor;mutableCtor.defineProperty(r.get("staticKey").string(),*oldStatic);}if(oldHas)setProto.defineProperty("has",*oldHas);
    if(operation=="serialize"||operation=="deserialize"){out.set("data",normal(data));try{out.set("method",normal(node.get("method")));}catch(const JSTypeError&){out.set("method","#TypeError");}}
    out.set("events",s.get("events"));ThreeUtils::setConsoleFunction(previousConsole);return out;
}
JSValue descriptors(const JSValue& value){auto out=array();for(const auto& key:value.ownPropertyNames()){const auto d=value.getOwnPropertyDescriptor(key).value();auto field=d.value.value_or(JSValue{});call(out,"push",{array({key,normal(field),field.isCallable()?field.get("name"):JSValue(nullptr),field.isCallable()?field.get("length"):JSValue(nullptr),d.writable.value_or(false),d.enumerable.value_or(false),d.configurable.value_or(false),d.getter&&d.getter->isCallable()?d.getter->get("name"):JSValue(nullptr)})});}return out;}
JSValue metadata(){const auto module=nativeMathNodeModule(),ctor=nativeMathNodeConstructor(),proto=ctor.get("prototype");auto n=ctor.construct({"sin",7});n.set("id",100);auto out=JSValue::object();out.set("instance",descriptors(n));out.set("methods",descriptors(proto));out.set("statics",descriptors(ctor));out.set("parent",proto.getPrototype().get("constructor").get("name"));out.set("arity",ctor.get("length"));auto keys=array(),functions=array();for(const auto& key:module.ownPropertyNames()){call(keys,"push",{key});const auto v=module.get(key);if(v.isCallable())call(functions,"push",{array({key,v.get("name"),normal(v.get("length")),v.isConstructible()})});}out.set("keys",keys);out.set("functions",functions);out.set("PIIdentity",module.get("PI2").sameValue(module.get("TWO_PI")));out.set("randLayout",normal(module.get("rand").get("shaderNode").get("layout")));return out;}
JSValue construct(const JSValue& r){const auto baseline=nativeNodeConstructor().construct().get("id").number();auto out=JSValue::object();try{const auto a=values(r.get("args"));const auto n=nativeMathNodeConstructor().construct(a);out.set("result",normal(n,baseline));try{nativeMathNodeConstructor().call({},a);out.set("call","ok");}catch(const JSTypeError&){out.set("call","#TypeError");}}catch(const JSTypeError&){out.set("result","#TypeError");}return out;}
JSValue factory(const JSValue& r){const auto baseline=nativeNodeConstructor().construct().get("id").number();auto out=JSValue::object(),events=array();const auto previous=ThreeUtils::getConsoleFunction();ThreeUtils::setConsoleFunction(JSValue::function(+[](const JSValue&,const Args& args,const Args& c)->JSValue{call(c[0],"push",{normal(array(args))});return {};},{events}));try{const auto args=values(r.get("args"));auto value=r.get("name").strictEquals("randBody")?nativeMathNodeModule().get("rand").get("shaderNode").get("jsFunc").call({}, {array(args)}):nativeMathNodeModule().get(r.get("name")).call({},args);out.set("result",normal(value,baseline));}catch(const JSTypeError&){out.set("result","#TypeError");}catch(const JSRangeError&){out.set("result","#RangeError");}catch(const std::exception& e){out.set("result","#Error");out.set("nativeError",e.what());}out.set("events",events);ThreeUtils::setConsoleFunction(previous);return out;}
JSValue integration(const JSValue& r){auto renderer=JSValue::object(),debug=JSValue::object(),diagnostics=JSValue::object();diagnostics.set("keywords",false);debug.set("diagnostics",diagnostics);renderer.set("debug",debug);renderer.set("coordinateSystem",r.get("coordinate"));auto builder=nativeNodeBuilderConstructor().construct({nullptr,renderer,nullptr});builder.set("shaderStage","fragment");const bool relative=r.get("relativeIntegrationNodeIds").toBoolean();const double baseline=relative?nativeNodeConstructor().construct().get("id").number():0;auto out=JSValue::object(),results=array();try{const auto n=nativeMathNodeModule().get(r.get("name")).call({},values(r.get("args")));for(const auto* stage:{"setup","analyze","generate"}){builder.set("buildStage",stage);call(results,"push",{normal(call(n,"build",{builder,r.get("output")}),baseline,nullptr,relative)});}out.set("results",results);out.set("flow",normal(builder.get("flow")));}catch(const JSTypeError&){out.set("error","#TypeError");}catch(const std::exception& e){out.set("error","#Error");out.set("nativeError",e.what());}return out;}
}
int main(){try{const std::string text(std::istreambuf_iterator<char>(std::cin),{});const auto cases=Animation_detail::parseJSON(text);nativeMathNodeModule();auto out=array();for(unsigned i=0;i<cases.length();++i){const auto r=cases.get(std::to_string(i)),kind=r.get("kind");call(out,"push",{kind.strictEquals("metadata")?metadata():kind.strictEquals("construct")?construct(r):kind.strictEquals("factory")?factory(r):kind.strictEquals("integration")?integration(r):protocol(r)});}std::cout<<out.toJSON()->dump();}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
