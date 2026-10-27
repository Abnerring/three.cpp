#include "animation/AnimationUtils.h"
#include "nodes/accessors/ReferenceBaseNode.h"
#include "nodes/accessors/ReferenceElementNode.h"
#include "nodes/accessors/RendererReferenceNode.h"
#include "nodes/display/ToneMappingNode.h"
#include "nodes/utils/ArrayElementNode.h"
#include "nodes/core/Node.h"
#include "nodes/core/NodeBuilder.h"
#include "nodes/core/UniformGroupNode.h"
#include "nodes/tsl/TSLCore.h"
#include "utils.h"
#include <cmath>
#include <iostream>
#include <limits>

namespace {
using Args=std::vector<JSValue>;
JSValue arg(const Args& a,std::size_t i){return i<a.size()?a[i]:JSValue{};}
void own(JSValue o,const std::string& k,const JSValue& v){JSPropertyDescriptor d;d.value=v;d.writable=d.enumerable=d.configurable=true;o.defineProperty(k,d);}
JSValue call(const JSValue& o,const char* k,const Args& a={}){return o.get(k).call(o,a);}
JSValue array(const Args& values={}){auto a=JSValue::array();for(const auto& v:values)call(a,"push",{v});return a;}
JSValue normal(const JSValue& v,std::vector<JSValue>* seen=nullptr){
    if(v.isUndefined())return "#undefined";if(v.isSymbol())return "#symbol";if(v.isCallable())return "#function";
    if(v.type()==JSValue::Type::Number){if(std::isnan(v.number()))return "#NaN";if(std::isinf(v.number()))return v.number()>0?"#Infinity":"#-Infinity";if(v.number()==0&&std::signbit(v.number()))return "#-0";}
    if(!v.isObject())return v;if(v.get("_tag").toBoolean())return "@"+v.get("_tag").toString();std::vector<JSValue> local;if(!seen)seen=&local;for(const auto& p:*seen)if(p.sameValue(v))return "#repeat";seen->push_back(v);
    auto o=v.type()==JSValue::Type::Array?JSValue::array(v.length()):JSValue::object();for(const auto& k:v.keys())own(o,k,normal(v.get(k),seen));seen->pop_back();return o;
}
JSValue failure(std::exception_ptr p){try{std::rethrow_exception(p);}catch(const JSTypeError&){return "#TypeError";}catch(const JSRangeError&){return "#RangeError";}catch(const std::exception& e){auto o=JSValue::object();o.set("nativeError",e.what());return o;}}
JSValue relativeIntegrationIds(JSValue value,double baseline){if(!value.isObject())return value;for(const auto& key:value.keys()){const auto field=value.get(key);if(key=="id"&&value.get("isNode").strictEquals(true)&&field.toNumber()>baseline)value.set(key,field.toNumber()-baseline);else if(field.isObject())value.set(key,relativeIntegrationIds(field,baseline));}return value;}
JSValue module(const JSValue& name){if(name.strictEquals("ReferenceBaseNode"))return nativeReferenceBaseNodeModule();if(name.strictEquals("ReferenceElementNode"))return nativeReferenceElementNodeModule();if(name.strictEquals("RendererReferenceNode"))return nativeRendererReferenceNodeModule();return nativeToneMappingNodeModule();}
void event(const JSValue& s,const Args& a){if(s.get("active").toBoolean())call(s.get("events"),"push",{array(a)});}
void checkpoint(const JSValue& s,const std::string& label){event(s,{label});if(s.get("active").toBoolean()&&s.get("request").get("throwAt").strictEquals(label))throw JSTypeError(label);}
JSValue tagged(const char* tag){auto o=JSValue::object();o.set("_tag",tag);return o;}
JSValue decode(const JSValue& v){
    if(!v.isObject())return v;const auto tag=v.get("tag");if(tag.strictEquals("undefined"))return {};if(tag.strictEquals("NaN"))return std::numeric_limits<double>::quiet_NaN();if(tag.strictEquals("symbol"))return JSValue::symbol("fixture");
    if(tag.strictEquals("node")){auto n=nativeNodeConstructor().construct({v.has("nodeType")?v.get("nodeType"):JSValue("float")});n.set("_tag",v.get("name"));return n;}
    if(v.type()==JSValue::Type::Array){auto a=array();for(unsigned i=0;i<v.length();++i)call(a,"push",{decode(v.get(std::to_string(i)))});return a;}return v;
}
Args values(const JSValue& v){Args a;for(unsigned i=0;i<v.length();++i)a.push_back(decode(v.get(std::to_string(i))));return a;}
JSValue descriptors(const JSValue& o){auto a=array();for(const auto& k:o.ownPropertyNames()){const auto d=o.getOwnPropertyDescriptor(k).value();const auto v=d.value.value_or(JSValue{});call(a,"push",{array({k,normal(v),v.isCallable()?v.get("name"):JSValue(nullptr),v.isCallable()?v.get("length"):JSValue(nullptr),v.isConstructible(),d.writable.value_or(false),d.enumerable.value_or(false),d.configurable.value_or(false),d.getter?d.getter->get("name"):JSValue(nullptr)})});}return a;}
JSValue metadata(const JSValue& r){const auto m=module(r.get("class")),c=m.get("default");auto o=JSValue::object(),entries=array();for(const auto& k:m.ownPropertyNames()){const auto v=m.get(k);call(entries,"push",{array({k,normal(v),v.isCallable()?v.get("name"):JSValue(nullptr),v.isCallable()?v.get("length"):JSValue(nullptr),v.isConstructible()})});}o.set("exports",entries);o.set("statics",descriptors(c));o.set("prototype",descriptors(c.get("prototype")));o.set("parent",c.getPrototype().get("name"));o.set("prototypeParent",c.get("prototype").getPrototype().get("constructor").get("name"));return o;}
JSValue factory(const JSValue& r){auto o=JSValue::object();const auto baseline=nativeNodeConstructor().construct().get("id").number();try{const auto args=values(r.get("args"));const auto m=module(r.get("class")),fn=m.get(r.get("name"));const auto n=r.get("construct").toBoolean()||(r.get("name").strictEquals("default")&&!r.get("callClass").toBoolean())?fn.construct(args):fn.call({},args);o.set("result",normal(n));o.set("fields",descriptors(n));o.set("prototypeMatches",n.getPrototype().sameValue(m.get("default").get("prototype")));}catch(...){o.set("error",failure(std::current_exception()));}o.set("allocationDelta",nativeNodeConstructor().construct().get("id").number()-baseline);return o;}
void observe(const JSValue& s,JSValue o,const char* key,const std::string& label){
    auto slot=JSValue::object();slot.set("value",o.get(key));JSPropertyDescriptor d;d.enumerable=d.configurable=true;
    d.getter=JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue{checkpoint(c[0],"get:"+c[2].string());return c[1].get("value");},{s,slot,label});
    d.setter=JSValue::function(+[](const JSValue&,const Args& a,const Args& c)->JSValue{checkpoint(c[0],"set:"+c[2].string());auto slot=c[1];slot.set("value",arg(a,0));return {};},{s,slot,label});o.defineProperty(key,d);
}
JSValue callback(const JSValue& s,const std::string& label){return JSValue::function(+[](const JSValue& self,const Args& a,const Args& c)->JSValue{
    auto s=c[0];const auto label=c[1].string();Args row{label};for(const auto& v:a)row.push_back(normal(v));event(s,row);if(s.get("request").get("throwAt").strictEquals(label))throw JSTypeError(label);
    const auto r=s.get("request");if(label=="uniform.getNodeType"||label=="ref.getNodeType")return "vec3";if(label=="ref.build")return "referenceSnippet";if(label=="index.getNodeType")return r.has("indexType")?r.get("indexType"):JSValue("float");if(label=="index.build")return "indexSnippet";
    if(label=="isVector")return r.get("vector").toBoolean();if(label=="isInteger")return r.get("integer").toBoolean();if(label=="format")return array({"format",arg(a,0),arg(a,1),arg(a,2)});
    if(label=="getToneMappingFunction"){const auto mode=r.get("function");if(mode.strictEquals("null"))return nullptr;if(mode.strictEquals("undefined"))return {};if(mode.strictEquals("number"))return 5;return s.get("toneFunction");}
    if(label=="toneFunction"){event(s,{"tone.this",self.isUndefined()});return r.has("mapped")?decode(r.get("mapped")):JSValue(.25);}return {};
},{s,label});}
JSValue state(const JSValue& r){auto s=JSValue::object();s.set("request",r);s.set("events",array());s.set("active",true);return s;}
JSValue referenceProtocol(const JSValue& r){
    auto s=state(r),root=tagged("root"),nested=tagged("nested"),other=tagged("other"),builder=tagged("builder"),group=tagged("group"),fake=tagged("uniform");nested.set("b",decode(r.get("value")));root.set("a",nested);other.set("a",nested);builder.set("object",other);builder.set("renderer",other);fake.set("getNodeType",callback(s,"uniform.getNodeType"));
    const auto C=module(r.get("class")).get("default");auto n=C.construct({"a.b",r.has("uniformType")?r.get("uniformType"):JSValue("float"),r.get("fallback").toBoolean()?JSValue(nullptr):root});n.set("_tag","node");if(r.get("initialNode").toBoolean())n.set("node",fake);if(r.get("group").toBoolean())n.set("group",group);
    if(r.get("observed").toBoolean()){
        for(const auto* k:{"object","reference","renderer","node","group","uniformType","properties","setNodeType","updateReference","updateValue","getValueFromReference"})observe(s,n,k,std::string("node.")+k);
        observe(s,root,"a","root.a");observe(s,other,"a","other.a");observe(s,nested,"b","nested.b");observe(s,builder,"object","builder.object");observe(s,builder,"renderer","builder.renderer");observe(s,fake,"getNodeType","uniform.getNodeType");
    }
    auto o=JSValue::object();try{const auto op=r.get("operation").string();Args a;if(op=="generateNodeType"||op=="updateReference")a={builder};else if(op=="setNodeType")a={r.has("newType")?r.get("newType"):JSValue("vec2")};else if(op=="setGroup")a={group};else if(op=="getValueFromReference"&&r.get("explicitObject").toBoolean())a={root};else if(op=="element")a={decode(r.get("index"))};o.set("result",normal(call(n,op.c_str(),a)));}catch(...){o.set("error",failure(std::current_exception()));}
    s.set("active",false);o.set("reference",normal(n.get("reference")));o.set("node",normal(n.get("node")));o.set("group",normal(n.get("group")));o.set("events",s.get("events"));return o;
}
JSValue elementProtocol(const JSValue& r){
    auto s=state(r),ref=tagged("ref"),index=tagged("index"),builder=tagged("builder");ref.set("uniformType","vec2");for(const auto* k:{"getNodeType","build"})ref.set(k,callback(s,std::string("ref.")+k));for(const auto* k:{"getNodeType","build"})index.set(k,callback(s,std::string("index.")+k));for(const auto* k:{"isVector","isInteger","format"})builder.set(k,callback(s,k));
    auto n=nativeReferenceElementNodeBinding("default").construct({ref,index});n.set("_tag","element");n.set("getNodeType",callback(s,"uniform.getNodeType"));
    if(r.get("observed").toBoolean()){for(const auto* k:{"node","referenceNode","indexNode","getNodeType"})observe(s,n,k,std::string("element.")+k);for(const auto* k:{"uniformType","getNodeType","build"})observe(s,ref,k,std::string("ref.")+k);for(const auto* k:{"getNodeType","build"})observe(s,index,k,std::string("index.")+k);for(const auto* k:{"isVector","isInteger","format"})observe(s,builder,k,std::string("builder.")+k);}
    auto o=JSValue::object();try{o.set("result",normal(call(n,r.get("operation").string().c_str(),{builder})));}catch(...){o.set("error",failure(std::current_exception()));}s.set("active",false);o.set("events",s.get("events"));return o;
}
JSValue toneProtocol(const JSValue& r){
    auto s=state(r),color=tagged("color"),context=tagged("context"),builder=tagged("builder"),renderer=tagged("renderer"),library=tagged("library"),exposure=tagged("exposure");color.set("rgb",.3);color.set("a",.7);context.set("color",color);library.set("getToneMappingFunction",callback(s,"getToneMappingFunction"));renderer.set("library",library);builder.set("renderer",renderer);builder.set("context",context);s.set("toneFunction",callback(s,"toneFunction"));
    auto n=nativeToneMappingNodeBinding("default").construct({decode(r.get("mapping")),exposure,r.get("givenColor").toBoolean()?color:decode(r.get("colorValue"))});n.set("_tag","node");
    const auto prior=ThreeUtils::getConsoleFunction();ThreeUtils::setConsoleFunction(JSValue::function(+[](const JSValue&,const Args& a,const Args& c)->JSValue{Args row{"console"};for(const auto& v:a)row.push_back(normal(v));event(c[0],row);return {};},{s}));
    if(r.get("observed").toBoolean()){for(const auto* k:{"colorNode","_toneMapping","exposureNode"})observe(s,n,k,std::string("node.")+k);for(const auto* k:{"context","renderer"})observe(s,builder,k,std::string("builder.")+k);observe(s,context,"color","context.color");observe(s,renderer,"library","renderer.library");observe(s,library,"getToneMappingFunction","library.getToneMappingFunction");observe(s,color,"rgb","color.rgb");observe(s,color,"a","color.a");}
    auto o=JSValue::object();try{const auto op=r.get("operation").string();o.set("result",normal(call(n,op.c_str(),op=="setup"?Args{builder}:op=="setToneMapping"?Args{decode(r.get("value"))}:Args{})));}catch(...){o.set("error",failure(std::current_exception()));}s.set("active",false);o.set("mapping",normal(n.get("_toneMapping")));o.set("events",s.get("events"));ThreeUtils::setConsoleFunction(prior);return o;
}
JSValue integration(const JSValue& r){
    auto renderer=JSValue::object(),debug=JSValue::object(),diagnostics=JSValue::object();diagnostics.set("keywords",false);debug.set("diagnostics",diagnostics);renderer.set("debug",debug);renderer.set("coordinateSystem",2000);renderer.set("toneMappingExposure",.75);auto object=JSValue::object();object.set("gain",.25);
    auto builder=nativeNodeBuilderConstructor().construct({object,renderer,nullptr});builder.set("shaderStage","fragment");const bool relative=r.get("relativeIntegrationNodeIds").toBoolean();const double baseline=relative?nativeNodeConstructor().construct().get("id").toNumber():0;auto o=JSValue::object(),results=array();try{JSValue n;if(r.get("case").strictEquals("reference"))n=nativeReferenceBaseNodeBinding("reference").call({}, {"gain","float",object});else if(r.get("case").strictEquals("renderer"))n=nativeRendererReferenceNodeBinding("rendererReference").call({}, {"toneMappingExposure","float"});else {const auto color=nativeTSLCoreBinding("vec4").call({}, {.25,.5,.75,1});n=nativeToneMappingNodeBinding("toneMapping").call({}, {0,1,color});}for(const auto* stage:{"setup","analyze","generate"}){builder.set("buildStage",stage);call(results,"push",{normal(call(n,"build",{builder,r.get("output")}))});}o.set("results",results);o.set("flow",normal(builder.get("flow")));o.set("uniforms",normal(builder.get("uniforms")));}catch(...){o.set("error",failure(std::current_exception()));}return relative?relativeIntegrationIds(o,baseline):o;
}
JSValue fresh(const std::string& name){JSValue n;if(name=="ReferenceBaseNode")n=ReferenceBaseNode("gain","float").object();else if(name=="ReferenceElementNode")n=ReferenceElementNode(nullptr,nullptr).object();else if(name=="RendererReferenceNode")n=RendererReferenceNode("gain","float").object();else n=ToneMappingNode(0).object();auto o=JSValue::object();o.set("node",normal(n));o.set("id",n.get("id"));o.set("type",n.get("type"));o.set("chain",nativeNodeConstructor().get("prototype").get("toneMapping").isCallable());return o;}
}
int main(int argc,char** argv){try{if(argc==3&&std::string(argv[1])=="--fresh"){std::cout<<fresh(argv[2]).toJSON()->dump();return 0;}const std::string input((std::istreambuf_iterator<char>(std::cin)),{});const auto requests=Animation_detail::parseJSON(input);nativeReferenceBaseNodeModule();nativeReferenceElementNodeModule();nativeRendererReferenceNodeModule();nativeToneMappingNodeModule();auto out=array();for(unsigned i=0;i<requests.length();++i){const auto r=requests.get(std::to_string(i)),kind=r.get("kind");call(out,"push",{kind.strictEquals("metadata")?metadata(r):kind.strictEquals("factory")?factory(r):kind.strictEquals("reference")?referenceProtocol(r):kind.strictEquals("element")?elementProtocol(r):kind.strictEquals("tone")?toneProtocol(r):integration(r)});}std::cout<<out.toJSON()->dump();}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
