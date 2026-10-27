#include "animation/AnimationUtils.h"
#include "nodes/core/ArrayNode.h"
#include "nodes/core/IsolateNode.h"
#include "nodes/core/BypassNode.h"
#include "nodes/core/Node.h"
#include "nodes/core/NodeBuilder.h"
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
JSValue array(const Args& a={}){auto o=JSValue::array();for(std::size_t i=0;i<a.size();++i)own(o,std::to_string(i),a[i]);return o;}
JSValue normal(const JSValue& v,double base=0,bool relative=false,std::vector<JSValue>* seen=nullptr){
    if(v.isUndefined())return "#undefined";if(v.isSymbol())return "#symbol";if(v.isCallable())return "#function";
    if(v.type()==JSValue::Type::Number){const auto n=v.number();if(std::isnan(n))return "#NaN";if(std::isinf(n))return n>0?"#Infinity":"#-Infinity";if(n==0&&std::signbit(n))return "#-0";}
    if(!v.isObject())return v;if(v.get("_tag").toBoolean())return "@"+v.get("_tag").toString();std::vector<JSValue> local;if(!seen)seen=&local;for(const auto& x:*seen)if(x.sameValue(v))return "#repeat";seen->push_back(v);
    auto out=v.type()==JSValue::Type::Array?JSValue::array(v.length()):JSValue::object();for(const auto& key:v.keys()){const auto field=v.get(key);own(out,key,relative&&key=="id"&&v.get("isNode").strictEquals(true)&&field.toNumber()>base?JSValue(field.toNumber()-base):normal(field,base,relative,seen));}seen->pop_back();return out;
}
void event(const JSValue& s,const Args& a){call(s.get("events"),"push",{array(a)});}
JSValue failure(std::exception_ptr p){try{std::rethrow_exception(p);}catch(const JSTypeError&){return "#TypeError";}catch(const JSRangeError&){return "#RangeError";}catch(const std::exception& e){auto o=JSValue::object();o.set("nativeError",e.what());return o;}}
JSValue module(const JSValue& name){return name.strictEquals("ArrayNode")?nativeArrayNodeModule():name.strictEquals("IsolateNode")?nativeIsolateNodeModule():nativeBypassNodeModule();}
JSValue decode(const JSValue& v,const JSValue& s){
    if(!v.isObject())return v;const auto tag=v.get("tag");if(tag.strictEquals("undefined"))return {};if(tag.strictEquals("NaN"))return std::numeric_limits<double>::quiet_NaN();if(tag.strictEquals("symbol"))return JSValue::symbol("fixture");if(tag.strictEquals("node")){auto n=nativeNodeConstructor().construct({v.has("nodeType")?v.get("nodeType"):JSValue("float")});n.set("_tag",v.get("name"));return n;}
    if(tag.strictEquals("sparse")){auto a=JSValue::array(static_cast<unsigned>(v.get("length").toNumber()));const auto entries=v.get("entries");for(unsigned i=0;i<entries.length();++i){const auto pair=entries.get(std::to_string(i));own(a,pair.get("0").toString(),decode(pair.get("1"),s));}return a;}
    if(tag.strictEquals("map")){
        auto o=JSValue::object();JSPropertyDescriptor d;d.configurable=true;d.getter=JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue{
            event(c[0],{"map.get"});if(c[0].get("request").get("throwAt").strictEquals("map.get"))throw JSTypeError("map.get");
            return JSValue::function(+[](const JSValue&,const Args& args,const Args& c)->JSValue{event(c[0],{"map.call",args.size()});if(c[0].get("request").get("throwAt").strictEquals("map.call"))throw JSTypeError("map.call");auto values=array();const auto items=c[1].get("items");for(unsigned i=0;i<items.length();++i)call(values,"push",{arg(args,0).call({}, {decode(items.get(std::to_string(i)),c[0]),i})});
                auto mapped=JSValue::object();mapped.set("_tag","mapped");mapped.set("values",values);JSPropertyDescriptor d;d.getter=JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue{event(c[0],{"mapped.length"});if(c[0].get("request").get("throwAt").strictEquals("mapped.length"))throw JSTypeError("mapped.length");return c[1];},{c[0],values.get("length")});mapped.defineProperty("length",d);return mapped;
            },c);
        },{s,v});o.defineProperty("map",d);return o;
    }
    if(v.type()==JSValue::Type::Array){auto a=array();for(unsigned i=0;i<v.length();++i)call(a,"push",{decode(v.get(std::to_string(i)),s)});return a;}return v;
}
Args values(const JSValue& a,const JSValue& s){Args out;for(unsigned i=0;i<a.length();++i)out.push_back(decode(a.get(std::to_string(i)),s));return out;}
JSValue descriptors(const JSValue& o){auto out=array();for(const auto& k:o.ownPropertyNames()){const auto d=o.getOwnPropertyDescriptor(k).value();const auto v=d.value.value_or(JSValue{});call(out,"push",{array({k,normal(v),v.isCallable()?v.get("name"):JSValue(nullptr),v.isCallable()?v.get("length"):JSValue(nullptr),d.writable.value_or(false),d.enumerable.value_or(false),d.configurable.value_or(false),d.getter&&d.getter->isCallable()?d.getter->get("name"):JSValue(nullptr)})});}return out;}
JSValue metadata(const JSValue& r){const auto m=module(r.get("class")),ctor=m.get("default");auto out=JSValue::object(),keys=array(),functions=array();for(const auto& k:m.ownPropertyNames()){call(keys,"push",{k});const auto v=m.get(k);call(functions,"push",{array({k,v.get("name"),v.get("length"),v.isConstructible()})});}out.set("keys",keys);out.set("functions",functions);out.set("prototype",descriptors(ctor.get("prototype")));out.set("statics",descriptors(ctor));out.set("instance",descriptors(ctor.construct()));return out;}
JSValue factory(const JSValue& r){auto s=JSValue::object();s.set("request",r);s.set("events",array());const auto old=ThreeUtils::getConsoleFunction();ThreeUtils::setConsoleFunction(JSValue::function(+[](const JSValue&,const Args& a,const Args& c)->JSValue{event(c[0],{"console",normal(array(a))});return {};},{s}));const auto baseline=nativeNodeConstructor().construct().get("id").number();auto out=JSValue::object();
    try{const auto args=values(r.get("args"),s);const auto m=module(r.get("class"));const auto name=r.get("name");JSValue n;
        if(name.strictEquals("toArray")){const auto node=arg(args,0);Args rest;if(!args.empty())rest.assign(args.begin()+1,args.end());n=call(node,"toArray",rest);}
        else {const auto fn=m.get(name);n=r.get("construct").toBoolean()||name.strictEquals("default")?fn.construct(args):fn.call({},args);}
        out.set("result",normal(n));out.set("fields",descriptors(n));if(n.get("values").isObject()){auto keys=array();for(const auto& k:n.get("values").ownPropertyNames())call(keys,"push",{k});out.set("valueKeys",keys);}
    }catch(...){out.set("error",failure(std::current_exception()));}
    out.set("allocationDelta",nativeNodeConstructor().construct().get("id").number()-baseline);out.set("events",s.get("events"));ThreeUtils::setConsoleFunction(old);return out;
}
JSValue cb(const JSValue&,const Args& args,const Args& c){auto s=c[0];const auto name=c[1].string();Args row{name};for(const auto& v:args)row.push_back(normal(v));event(s,row);const auto r=s.get("request");if(r.get("throwAt").strictEquals(name))throw JSTypeError("callback");
    if(name=="getCache")return s.get("current");if(name=="getCacheFromNode")return s.get("next");if(name=="setCache"){const auto count=s.get("sets").toNumber()+1;s.set("sets",count);if(r.get("throwAt").strictEquals("setCache."+std::to_string(static_cast<int>(count))))throw JSTypeError("setCache");s.set("current",arg(args,0));return {};}
    if(name=="a.getNodeType"||name=="getNodeType")return "vec3";if(name=="a.getMemberType"||name=="super.getMemberType")return "memberType";
    if(name=="a.build")return r.has("snippet")?decode(r.get("snippet"),s):JSValue("snippet");if(name=="b.build")return "outputSnippet";
    if(name=="generateArray")return array({"array",arg(args,0),arg(args,1),normal(arg(args,2))});return {};
}
JSValue callback(const JSValue& s,const std::string& name){return JSValue::function(cb,{s,name});}
void observe(const JSValue& s,JSValue o,const char* key,const std::string& label){const auto value=o.get(key);JSPropertyDescriptor d;d.configurable=true;d.getter=JSValue::function(+[](const JSValue& self,const Args&,const Args& c)->JSValue{event(c[0],{"get",c[2],self.sameValue(c[3])});if(c[0].get("request").get("throwAt").strictEquals(c[2]))throw JSTypeError("get");return c[1];},{s,value,label,o});o.defineProperty(key,d);}
JSValue protocol(const JSValue& r){auto s=JSValue::object();s.set("request",r);s.set("events",array());s.set("sets",0);auto oldCache=JSValue::object(),next=JSValue::object();oldCache.set("_tag","old");next.set("_tag","next");s.set("current",oldCache);s.set("next",next);auto a=JSValue::object(),b=JSValue::object(),builder=JSValue::object(),list=JSValue::object();a.set("_tag","a");b.set("_tag","b");builder.set("_tag","builder");list.set("_tag","values");list.set("0",a);
    for(const auto* k:{"getNodeType","getMemberType","build"}){a.set(k,callback(s,std::string("a.")+k));b.set(k,callback(s,std::string("b.")+k));}
    for(const auto* k:{"getCache","getCacheFromNode","setCache","generateArray","addLineFlowCode"})builder.set(k,callback(s,k));
    const auto ctor=module(r.get("class")).get("default");auto n=r.get("class").strictEquals("ArrayNode")?ctor.construct({decode(r.get("nodeType"),s),r.get("count"),list}):r.get("class").strictEquals("IsolateNode")?ctor.construct({a,decode(r.get("parent"),s)}):ctor.construct({b,a});n.set("_tag","node");n.set("getNodeType",callback(s,"getNodeType"));
    auto base=nativeNodeConstructor().get("prototype");const auto prior=base.getOwnPropertyDescriptor("getMemberType");base.set("getMemberType",callback(s,"super.getMemberType"));
    if(r.get("observed").toBoolean()){
        for(const auto* k:{"nodeType","values","count","node","parent","outputNode","callNode","getNodeType"})observe(s,n,k,std::string("node.")+k);
        for(const auto* k:{"getNodeType","getMemberType","build"}){observe(s,a,k,std::string("a.")+k);observe(s,b,k,std::string("b.")+k);}observe(s,list,"0","values.0");
        for(const auto* k:{"getCache","getCacheFromNode","setCache","generateArray","addLineFlowCode"})observe(s,builder,k,std::string("builder.")+k);
    }
    auto out=JSValue::object();try{Args args{builder};if(r.get("operation").strictEquals("setParent"))args={decode(r.get("value"),s)};if(r.get("operation").strictEquals("getMemberType"))args.push_back("member");if(r.get("operation").strictEquals("build")){args.push_back("float");args.push_back(9);}out.set("result",normal(call(n,r.get("operation").string().c_str(),args)));}catch(...){out.set("error",failure(std::current_exception()));}
    base.defineProperty("getMemberType",*prior);out.set("cache",normal(s.get("current")));out.set("events",s.get("events"));return out;
}
JSValue integration(const JSValue& r){auto renderer=JSValue::object(),debug=JSValue::object(),diagnostics=JSValue::object();diagnostics.set("keywords",false);debug.set("diagnostics",diagnostics);renderer.set("debug",debug);renderer.set("coordinateSystem",2000);auto builder=nativeNodeBuilderConstructor().construct({nullptr,renderer,nullptr});builder.set("shaderStage","fragment");const bool relative=r.get("relativeIntegrationNodeIds").toBoolean();const auto base=relative?nativeNodeConstructor().construct().get("id").number():0;auto out=JSValue::object(),results=array();auto s=JSValue::object();s.set("request",r);s.set("events",array());
    try{const auto n=module(r.get("class")).get(r.get("name")).call({},values(r.get("args"),s));for(const auto* stage:{"setup","analyze","generate"}){builder.set("buildStage",stage);call(results,"push",{normal(call(n,"build",{builder,"float"}),base,relative)});}out.set("results",results);out.set("flow",normal(builder.get("flow"),base,relative));}catch(...){out.set("error",failure(std::current_exception()));}return out;
}
}
int main(){try{const std::string input(std::istreambuf_iterator<char>(std::cin),{});const auto requests=Animation_detail::parseJSON(input);nativeArrayNodeModule();nativeIsolateNodeModule();nativeBypassNodeModule();auto out=array();for(unsigned i=0;i<requests.length();++i){const auto r=requests.get(std::to_string(i));const auto kind=r.get("kind");call(out,"push",{kind.strictEquals("metadata")?metadata(r):kind.strictEquals("factory")?factory(r):kind.strictEquals("integration")?integration(r):protocol(r)});}std::cout<<out.toJSON()->dump();}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
