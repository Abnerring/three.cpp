#include "animation/AnimationUtils.h"
#include "nodes/accessors/BufferAttributeNode.h"
#include "nodes/core/Node.h"
#include "nodes/core/NodeBuilder.h"
#include "nodes/core/NodeCache.h"
#include "nodes/core/VaryingNode.h"
#include "nodes/tsl/TSLCore.h"
#include "core/InterleavedBuffer.h"
#include "core/InterleavedBufferAttribute.h"
#include "core/NativeObject.h"
#include <cmath>
#include <iostream>
#include <regex>

namespace {
using Args=std::vector<JSValue>;
JSValue arg(const Args& a,unsigned i){return i<a.size()?a[i]:JSValue{};}
void own(JSValue o,const std::string& k,const JSValue& v){JSPropertyDescriptor d;d.value=v;d.writable=d.enumerable=d.configurable=true;o.defineProperty(k,d);}
JSValue array(const Args& a={}){auto o=JSValue::array();for(const auto& v:a)own(o,std::to_string(o.length()),v);return o;}
JSValue call(const JSValue& o,const char* k,const Args& a={}){return o.get(k).call(o,a);}
bool relativeIds=false;double integrationBaseline=0;
JSValue normal(const JSValue& v,std::vector<JSValue>* seen=nullptr){
    if(v.isUndefined())return "#undefined";if(v.isCallable())return "#function";if(v.isSymbol())return "#symbol";
    if(v.type()==JSValue::Type::Number){if(std::isnan(v.number()))return "#NaN";if(std::isinf(v.number()))return v.number()>0?"#Infinity":"#-Infinity";if(v.number()==0&&std::signbit(v.number()))return "#-0";}
    if(!v.isObject())return v;if(v.get("fixtureTag").toBoolean())return "@"+v.get("fixtureTag").toString();
    std::vector<JSValue> local;if(!seen)seen=&local;for(unsigned i=0;i<seen->size();++i)if(v.sameValue((*seen)[i]))return array({"#ref",i});seen->push_back(v);
    auto out=v.type()==JSValue::Type::Array?JSValue::array(v.length()):JSValue::object();
    for(const auto& k:v.keys()){const auto x=v.get(k);if(k=="uuid"&&x.type()==JSValue::Type::String){const bool valid=std::regex_match(x.string(),std::regex("[0-9a-fA-F]{8}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-[0-9a-fA-F]{12}"));own(out,k,valid?JSValue("#valid-random-uuid"):normal(x,seen));}
        else if(relativeIds&&integrationBaseline&&k=="id"&&v.get("isNode").strictEquals(true)&&x.type()==JSValue::Type::Number&&x.number()>integrationBaseline)own(out,k,x.number()-integrationBaseline);
        else own(out,k,normal(x,seen));}
    return out;
}
JSValue failure(){try{throw;}catch(const JSTypeError&){return "#TypeError";}catch(const JSRangeError&){return "#RangeError";}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return "#Error";}}
void event(JSValue s,const Args& a){call(s.get("events"),"push",{array(a)});}
JSValue tag(const char* value){auto o=JSValue::object();o.set("fixtureTag",value);return o;}
JSValue decode(const JSValue& v){
    if(!v.isObject())return v;const auto kind=v.get("tag");if(kind.strictEquals("undefined"))return {};if(kind.strictEquals("NaN"))return std::nan("");if(kind.strictEquals("symbol"))return JSValue::symbol(std::string("fixture"));
    if(kind.strictEquals("typed")){const auto n=v.get("kind");return NativeArray_detail::constructor(n.isUndefined()?6:static_cast<std::size_t>(n.toNumber())).construct({v.get("values")});}
    if(kind.strictEquals("attribute")){auto a=nativeBufferAttributeConstructor().construct({decode(v.get("array")),v.get("itemSize"),v.get("normalized")});if(v.hasOwn("instanced"))a.set("isInstancedBufferAttribute",v.get("instanced"));if(v.hasOwn("usage"))a.set("usage",v.get("usage"));return a;}
    if(kind.strictEquals("interleaved"))return InterleavedBuffer(decode(v.get("array")),v.get("stride")).object();
    if(kind.strictEquals("fake")){auto o=JSValue::object();for(const auto& k:v.keys())if(k!="tag")own(o,k,decode(v.get(k)));return o;}
    if(v.type()==JSValue::Type::Array){auto a=JSValue::array(v.length());for(unsigned i=0;i<v.length();++i)own(a,std::to_string(i),decode(v.get(std::to_string(i))));return a;}return v;
}
JSValue desc(const JSValue& o){auto a=array();for(const auto& k:o.ownPropertyNames()){const auto d=o.getOwnPropertyDescriptor(k);const auto v=d->value.value_or(JSValue{});call(a,"push",{array({k,normal(v),v.isCallable()?v.get("name"):JSValue(nullptr),v.isCallable()?v.get("length"):JSValue(nullptr),d->writable.value_or(false),d->enumerable.value_or(false),d->configurable.value_or(false),d->getter?d->getter->get("name"):JSValue(nullptr)})});}return a;}
JSValue metadata(){const auto m=nativeBufferAttributeNodeModule(),c=m.get("default");auto out=JSValue::object(),keys=array(),functions=array();for(const auto& k:m.keys()){call(keys,"push",{k});const auto v=m.get(k);call(functions,"push",{array({k,v.get("name"),v.get("length"),v.isConstructible()})});}out.set("keys",keys);out.set("functions",functions);out.set("parent",c.getPrototype().get("name"));out.set("prototype",desc(c.get("prototype")));out.set("statics",desc(c));out.set("instance",desc(c.construct()));return out;}
JSValue factory(const JSValue& r){auto out=JSValue::object();const auto base=nativeNodeConstructor().construct().get("id").toNumber();try{Args args;for(unsigned i=0;i<r.get("args").length();++i)args.push_back(decode(r.get("args").get(std::to_string(i))));const auto f=nativeBufferAttributeNodeModule().get(r.get("name"));const auto n=r.get("construct").toBoolean()?f.construct(args):r.get("name").strictEquals("default")?f.construct(args):f.call({},args);out.set("result",normal(n));}catch(...){out.set("error",failure());}out.set("allocationDelta",nativeNodeConstructor().construct().get("id").toNumber()-base);return out;}
void observe(JSValue s,JSValue o,const char* key,const std::string& label){auto record=JSValue::object();record.set("value",o.get(key));record.set("object",o);record.set("key",key);record.set("label",label);call(s.get("records"),"push",{record});JSPropertyDescriptor d;d.enumerable=d.configurable=true;
    d.getter=JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue{event(c[0],{"get",c[1].get("label")});if(c[0].get("r").get("throwAt").strictEquals(c[1].get("label")))throw JSTypeError("getter");return c[1].get("value");},{s,record});
    d.setter=JSValue::function(+[](const JSValue&,const Args& a,const Args& c)->JSValue{const auto label="set "+c[1].get("label").toString();event(c[0],{label,normal(arg(a,0))});if(c[0].get("r").get("throwAt").strictEquals(label))throw JSTypeError("setter");auto record=c[1];record.set("value",arg(a,0));return {};},{s,record});o.defineProperty(key,d);
}
void restore(JSValue s){const auto records=s.get("records");for(unsigned i=0;i<records.length();++i){const auto r=records.get(std::to_string(i));own(r.get("object"),r.get("key").string(),r.get("value"));}}
JSValue callback(const JSValue& s,const char* name){return JSValue::function(+[](const JSValue& self,const Args& args,const Args& c)->JSValue{
    const auto s=c[0],name=c[1],r=s.get("r");Args row{name};for(const auto& a:args)row.push_back(normal(a));event(s,row);if(r.get("throwAt").strictEquals(name))throw JSTypeError("callback");
    if(name.strictEquals("getTypeFromAttribute")||name.strictEquals("getNodeType"))return r.get("type");if(name.strictEquals("getTypeLength"))return r.get("size");
    if(name.strictEquals("getData"))return s.get("cacheData");if(name.strictEquals("setData")){auto state=s;state.set("cacheData",arg(args,1));return {};}
    if(name.strictEquals("getBufferAttributeFromNode"))return tag("nodeAttribute");if(name.strictEquals("getPropertyName"))return r.get("propertyName");
    if(name.strictEquals("varying.build")){event(s,{"varying.fields",self.get("name"),self.get("node").get("node").sameValue(s.get("node")),self.get("node").get("name")});return "VARYING";}
    return {};
},{s,name},name,0);}
JSValue protocol(const JSValue& r){auto s=JSValue::object();s.set("r",r);s.set("events",array());s.set("records",array());auto value=decode(r.get("value"));if(value.isObject())value.set("fixtureTag","value");auto n=nativeBufferAttributeNodeConstructor().construct({value,decode(r.get("bufferType")),decode(r.get("stride")),decode(r.get("offset"))});n.set("id",100);n.set("fixtureTag","node");s.set("node",n);n.set("getNodeType",callback(s,"getNodeType"));
    if(r.hasOwn("attribute")){auto attribute=decode(r.get("attribute"));if(attribute.isObject())attribute.set("fixtureTag","attribute");n.set("attribute",attribute);}if(r.hasOwn("instanced"))n.set("instanced",decode(r.get("instanced")));
    auto cache=tag("cache"),builder=tag("builder"),context=tag("context");cache.set("getData",callback(s,"getData"));cache.set("setData",callback(s,"setData"));builder.set("globalCache",cache);builder.set("context",context);context.set("nodeName",decode(r.get("nodeName")));builder.set("shaderStage",decode(r.get("stage")));
    for(const auto* k:{"getTypeFromAttribute","getTypeLength","getBufferAttributeFromNode","getPropertyName"})builder.set(k,callback(s,k));
    if(r.get("existingData").toBoolean()){auto data=tag("cacheData"),node=tag("cachedNode");node.set("id",42);data.set("node",node);s.set("cacheData",data);}
    const auto p=nativeVaryingNodeConstructor().get("prototype");const auto old=p.getOwnPropertyDescriptor("build");auto mutableP=p;mutableP.set("build",callback(s,"varying.build"));
    if(r.get("observed").toBoolean()){
        for(const auto* k:{"bufferStride","bufferOffset","value","attribute","bufferType","usage","instanced","getNodeType","id"})observe(s,n,k,std::string("node.")+k);
        for(const auto* k:{"globalCache","context","shaderStage","getTypeFromAttribute","getTypeLength","getBufferAttributeFromNode","getPropertyName"})observe(s,builder,k,std::string("builder.")+k);
        for(const auto* k:{"getData","setData"})observe(s,cache,k,std::string("cache.")+k);observe(s,context,"nodeName","context.nodeName");
        if(value.isObject())for(const auto* k:{"isInterleavedBuffer","isBufferAttribute","array"})observe(s,value,k,std::string("value.")+k);
    }
    if(r.get("nodeNameNonConfigurable").toBoolean()){auto d=context.getOwnPropertyDescriptor("nodeName");d->configurable=false;context.defineProperty("nodeName",*d);}
    auto out=JSValue::object();try{const auto operation=r.get("operation").string();out.set("result",normal(call(n,operation.c_str(),operation=="setUsage"||operation=="setInstanced"?Args{decode(r.get("argument"))}:Args{builder})));}catch(...){out.set("error",failure());}
    const bool nodeNamePresent=context.hasOwn("nodeName");if(old)mutableP.defineProperty("build",*old);else mutableP.erase("build");restore(s);out.set("events",s.get("events"));out.set("bufferType",normal(n.get("bufferType")));out.set("usage",normal(n.get("usage")));out.set("instanced",normal(n.get("instanced")));out.set("name",normal(n.get("name")));out.set("attribute",normal(n.get("attribute")));out.set("nodeNamePresent",nodeNamePresent);return out;
}
JSValue sharing(const JSValue& r){auto out=JSValue::object();try{const auto value=decode(r.get("value")),c=nativeBufferAttributeNodeConstructor();auto a=c.construct({value,"vec3",3,0}),b=c.construct({value,"vec2",2,1});a.set("usage",35048);b.set("usage",35044);a.set("getNodeType",JSValue::function(+[](const JSValue&,const Args&,const Args&)->JSValue{return "vec3";}));b.set("getNodeType",JSValue::function(+[](const JSValue&,const Args&,const Args&)->JSValue{return "vec2";}));auto builder=JSValue::object();builder.set("getTypeLength",JSValue::function(+[](const JSValue&,const Args& a,const Args&)->JSValue{return arg(a,0).strictEquals("vec3")?3:2;}));
        call(a,"setup",{builder});call(b,"setup",{builder});const auto x=a.get("attribute"),y=b.get("attribute");out.set("sameData",x.get("data").sameValue(y.get("data")));out.set("sameArray",x.get("array").sameValue(y.get("array")));out.set("differentAttributes",!x.sameValue(y));out.set("stride",x.get("data").get("stride"));out.set("usage",x.get("data").get("usage"));out.set("offsets",array({x.get("offset"),y.get("offset")}));out.set("instanced",array({x.get("isInstancedBufferAttribute"),y.get("isInstancedBufferAttribute")}));
        auto cache=nativeNodeCacheConstructor().construct(),hashBuilder=JSValue::object();hashBuilder.set("globalCache",cache);a.set("id",100);b.set("id",101);a.set("bufferStride",0);a.set("bufferOffset",0);b.set("bufferStride",0);b.set("bufferOffset",0);out.set("hashes",array({call(a,"getHash",{hashBuilder}),call(b,"getHash",{hashBuilder})}));
    }catch(...){out.set("error",failure());}return out;}
