#include "animation/AnimationUtils.h"
#include "nodes/core/PropertyNode.h"
#include "nodes/core/Node.h"
#include "nodes/tsl/TSLCore.h"
#include "utils.h"
#include <cmath>
#include <iostream>
#include <limits>

namespace {
using Args=std::vector<JSValue>;
JSValue arg(const Args& args,std::size_t i){return i<args.size()?args[i]:JSValue{};}
void own(JSValue object,const std::string& key,const JSValue& value){JSPropertyDescriptor d;d.value=value;d.writable=d.enumerable=d.configurable=true;object.defineProperty(key,d);}
JSValue call(const JSValue& object,const char* key,const Args& args={}){return object.get(key).call(object,args);}
JSValue array(const Args& values={}){auto out=JSValue::array();for(const auto& value:values)call(out,"push",{value});return out;}
JSValue normal(const JSValue& value,std::vector<JSValue>* seen=nullptr){
    if(value.isUndefined())return "#undefined";if(value.isSymbol())return "#symbol";if(value.isCallable())return "#function";
    if(value.type()==JSValue::Type::Number){const auto n=value.number();if(std::isnan(n))return "#NaN";if(std::isinf(n))return n>0?"#Infinity":"#-Infinity";if(n==0&&std::signbit(n))return "#-0";}
    if(!value.isObject())return value;
    std::vector<JSValue> local;if(!seen)seen=&local;for(const auto& item:*seen)if(item.sameValue(value))return "#repeat";seen->push_back(value);
    auto out=value.type()==JSValue::Type::Array?JSValue::array(value.length()):JSValue::object();
    for(const auto& key:value.keys())own(out,key,normal(value.get(key),seen));seen->pop_back();return out;
}
void event(const JSValue& state,const Args& values){call(state.get("events"),"push",{array(values)});}
JSValue decode(const JSValue& value,const JSValue& state){
    if(!value.isObject())return value;const auto tag=value.get("tag");
    if(tag.strictEquals("undefined"))return {};if(tag.strictEquals("NaN"))return std::numeric_limits<double>::quiet_NaN();if(tag.strictEquals("negativeZero"))return -0.0;
    if(tag.strictEquals("infinity"))return std::numeric_limits<double>::infinity();if(tag.strictEquals("symbol"))return JSValue::symbol("fixture");
    if(tag.strictEquals("node"))return nativeNodeConstructor().construct({value.has("nodeType")?value.get("nodeType"):JSValue("float")});
    if(tag.strictEquals("function"))return JSValue::function(+[](const JSValue&,const Args&,const Args&)->JSValue{return {};}, {},"",0);
    if(tag.strictEquals("flags")){
        auto object=JSValue::object();
        for(const auto* flag:{"isNode","isVector2","isVector3","isVector4","isMatrix2","isMatrix3","isMatrix4","isColor"}){
            JSPropertyDescriptor d;d.configurable=true;d.getter=JSValue::function(+[](const JSValue& self,const Args&,const Args& c)->JSValue{
                event(c[0],{"get",c[2],self.sameValue(c[1])});const auto request=c[0].get("request");if(request.get("throwAt").strictEquals(c[2]))throw JSTypeError("flag");return c[3].get("flag").strictEquals(c[2]);
            },{state,object,flag,value});object.defineProperty(flag,d);
        }return object;
    }
    if(value.type()==JSValue::Type::Array){auto out=array();for(unsigned i=0;i<value.length();++i)call(out,"push",{decode(value.get(std::to_string(i)),state)});return out;}
    return value;
}
Args values(const JSValue& supplied,const JSValue& state){Args out;for(unsigned i=0;i<supplied.length();++i)out.push_back(decode(supplied.get(std::to_string(i)),state));return out;}
JSValue failure(std::exception_ptr error){try{std::rethrow_exception(error);}catch(const JSTypeError&){return "#TypeError";}catch(const JSRangeError&){return "#RangeError";}catch(const std::exception& e){auto out=JSValue::object();out.set("nativeError",e.what());return out;}}
JSValue descriptors(const JSValue& value){auto out=array();for(const auto& key:value.ownPropertyNames()){
    const auto d=value.getOwnPropertyDescriptor(key).value();const auto field=d.value.value_or(JSValue{});
    call(out,"push",{array({key,normal(field),d.writable.value_or(false),d.enumerable.value_or(false),d.configurable.value_or(false),d.getter&&d.getter->isCallable()?d.getter->get("name"):JSValue(nullptr)})});
}return out;}
JSValue metadata(){const auto module=nativePropertyNodeModule(),ctor=nativePropertyNodeConstructor();auto out=JSValue::object(),keys=array(),functions=array(),nodes=array();
    for(const auto& key:module.ownPropertyNames()){call(keys,"push",{key});const auto value=module.get(key);if(value.isCallable())call(functions,"push",{array({key,value.get("name"),value.get("length"),value.isConstructible()})});else call(nodes,"push",{key});}
    out.set("keys",keys);out.set("functions",functions);out.set("nodes",nodes);out.set("prototype",descriptors(ctor.get("prototype")));out.set("constructor",descriptors(ctor));out.set("moduleIdentity",module.sameValue(nativePropertyNodeModule()));out.set("defaultIdentity",module.get("default").sameValue(ctor));return out;
}
JSValue snapshot(const JSValue& request){const auto module=nativePropertyNodeModule(),node=module.get(request.get("name"));auto out=JSValue::object();out.set("node",normal(node));out.set("fields",descriptors(node));out.set("type",node.get("type"));out.set("prototype",node.getPrototype().sameValue(nativePropertyNodeConstructor().get("prototype")));out.set("repeatIdentity",node.sameValue(nativePropertyNodeModule().get(request.get("name"))));out.set("nodeObjectIdentity",node.sameValue(nativeTSLCore().get("nodeObject").call({}, {node})));out.set("hash",normal(call(node,"getHash")));out.set("cacheKey",normal(call(node,"customCacheKey")));out.set("cacheable",call(node,"isCacheable"));return out;}
JSValue factory(const JSValue& request){auto state=JSValue::object();state.set("request",request);state.set("events",array());const auto baseline=nativeNodeConstructor().construct().get("id").number();auto out=JSValue::object();
    const auto console=ThreeUtils::getConsoleFunction();ThreeUtils::setConsoleFunction(JSValue::function(+[](const JSValue&,const Args& args,const Args& c)->JSValue{event(c[0],{"console",normal(array(args))});return {};},{state}));
    try{
        const auto args=values(request.get("args"),state);const auto target=nativePropertyNodeModule().get(request.get("name"));
        const auto create=[&](){return request.get("invokeMode").strictEquals("construct")||(!request.get("invokeMode").strictEquals("call")&&request.get("name").strictEquals("default"))?target.construct(args):target.call({},args);};
        const auto first=create();out.set("first",normal(first));out.set("fields",descriptors(first));
        if(request.get("repeat").toBoolean()){
            const auto second=create();out.set("second",normal(second));out.set("sameNode",first.sameValue(second));out.set("samePlaceholder",first.get("placeholderNode").sameValue(second.get("placeholderNode")));
            const auto index=request.get("name").strictEquals("default")?3u:2u;out.set("placeholderIsArgument",first.get("placeholderNode").sameValue(index<args.size()?args[index]:JSValue(nullptr)));
            out.set("nodeObjectIdentity",nativeTSLCore().get("nodeObject").call({}, {first}).sameValue(first));
        }
    }catch(...){out.set("error",failure(std::current_exception()));}
    out.set("allocationDelta",nativeNodeConstructor().construct().get("id").number()-baseline);out.set("events",state.get("events"));ThreeUtils::setConsoleFunction(console);return out;
}
JSValue protocol(const JSValue& request){auto state=JSValue::object();state.set("request",request);state.set("events",array());auto node=nativePropertyNodeModule().get(request.get("factory")).call({}, {request.get("nodeType"),request.get("propertyName"),nullptr});node.set("id",900);node.set("_fixtureTag","node");
    auto variable=JSValue::object();variable.set("_fixtureTag","variable");variable.set("needsInterpolation",false);auto builder=JSValue::object();builder.set("_fixtureTag","builder");auto placeholder=JSValue::object();placeholder.set("_fixtureTag","placeholder");
    state.set("node",node);state.set("variable",variable);state.set("data",JSValue::object());state.set("properties",JSValue::object());
    const auto fn=[&](const std::string& name){return JSValue::function(+[](const JSValue&,const Args& args,const Args& c)->JSValue{
        const auto state=c[0];const auto name=c[1].string();Args trace{name};for(const auto& value:args){const auto tag=value.isObject()?value.get("_fixtureTag"):JSValue{};trace.push_back(tag.toBoolean()?JSValue("@"+tag.toString()):normal(value));}event(state,trace);const auto request=state.get("request");if(request.get("throwAt").strictEquals(name))throw JSTypeError("callback");
        if(name=="getVarFromNode"||name=="getVaryingFromNode")return state.get("variable");if(name=="getDataFromNode")return state.get("data");if(name=="getNodeProperties")return state.get("properties");if(name=="getOutputType")return "vec4";if(name=="hasWriteUsage")return request.get("write");if(name=="getPropertyName")return "variable";if(name=="placeholder.build")return "initial";return {};
    },{state,name});};
    placeholder.set("build",fn("placeholder.build"));if(request.get("placeholder").toBoolean())node.set("placeholderNode",placeholder);
    for(const auto* key:{"getVarFromNode","getVaryingFromNode","getDataFromNode","getNodeProperties","getOutputType","hasWriteUsage","getPropertyName","addLineFlowCode"})builder.set(key,fn(key));
    const auto observe=[&](JSValue object,const char* key,const std::string& label){const auto value=object.get(key);JSPropertyDescriptor d;d.configurable=true;d.getter=JSValue::function(+[](const JSValue& self,const Args&,const Args& c)->JSValue{event(c[0],{"get",c[2],self.sameValue(c[3])});if(c[0].get("request").get("throwAt").strictEquals(c[2]))throw JSTypeError("get");return c[1];},{state,value,label,object});object.defineProperty(key,d);};
    if(request.get("observed").toBoolean()){
        for(const auto* key:{"name","varying","placeholderNode","getNodeType"})observe(node,key,std::string("node.")+key);
        for(const auto* key:{"getVarFromNode","getVaryingFromNode","getDataFromNode","getNodeProperties","getOutputType","hasWriteUsage","getPropertyName","addLineFlowCode"})observe(builder,key,std::string("builder.")+key);
        observe(placeholder,"build","placeholder.build");
    }
    auto out=JSValue::object();try{out.set("result",normal(call(node,request.get("operation").string().c_str(),{builder})));}catch(...){out.set("error",failure(std::current_exception()));}out.set("events",state.get("events"));out.set("needsInterpolation",variable.get("needsInterpolation"));return out;
}
}
int main(){try{const std::string input(std::istreambuf_iterator<char>(std::cin),{});const auto requests=Animation_detail::parseJSON(input);nativePropertyNodeModule();auto out=array();for(unsigned i=0;i<requests.length();++i){const auto r=requests.get(std::to_string(i));const auto kind=r.get("kind");call(out,"push",{kind.strictEquals("metadata")?metadata():kind.strictEquals("snapshot")?snapshot(r):kind.strictEquals("factory")?factory(r):protocol(r)});}std::cout<<out.toJSON()->dump();}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
