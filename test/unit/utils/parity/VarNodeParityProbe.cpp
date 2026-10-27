#include "animation/AnimationUtils.h"
#include "nodes/core/VarNode.h"
#include "nodes/core/Node.h"
#include "nodes/tsl/TSLCore.h"
#include "utils.h"
#include <iostream>
namespace {
using Args=std::vector<JSValue>;
JSValue arg(const Args& a,std::size_t i){return i<a.size()?a[i]:JSValue{};}
JSValue call(const JSValue& o,const char* k,const Args& a={}){return o.get(k).call(o,a);}
void own(JSValue o,const std::string& k,const JSValue& v){JSPropertyDescriptor d;d.value=v;d.writable=d.enumerable=d.configurable=true;o.defineProperty(k,d);}
JSValue array(const Args& a={}){auto o=JSValue::array();for(const auto& v:a)own(o,std::to_string(o.length()),v);return o;}
JSValue normal(const JSValue& v){if(v.isUndefined())return "#undefined";if(v.isCallable())return "#function";if(v.isSymbol())return "#symbol";if(!v.isObject())return v;if(v.get("_tag").toBoolean())return "@"+v.get("_tag").toString();if(v.get("isConstNode").toBoolean())return array({"const",normal(v.get("value")),v.get("nodeType")});auto o=v.type()==JSValue::Type::Array?JSValue::array():JSValue::object();for(const auto& k:v.keys())own(o,k,normal(v.get(k)));return o;}
void event(JSValue s,const Args& a){call(s.get("events"),"push",{array(a)});}
JSValue attempt(JSValue o,const std::string& method,const Args& a){try{return normal(call(o,method.c_str(),a));}catch(const JSTypeError&){return "#TypeError";}catch(const JSRangeError&){return "#RangeError";}}
JSValue fn(const JSValue& self,const Args& args,const Args& c){auto s=c[0];const auto name=c[1].string();Args trace{name};for(const auto& v:args)trace.push_back(normal(v));event(s,trace);const auto r=s.get("r");if(r.get("throwAt").strictEquals(name))throw JSTypeError(name);
    if(name=="getShared")return r.get("shared").toBoolean()?s.get("shared"):s.get("var");if(name=="_hasStack")return r.get("hasStack");if(name=="getDataFromNode")return arg(args,0).sameValue(s.get("loop"))?s.get("loopData"):s.get("data");
    if(name=="getNodeType")return r.has("type")?r.get("type"):JSValue("vec3");if(name=="getArrayCount")return 4;if(name=="getElementType")return "float";if(name=="getMemberType")return "member:"+arg(args,1).toString();
    if(name=="getBaseStack")return s.get("baseStack");if(name=="isCacheable")return r.get("cacheable");if(name=="isDeterministic")return r.get("deterministic");
    if(name=="getVectorType")return "vec3";if(name=="getVarFromNode")return s.get("nodeVar");if(name=="getPropertyName")return "variable";
    if(name=="generateConstStatement")return "const-decl";if(name=="generateLetStatement")return "let-decl";if(name=="generateVarStatement")return "var-decl";
    if(name=="node.build")return "input-snippet";if(name=="super.build")return "super-snippet";if(name=="shared.build")return "shared-snippet";return {};
}
JSValue callback(JSValue s,const char* name){return JSValue::function(fn,{s,name});}
void observe(JSValue s,JSValue o,const char* key,const std::string& label){const auto v=o.get(key);JSPropertyDescriptor d;d.configurable=d.enumerable=true;d.getter=JSValue::function([](const JSValue&,const Args&,const Args& c)->JSValue{event(c[0],{"get",c[2]});if(c[0].get("r").get("throwAt").sameValue(c[2]))throw JSTypeError("get");return c[1];},{s,v,label});o.defineProperty(key,d);}
JSValue protocol(const JSValue& r){
    auto s=JSValue::object();s.set("r",r);s.set("events",array());auto node=JSValue::object();node.set("_tag","node");
    for(const auto* name:{"getNodeType","getMemberType","getElementType","getArrayCount","isCacheable"})node.set(name,callback(s,name));node.set("build",callback(s,"node.build"));
    auto n=nativeVarNodeConstructor().construct({node,r.has("name")?r.get("name"):JSValue(nullptr),r.get("readOnly")});n.set("_tag","var");n.set("intent",r.get("intent"));s.set("var",n);n.set("getShared",callback(s,"getShared"));
    auto shared=JSValue::object();shared.set("_tag","shared");shared.set("build",callback(s,"shared.build"));s.set("shared",shared);
    auto data=JSValue::object();s.set("data",data);if(r.has("forceDeclaration")||(!r.get("operation").strictEquals("build")&&!r.get("operation").strictEquals("generate")))data.set("forceDeclaration",r.get("forceDeclaration"));if(r.has("assign"))data.set("assign",r.get("assign"));
    auto stack=JSValue::object();stack.set("_tag","stack");if(r.get("hasStack").toBoolean())data.set("stack",stack);auto loop=JSValue::object();loop.set("_tag","loop");s.set("loop",loop);auto loopData=JSValue::object();loopData.set("stack",stack);s.set("loopData",loopData);
    auto builder=JSValue::object();builder.set("_tag","builder");builder.set("buildStage",r.has("stage")?r.get("stage"):JSValue("generate"));builder.set("flowBlock",r.get("flowBlock").toBoolean()?JSValue::object():JSValue(nullptr));builder.set("activeStacks",r.get("active").toBoolean()?array({stack}):array());
    auto context=JSValue::object();if(r.get("loop").toBoolean())context.set("nodeLoop",loop);if(r.get("block").toBoolean())context.set("nodeBlock",true);builder.set("context",context);
    auto nodeVar=JSValue::object();nodeVar.set("_tag","nodeVar");nodeVar.set("type","vec3");nodeVar.set("readOnly",r.get("varReadOnly"));nodeVar.set("local",r.get("local"));nodeVar.set("count",3);s.set("nodeVar",nodeVar);
    auto baseStack=JSValue::object();baseStack.set("addToStack",callback(s,"addToStack"));s.set("baseStack",baseStack);
    for(const auto* name:{"getDataFromNode","getBaseStack","isDeterministic","getVectorType","getVarFromNode","getPropertyName","generateConstStatement","generateLetStatement","generateVarStatement","addLineFlowCode"})builder.set(name,callback(s,name));
    if(r.get("operation").strictEquals("generate"))n.set("getNodeType",callback(s,"getNodeType"));
    if(r.get("observed").toBoolean()){
        for(const auto* key:{"node","name","readOnly","intent","getShared","isIntent","isAssign","_hasStack"})observe(s,n,key,std::string("var.")+key);
        for(const auto* key:{"getNodeType","getMemberType","getElementType","getArrayCount","isCacheable","build"})observe(s,node,key,std::string("node.")+key);
        for(const auto* key:{"getDataFromNode","getBaseStack","isDeterministic","getVectorType","getVarFromNode","getPropertyName","generateConstStatement","generateLetStatement","generateVarStatement","addLineFlowCode","context","buildStage","flowBlock","activeStacks"})observe(s,builder,key,std::string("builder.")+key);
    }
    auto proto=nativeNodeConstructor().get("prototype");const auto old=proto.getOwnPropertyDescriptor("build").value();proto.set("build",callback(s,"super.build"));
    const auto previous=ThreeUtils::getConsoleFunction();ThreeUtils::setConsoleFunction(JSValue::function([](const JSValue&,const Args& a,const Args& c)->JSValue{Args trace{"console"};for(const auto& v:a)trace.push_back(normal(v));event(c[0],trace);return {};},{s}));
    auto out=JSValue::object();out.set("result",attempt(n,r.get("operation").string(),{builder,"member","extra"}));out.set("events",s.get("events"));out.set("data",normal(data));
    ThreeUtils::setConsoleFunction(previous);proto.defineProperty("build",old);return out;
}
JSValue factory(const JSValue& r){
    auto node=nativeNodeConstructor().construct({"float"});node.set("_tag","node");const auto mod=nativeVarNodeModule();const auto value=mod.get(r.get("name")).call({}, {node,"myVar"});auto out=JSValue::object();
    for(const auto* key:{"type","nodeType","name","global","parents","isVarNode","readOnly","intent"})out.set(key,normal(value.get(key)));out.set("nodeIdentity",value.get("node").sameValue(node));return out;
}
JSValue metadata(){const auto c=nativeVarNodeConstructor(),p=c.get("prototype");auto n=c.construct({7});n.set("id",100);auto out=JSValue::object(),ownFields=array(),methods=array();
    for(const auto& key:n.ownPropertyNames()){const auto d=n.getOwnPropertyDescriptor(key).value();call(ownFields,"push",{array({key,normal(n.get(key)),d.writable.value_or(false),d.enumerable.value_or(false),d.configurable.value_or(false)})});}
    for(const auto& key:p.ownPropertyNames()){const auto d=p.getOwnPropertyDescriptor(key).value();const auto v=d.value.value_or(JSValue{});call(methods,"push",{array({key,normal(v),v.isCallable()?v.get("name"):JSValue{},v.isCallable()?v.get("length"):JSValue{},d.writable.value_or(false),d.enumerable.value_or(false),d.configurable.value_or(false)})});}
    out.set("own",ownFields);out.set("methods",methods);out.set("parent",p.getPrototype().get("constructor").get("name"));out.set("arity",c.get("length"));return out;
}
}
int main(){try{const std::string text(std::istreambuf_iterator<char>(std::cin),{});const auto requests=Animation_detail::parseJSON(text);nativeVarNodeModule();auto out=array();for(std::uint32_t i=0;i<requests.length();++i){const auto r=requests.get(std::to_string(i));call(out,"push",{r.get("kind").strictEquals("metadata")?metadata():r.get("kind").strictEquals("factory")?factory(r):protocol(r)});}std::cout<<out.toJSON()->dump();}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
