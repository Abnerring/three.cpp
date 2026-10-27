#include "nodes/core/AssignNode.h"
#include "nodes/core/Node.h"
#include "nodes/core/NodeBuilder.h"
#include "nodes/core/PropertyNode.h"
#include "nodes/core/ContextNode.h"
#include "nodes/tsl/TSLCore.h"
#include "animation/AnimationUtils.h"
#include <iostream>
#include <iterator>

namespace {
using Args=std::vector<JSValue>;
JSValue arg(const Args& a,std::size_t i){return i<a.size()?a[i]:JSValue{};}
JSValue array(const Args& args={}){auto out=JSValue::array();for(const auto& a:args){JSPropertyDescriptor d;d.value=a;d.writable=d.enumerable=d.configurable=true;out.defineProperty(std::to_string(out.length()),d);}return out;}
JSValue call(const JSValue& o,const char* key,const Args& a={}){const auto fn=o.get(key);return fn.call(o,a);}
void own(JSValue o,const std::string& k,const JSValue& v){JSPropertyDescriptor d;d.value=v;d.writable=d.enumerable=d.configurable=true;o.defineProperty(k,d);}
JSValue normal(const JSValue& v){
    if(v.isUndefined())return "#undefined";if(v.isCallable())return "#function";if(v.isSymbol())return "#symbol";if(!v.isObject())return v;
    const auto tag=v.get("_tag");if(tag.toBoolean())return "@"+tag.toString();
    if(v.get("isConstNode").toBoolean())return array({"const",normal(v.get("value")),normal(v.get("nodeType"))});
    if(v.get("isAssignNode").toBoolean())return array({"assign",normal(v.get("targetNode")),normal(v.get("sourceNode"))});
    auto out=v.type()==JSValue::Type::Array?JSValue::array(v.length()):JSValue::object();
    for(const auto& key:v.ownPropertyNames()){const auto d=v.getOwnPropertyDescriptor(key);if(d&&d->enumerable.value_or(false))own(out,key,normal(v.get(key)));}return out;
}
template<class F>JSValue attempt(F body){try{return normal(body());}catch(const JSTypeError&){return "#TypeError";}catch(const JSRangeError&){return "#RangeError";}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return "#Error";}}
void event(JSValue s,const Args& a){call(s.get("events"),"push",{array(a)});}
void observe(JSValue s,JSValue o,const char* key,const std::string& label){const auto value=o.get(key);JSPropertyDescriptor d;d.configurable=d.enumerable=true;d.getter=JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue{event(c[0],{"get",c[2]});if(c[0].get("r").get("throwAt").sameValue(c[2]))throw JSTypeError("get");return c[1];},{s,value,label});o.defineProperty(key,d);}
JSValue fallback(const JSValue& r,const char* key,const JSValue& value){const auto v=r.get(key);return v.isNull()||v.isUndefined()?value:v;}
JSValue special(JSValue s,const JSValue& label,const JSValue& text){auto o=JSValue::object();o.setPrimitiveConverter(JSValue::function(+[](const JSValue&,const Args& a,const Args& c)->JSValue{event(c[0],{"primitive",c[1],arg(a,0)});if(c[0].get("r").get("throwAt").strictEquals("primitive."+c[1].toString()))throw JSTypeError("primitive");return c[2];},{s,label,text}));return o;}
JSValue callback(const JSValue&,const Args& args,const Args& c){
    auto s=c[0];const auto r=s.get("r");const auto name=c[1].string();Args trace{c[1]};for(const auto& a:args)trace.push_back(normal(a));event(s,trace);
    if(r.get("throwAt").sameValue(c[1]))throw JSTypeError("callback");
    if(name=="target.getNodeType"||name=="context.getNodeType")return fallback(r,"targetType","vec2");if(name=="root.getNodeType")return "vec4";if(name=="source.getNodeType")return fallback(r,"sourceType","vec2");
    if(name=="target.getScope")return s.get("scope");if(name=="target.context")return s.get("context");if(name=="root.context")return s.get("rootContext");
    if(name=="context.build")return r.get("snippetObject").toBoolean()?special(s,"target","target"):JSValue("target");if(name=="source.build")return r.get("snippetObject").toBoolean()?special(s,"source","source"):JSValue("source");if(name=="rootContext.build")return r.get("snippetObject").toBoolean()?special(s,"root","root"):JSValue("root");
    if(name=="isAvailable")return r.get("available");if(name=="getTypeLength")return fallback(r,"rootLength",4);
    if(name=="getDataFromNode")return arg(args,0).sameValue(s.get("scope"))?s.get("scopeData"):s.get("data");if(name=="getNodeProperties")return s.get("properties");
    if(name=="getVarFromNode")return s.get("variable");if(name=="getPropertyName")return r.get("snippetObject").toBoolean()?special(s,"property","temp"):JSValue("temp");
    if(name=="addLineFlowCode"){
        const auto lines=s.get("lines").toNumber()+1;s.set("lines",lines);
        if(r.get("clearAfterLine").strictEquals(lines))own(s.get("target"),"components","");
        if(r.get("throwAt").strictEquals("line:"+JSValue(lines).toString()))throw JSTypeError("line");
    }
    if(name=="format")return array(args);return {};
}
JSValue fn(JSValue s,const char* name){return JSValue::function(callback,{s,name});}
JSValue protocol(const JSValue& r){
    auto s=JSValue::object();s.set("r",r);s.set("events",array());s.set("lines",0);
    auto root=JSValue::object();root.set("_tag","root");root.set("getNodeType",fn(s,"root.getNodeType"));root.set("context",fn(s,"root.context"));
    auto target=JSValue::object();target.set("_tag","target");target.set("getScope",fn(s,"target.getScope"));target.set("context",fn(s,"target.context"));target.set("getNodeType",fn(s,"target.getNodeType"));target.set("isSplitNode",r.get("isSplit"));target.set("components",fallback(r,"components","yx"));target.set("node",root);s.set("target",target);
    auto context=JSValue::object();context.set("_tag","context");context.set("build",fn(s,"context.build"));context.set("getNodeType",fn(s,"context.getNodeType"));context.set("node",target);s.set("context",context);
    auto rootContext=JSValue::object();rootContext.set("_tag","rootContext");rootContext.set("build",fn(s,"rootContext.build"));s.set("rootContext",rootContext);
    auto source=JSValue::object();source.set("_tag","source");source.set("build",fn(s,"source.build"));source.set("getNodeType",fn(s,"source.getNodeType"));
    auto n=nativeAssignNodeConstructor().construct({target,source});n.set("_tag","assign");auto scope=JSValue::object();scope.set("_tag","scope");s.set("scope",scope);
    auto scopeData=JSValue::object();if(r.get("readonlyScope").toBoolean()){JSPropertyDescriptor d;d.value=false;d.enumerable=true;scopeData.defineProperty("assign",d);}s.set("scopeData",scopeData);
    auto data=JSValue::object();
    // The repeat fixture owns property:undefined before JSON transport drops it.
    if(r.hasOwn("property")||r.get("repeat").strictEquals(2))data.set("propertyName",r.get("property"));
    if(r.get("observeProperty").toBoolean()){
        auto state=JSValue::object();state.set("value",data.get("propertyName"));JSPropertyDescriptor d;d.configurable=d.enumerable=true;
        d.getter=JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue{event(c[0],{"getProperty"});if(c[0].get("r").get("throwAt").strictEquals("getProperty"))throw JSTypeError("get");return c[1].get("value");},{s,state});
        d.setter=JSValue::function(+[](const JSValue&,const Args& args,const Args& c)->JSValue{event(c[0],{"setProperty",normal(arg(args,0))});if(c[0].get("r").get("throwAt").strictEquals("setProperty"))throw JSTypeError("set");auto state=c[1];state.set("value",arg(args,0));return {};},{s,state});data.defineProperty("propertyName",d);
    }s.set("data",data);
    auto properties=JSValue::object();if(!r.get("operation").strictEquals("setup")){properties.set("targetNode",context);properties.set("sourceNode",source);}s.set("properties",properties);
    auto variable=JSValue::object();variable.set("_tag","variable");s.set("variable",variable);
    if(r.get("throwWrite").toBoolean()){JSPropertyDescriptor d;d.configurable=d.enumerable=true;d.setter=JSValue::function(+[](const JSValue&,const Args& args,const Args& c)->JSValue{event(c[0],{"write",c[0].get("r").get("throwWrite"),normal(arg(args,0))});throw JSTypeError("write");},{s});properties.defineProperty(r.get("throwWrite"),d);}
    auto builder=JSValue::object();builder.set("_tag","builder");for(const auto* key:{"isAvailable","getTypeLength","getDataFromNode","getNodeProperties","getVarFromNode","getPropertyName","addLineFlowCode","format"})builder.set(key,fn(s,key));
    if(r.get("observed").toBoolean()){
        for(const auto* key:{"targetNode","sourceNode","needsSplitAssign"})observe(s,n,key,std::string("assign.")+key);
        for(const auto* key:{"isSplitNode","components","node","getScope","context","getNodeType"})observe(s,target,key,std::string("target.")+key);
        for(const auto* key:{"node","build","getNodeType"})observe(s,context,key,std::string("context.")+key);
        for(const auto* key:{"getNodeType","context"})observe(s,root,key,std::string("root.")+key);observe(s,rootContext,"build","rootContext.build");
        for(const auto* key:{"build","getNodeType"})observe(s,source,key,std::string("source.")+key);
        for(const auto& key:builder.ownPropertyNames())if(key!="_tag")observe(s,builder,key.c_str(),"builder."+key);
    }
    auto results=array();const auto repeat=fallback(r,"repeat",1).toNumber();for(double i=0;i<repeat;++i)call(results,"push",{attempt([&]{return call(n,r.get("operation").string().c_str(),{builder,r.get("output")});})});
    auto out=JSValue::object();out.set("results",results);out.set("events",s.get("events"));out.set("scopeData",normal(scopeData));out.set("data",attempt([&]{return data;}));out.set("properties",normal(properties));return out;
}
JSValue metadata(){
    const auto c=nativeAssignNodeConstructor(),p=c.get("prototype");auto n=c.construct({1,2});n.set("id",100);auto out=JSValue::object(),fields=array(),methods=array(),exports=array();
    for(const auto& k:n.ownPropertyNames()){const auto d=n.getOwnPropertyDescriptor(k).value();call(fields,"push",{array({k,normal(n.get(k)),d.writable.value_or(false),d.enumerable.value_or(false),d.configurable.value_or(false)})});}
    for(const auto& k:p.ownPropertyNames()){const auto d=p.getOwnPropertyDescriptor(k).value();const auto v=d.value.value_or(JSValue{});call(methods,"push",{array({k,normal(v),v.isCallable()?v.get("name"):JSValue(nullptr),v.isCallable()?v.get("length"):JSValue(nullptr),d.writable.value_or(false),d.enumerable.value_or(false),d.configurable.value_or(false)})});}
    for(const auto& k:nativeAssignNodeModule().ownPropertyNames())call(exports,"push",{k});out.set("name",c.get("name"));out.set("arity",c.get("length"));out.set("type",c.get("type"));out.set("parent",c.getPrototype().get("name"));out.set("own",fields);out.set("methods",methods);out.set("exports",exports);return out;
}
JSValue factory(){auto target=nativeNodeConstructor().construct({"float"}),source=nativeNodeConstructor().construct({"float"});target.set("_tag","target");source.set("_tag","source");const auto n=nativeAssignNodeModule().get("assign").call({}, {target,source});auto out=JSValue::object();out.set("value",normal(n));out.set("target",n.get("targetNode").sameValue(target));out.set("source",n.get("sourceNode").sameValue(source));return out;}
JSValue integration(const JSValue& r){
    nativeNodeBuilderConstructor();nativeContextNodeModule();const auto property=nativePropertyNodeModule().get("property");auto renderer=JSValue::object(),debug=JSValue::object(),diagnostics=JSValue::object();diagnostics.set("keywords",false);debug.set("diagnostics",diagnostics);renderer.set("debug",debug);renderer.set("coordinateSystem",2001);
    auto builder=NodeBuilder(nullptr,renderer,nullptr).object();builder.set("shaderStage","fragment");builder.set("isAvailable",JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue{return c[0];},{r.get("available")}));
    const auto root=property.call({}, {"vec3","targetValue"});const auto components=r.get("components");const auto target=components.toBoolean()?root.get(components):root;
    const auto source=nativeTSLCore().get(components.toBoolean()?"vec2":"vec3").call({},components.toBoolean()?Args{1,2}:Args{1,2,3});const auto n=nativeAssignNodeModule().get("assign").call({}, {target,source});auto results=array();
    for(const auto* stage:{"setup","analyze","generate"}){builder.set("buildStage",stage);call(results,"push",{attempt([&]{const auto v=call(n,"build",{builder,r.get("output")});return v.isObject()&&v.get("isNode").toBoolean()?JSValue("#"+v.get("type").toString()):v;})});}
    if(r.get("repeat").toBoolean()){builder.set("buildStage","generate");call(results,"push",{attempt([&]{return call(n,"build",{builder,r.get("output")});})});}
    auto out=JSValue::object();out.set("values",results);out.set("flow",normal(builder.get("flow")));out.set("assign",normal(call(builder,"getDataFromNode",{root}).get("assign")));out.set("contextAssign",normal(builder.get("context").get("assign")));
    const auto targetContext=call(builder,"getNodeProperties",{n}).get("targetNode");out.set("targetContext",!targetContext.isNull()&&!targetContext.isUndefined()&&targetContext.get("isContextNode").strictEquals(true));return out;
}
}
int main(){try{const std::string text(std::istreambuf_iterator<char>(std::cin),{});const auto requests=Animation_detail::parseJSON(text);nativeAssignNodeModule();auto out=array();for(std::uint32_t i=0;i<requests.length();++i){const auto r=requests.get(std::to_string(i)),kind=r.get("kind");call(out,"push",{kind.strictEquals("metadata")?metadata():kind.strictEquals("factory")?factory():kind.strictEquals("protocol")?protocol(r):integration(r)});}std::cout<<out.toJSON()->dump();return 0;}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