JSValue integration(const JSValue& r){auto renderer=JSValue::object(),debug=JSValue::object(),diagnostics=JSValue::object();diagnostics.set("keywords",false);debug.set("diagnostics",diagnostics);renderer.set("debug",debug);renderer.set("coordinateSystem",2000);auto builder=nativeNodeBuilderConstructor().construct({nullptr,renderer,nullptr});builder.set("shaderStage",r.get("stage"));if(r.hasOwn("nodeName")){auto context=builder.get("context");context.set("nodeName",r.get("nodeName"));}integrationBaseline=relativeIds?nativeNodeConstructor().construct().get("id").toNumber():0;auto out=JSValue::object();
    try{const auto n=nativeBufferAttributeNodeBinding("bufferAttribute").call({}, {decode(r.get("value")),r.get("bufferType")});auto results=array();for(const auto* stage:{"setup","analyze","generate"}){builder.set("buildStage",stage);call(results,"push",{normal(call(n,"build",{builder,r.get("bufferType")}))});}out.set("results",results);out.set("flow",normal(builder.get("flow")));out.set("bufferAttributes",normal(builder.get("bufferAttributes")));out.set("varyings",normal(builder.get("varyings")));out.set("attribute",normal(n.get("attribute")));out.set("nodeNamePresent",builder.get("context").hasOwn("nodeName"));}catch(...){out.set("error",failure());}integrationBaseline=0;return out;}
void lifetime(){
    for(unsigned round=0;round<3;++round){JSWeakValue bindingWitness,bufferWitness;
        {const auto binding=nativeBufferAttributeNodeBinding("bufferAttribute");auto facade=nativeBufferAttributeNodeModule();facade.set("bufferAttribute",nullptr);facade.set("default",nullptr);
            if(!nativeBufferAttributeNodeBinding("bufferAttribute").sameValue(binding))throw std::logic_error("Writable facade replaced lexical factory");
            auto bufferPrototype=InterleavedBuffer::prototype(),attributePrototype=InterleavedBufferAttribute::prototype();const auto oldBuffer=bufferPrototype.getOwnPropertyDescriptor("constructor"),oldAttribute=attributePrototype.getOwnPropertyDescriptor("constructor");
            const auto poison=JSValue::function(+[](const JSValue&,const Args&,const Args&)->JSValue{throw JSTypeError("Mutable prototype.constructor used as lexical import");});bufferPrototype.set("constructor",poison);attributePrototype.set("constructor",poison);
            auto values=array({1,2,3,4,5,6});const auto value=NativeArray_detail::constructor(6).construct({values});auto a=binding.call({}, {value,"vec3"}),b=binding.call({}, {value,"vec3"});
            const auto getType=JSValue::function(+[](const JSValue&,const Args&,const Args&)->JSValue{return "vec3";});a.set("getNodeType",getType);b.set("getNodeType",getType);auto builder=JSValue::object();builder.set("getTypeLength",JSValue::function(+[](const JSValue&,const Args&,const Args&)->JSValue{return 3;}));call(a,"setup",{builder});call(b,"setup",{builder});
            const auto buffer=a.get("attribute").get("data");if(!buffer.sameValue(b.get("attribute").get("data"))||!buffer.get("array").sameValue(value))throw std::logic_error("TypedArray buffer cache lost shared identity");
            binding.collectGarbage();if(!nativeBufferAttributeNodeBinding("bufferAttribute").sameValue(binding)||!a.get("attribute").get("data").sameValue(buffer))throw std::logic_error("GC discarded a live lexical/cache edge");
            bufferPrototype.defineProperty("constructor",*oldBuffer);attributePrototype.defineProperty("constructor",*oldAttribute);bindingWitness=JSWeakValue(binding);bufferWitness=JSWeakValue(buffer);
        }JSValue::nativeSafepoint();if(!bindingWitness.expired()||!bufferWitness.expired())throw std::logic_error("BufferAttribute module or Interleaved prototype retained a retired heap");
    }std::cout<<"3 fresh BufferAttributeNode binding/cache lifetimes passed\n";
}
}
int main(int argc,char** argv){try{if(argc>1&&std::string(argv[1])=="--lifetime"){lifetime();return 0;}relativeIds=argc>1&&std::string(argv[1])=="--relative-integration-node-ids";const std::string input(std::istreambuf_iterator<char>(std::cin),{});const auto requests=Animation_detail::parseJSON(input);nativeBufferAttributeNodeModule();auto out=array();for(unsigned i=0;i<requests.length();++i){const auto r=requests.get(std::to_string(i)),kind=r.get("kind");call(out,"push",{kind.strictEquals("metadata")?metadata():kind.strictEquals("factory")?factory(r):kind.strictEquals("sharing")?sharing(r):kind.strictEquals("integration")?integration(r):protocol(r)});}std::cout<<out.toJSON()->dump();}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
