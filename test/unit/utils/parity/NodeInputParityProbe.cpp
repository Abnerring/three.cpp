#include "animation/AnimationUtils.h"
#include "core/JSIterator.h"
#include "core/NativeArray.h"
#include "nodes/core/InputNode.h"
#include "nodes/core/ConstNode.h"
#include "nodes/core/UniformNode.h"
#include "nodes/core/UniformGroupNode.h"
#include "nodes/core/Node.h"
#include "nodes/core/NodeBuilder.h"
#include "nodes/core/NodeUtils.h"
#include "utils.h"
#include <cmath>
#include <iostream>
#include <limits>
namespace {
using Args=std::vector<JSValue>;
JSValue arg(const Args& args,std::size_t i){return i<args.size()?args[i]:JSValue{};}
JSValue call(const JSValue& self,const char* key,const Args& args={}){return self.get(key).call(self,args);}
JSValue array(const Args& args={}){auto out=JSValue::array();for(const auto& value:args)call(out,"push",{value});return out;}
void getter(JSValue object,const char* key,const JSValue& fn){JSPropertyDescriptor d;d.getter=fn;d.enumerable=d.configurable=true;object.defineProperty(key,d);}
JSValue ctor(const std::string& name){if(name=="InputNode")return nativeInputNodeConstructor();if(name=="ConstNode")return nativeConstNodeConstructor();if(name=="UniformNode")return nativeUniformNodeConstructor();return nativeUniformGroupNodeConstructor();}
JSValue normal(const JSValue& value,const JSValue& state,int depth=0){
    if(value.isUndefined())return "#undefined";if(value.isCallable())return "#function";if(value.isSymbol())return "#symbol";
    if(value.type()==JSValue::Type::Number){if(std::isnan(value.number()))return "#NaN";if(std::isinf(value.number()))return value.number()>0?"#Infinity":"#-Infinity";if(value.number()==0&&std::signbit(value.number()))return "#-0";}
    if(!value.isObject())return value;
    for(const auto* key:{"node","builder","frame","callback","uniform","variable"})if(value.strictEquals(state.get(key)))return std::string("#")+key;
    if(value.strictEquals(nativeObjectGroup()))return "#objectGroup";
    if(value.isHostObject("three.ArrayBuffer")){const auto bytes=NativeArray_detail::constructor(1).construct({value});auto out=JSValue::array();for(double i=0;i<bytes.get("length").number();++i)call(out,"push",{bytes.get(JSValue(i).toString())});return out;}
    if(depth>8)return "#depth";
    auto out=value.type()==JSValue::Type::Array?JSValue::array(value.length()):JSValue::object();
    for(const auto& key:value.keys()){if(key=="id"||key=="_uuid")continue;out.set(key,normal(value.get(key),state,depth+1));}return out;
}
void event(const JSValue& state,const Args& values){auto row=JSValue::array();for(const auto& value:values)call(row,"push",{normal(value,state)});call(state.get("events"),"push",{row});}
JSValue decode(const JSValue& value,const JSValue& state){
    if(!value.isObject())return value;
    if(value.type()==JSValue::Type::Array){auto out=JSValue::array();for(unsigned i=0;i<value.length();++i)call(out,"push",{decode(value.get(std::to_string(i)),state)});return out;}
    const auto tag=value.get("tag");if(tag.strictEquals("undefined"))return {};if(tag.strictEquals("nan"))return std::numeric_limits<double>::quiet_NaN();if(tag.strictEquals("negative-zero"))return -0.0;if(tag.strictEquals("infinity"))return std::numeric_limits<double>::infinity();if(tag.strictEquals("symbol"))return JSValue::symbol("fixture");
    if(tag.strictEquals("buffer"))return NativeArray_detail::constructor(1).construct({decode(value.get("values"),state)}).get("buffer");
    if(tag.strictEquals("coerce")){
        auto object=JSValue::object(),count=JSValue::object();count.set("n",0);object.setPrimitiveConverter(JSValue::function([](const JSValue&,const Args& args,const Args& c)->JSValue{
            auto count=c[2];const auto n=count.get("n").number();count.set("n",n+1);event(c[0],{"coerce",c[1].get("name"),arg(args,0),n});const auto values=c[1].get("values");return decode(values.get(JSValue(std::min(n,double(values.length()-1))).toString()),c[0]);
        },{state,value,count}));return object;
    }
    if(tag.strictEquals("arrayObject")){
        auto object=JSValue::object();const auto values=decode(value.get("values"),state);object.set("isVector2",value.get("vector2"));
        for(const auto* key:{"toArray","fromArray"})getter(object,key,JSValue::function([](const JSValue&,const Args&,const Args& c)->JSValue{
            event(c[0],{"get",c[1]});return JSValue::function([](const JSValue& self,const Args& args,const Args& c)->JSValue{event(c[0],{c[1],array(args)});if(c[1].strictEquals("toArray"))return c[2];return self;},c);
        },{state,key,values}));return object;
    }
    auto out=JSValue::object();for(const auto& key:value.keys())out.set(key,decode(value.get(key),state));return out;
}
JSValue builder(const JSValue& state){
    auto renderer=JSValue::object(),debug=JSValue::object(),diagnostics=JSValue::object();diagnostics.set("keywords",false);debug.set("diagnostics",diagnostics);renderer.set("debug",debug);
    auto out=NodeBuilder(nullptr,renderer,nullptr).object();out.set("shaderStage","fragment");state.get("events");return out;
}
JSValue run(const JSValue& request){
    auto state=JSValue::object();state.set("events",array());auto result=JSValue::object();const auto kind=request.get("kind").string();
    const auto className=request.hasOwn("class")?request.get("class").string():"UniformNode";const auto constructor=ctor(className);Args arguments;
    const auto specs=request.get("args");if(specs.type()==JSValue::Type::Array)for(unsigned i=0;i<specs.length();++i)arguments.push_back(decode(specs.get(std::to_string(i)),state));
    auto node=constructor.construct(arguments);state.set("node",node);
    auto b=builder(state);state.set("builder",b);
    ThreeUtils::setConsoleFunction(JSValue::function([](const JSValue&,const Args& args,const Args& c)->JSValue{event(c[0],{"console",arg(args,0),arg(args,1).get("startsWith").call(arg(args,1),{"THREE.TSL:"})});return {};},{state}));
    const auto proto=constructor.get("prototype"),originalParent=proto.getPrototype();const auto ctorParent=constructor.getPrototype();
    try{
        if(kind=="metadata"){
            auto fields=JSValue::object();for(const auto& key:node.ownPropertyNames()){if(key=="id"||key=="_uuid")continue;fields.set(key,normal(node.get(key),state));}result.set("fields",fields);
            auto methods=JSValue::array();for(const auto& key:proto.ownPropertyNames()){const auto d=*proto.getOwnPropertyDescriptor(key);if(key=="constructor")continue;const auto fn=proto.get(key);call(methods,"push",{array({key,fn.get("name"),fn.get("length"),fn.isConstructible(),d.enumerable.value_or(false),d.configurable.value_or(false),d.writable.value_or(false)})});}result.set("methods",methods);result.set("arity",constructor.get("length"));result.set("type",node.get("type"));result.set("cacheable",call(node,"isCacheable",{b}));
        }else if(kind=="type"){
            result.set("generated",normal(call(node,"generateNodeType",{b}),state));result.set("input",normal(call(node,"getInputType",{b}),state));result.set("precisionReturn",call(node,"setPrecision",{decode(request.get("precision"),state)}).strictEquals(node));result.set("precision",normal(node.get("precision"),state));
        }else if(kind=="const"){
            if(request.get("protocol").toBoolean()){
                for(const auto* name:{"generateConst","format"})getter(b,name,JSValue::function([](const JSValue&,const Args&,const Args& c)->JSValue{event(c[0],{"get",c[1]});return JSValue::function([](const JSValue&,const Args& args,const Args& c)->JSValue{event(c[0],{c[1],array(args)});return c[1].strictEquals("generateConst")?JSValue("literal"):JSValue("formatted");},c);},{state,name}));
                node.set("getNodeType",JSValue::function([](const JSValue&,const Args&,const Args& c)->JSValue{event(c[0],{"getNodeType"});return c[1];},{state,decode(request.get("type"),state)}));
            }
            result.set("value",normal(call(node,request.get("direct").toBoolean()?"generateConst":"generate",{b,decode(request.get("output"),state)}),state));
        }else if(kind=="serialize"||kind=="deserialize"){
            node.set("getSerializeChildren",JSValue::function([](const JSValue&,const Args&,const Args& c)->JSValue{event(c[0],{"getSerializeChildren"});return array();},{state}));
            auto data=request.hasOwn("data")?decode(request.get("data"),state):JSValue::object();
            if(request.hasOwn("precision"))node.set("precision",decode(request.get("precision"),state));if(request.hasOwn("weak"))node.set("isWeak",decode(request.get("weak"),state));
            result.set("return",normal(call(node,kind.c_str(),{data}),state));result.set("data",normal(data,state));
            auto fields=JSValue::object();for(const auto* key:{"value","nodeType","precision","isWeak","name","version","shared","order","updateType"})fields.set(key,normal(node.get(key),state));result.set("fields",fields);
        }else if(kind=="groups"){
            const auto module=nativeUniformGroupNodeModule();auto rows=JSValue::array();for(const auto* name:{"frameGroup","renderGroup","objectGroup"}){const auto group=module.get(name);call(rows,"push",{array({name,group.get("name"),group.get("shared"),group.get("order"),group.get("updateType"),group.strictEquals(module.get(name))})});}result.set("presets",rows);
            result.set("uniformGroup",node.get("groupNode").strictEquals(nativeObjectGroup()));const auto before=node.get("version");auto group=nativeUniformGroupNodeConstructor().construct({"custom",true,7,"frame"});call(group,"update");call(group,"update");result.set("updated",group.get("version"));result.set("needsUpdateOwn",group.hasOwn("needsUpdate"));
            auto factory=module.get(request.get("shared").toBoolean()?"sharedUniformGroup":"uniformGroup");const auto fresh=factory.call({},arguments);auto values=JSValue::object();for(const auto* key:{"name","shared","order","updateType","nodeType"})values.set(key,normal(fresh.get(key),state));result.set("factory",values);
        }else if(kind=="update"){
            auto frame=JSValue::object();state.set("frame",frame);auto count=JSValue::object();count.set("n",0);
            auto callback=JSValue::function([](const JSValue& self,const Args& args,const Args& c)->JSValue{
                auto count=c[2];const auto index=count.get("n").number();count.set("n",index+1);event(c[0],{"callback",self,array(args),index});
                if(c[1].get("throw").toNumber()==index)throw JSTypeError("callback");
                if(c[1].get("reenter").toBoolean()&&index==0)call(c[0].get("node"),"update",{arg(args,0)});
                if(c[1].get("mutate").toBoolean()){auto node=c[0].get("node");node.set("value","mutated");}
                const auto returns=c[1].get("returns");return index<double(returns.length())?decode(returns.get(JSValue(index).toString()),c[0]):JSValue{};
            },{state,request,count},"callback",2);state.set("callback",callback);
            if(request.get("customBind").toBoolean()){
                const auto bind=callback.get("bind");getter(callback,"bind",JSValue::function([](const JSValue& self,const Args&,const Args& c)->JSValue{event(c[0],{"get:bind",self});return JSValue::function([](const JSValue& self,const Args& args,const Args& c)->JSValue{event(c[0],{"call:bind",self,array(args)});return c[1].call(self,args);},c);},{state,bind}));
            }
            result.set("onUpdateReturn",call(node,"onUpdate",{callback,request.get("updateType")}).strictEquals(node));result.set("updateType",normal(node.get("updateType"),state));auto values=JSValue::array();
            for(int i=0;i<request.get("calls").toNumber();++i){try{call(node,"update",{frame});call(values,"push",{normal(node.get("value"),state)});}catch(const JSTypeError&){call(values,"push",{"#TypeError"});}}result.set("values",values);
        }else if(kind=="uniform"){
            auto data=JSValue::object();if(request.hasOwn("cached"))data.set("propertyName",decode(request.get("cached"),state));state.set("data",data);
            auto uniform=JSValue::object(),variable=JSValue::object();state.set("uniform",uniform);state.set("variable",variable);auto context=JSValue::object();if(request.hasOwn("label"))context.set("nodeName",decode(request.get("label"),state));b.set("context",context);
            node.set("getNodeType",JSValue::function([](const JSValue&,const Args&,const Args& c)->JSValue{event(c[0],{"getNodeType"});return c[1];},{state,request.get("type")}));
            node.set("getUniformHash",JSValue::function([](const JSValue&,const Args& args,const Args& c)->JSValue{event(c[0],{"getUniformHash",array(args)});return "fixture-hash";},{state}));if(request.hasOwn("name"))node.set("name",decode(request.get("name"),state));
            for(const auto* name:{"getNodeFromHash","setHashNode","getUniformFromNode","getPropertyName","getDataFromNode","getVarFromNode","format","addLineFlowCode"})getter(b,name,JSValue::function([](const JSValue&,const Args&,const Args& c)->JSValue{
                event(c[0],{"get",c[2]});return JSValue::function([](const JSValue&,const Args& args,const Args& c)->JSValue{
                    const auto state=c[0],request=c[1];const auto name=c[2].string();event(state,{name,array(args)});if(request.get("throw").strictEquals(name))throw JSTypeError("builder");
                    if(name=="getNodeFromHash")return request.get("found").strictEquals("self")?state.get("node"):request.hasOwn("found")?decode(request.get("found"),state):JSValue{};
                    if(name=="getUniformFromNode")return state.get("uniform");if(name=="getPropertyName")return arg(args,0).strictEquals(state.get("uniform"))?JSValue("u_value"):JSValue("v_bool");
                    if(name=="getDataFromNode")return state.get("data");if(name=="getVarFromNode")return state.get("variable");if(name=="format")return "fmt("+arg(args,0).toString()+","+arg(args,1).toString()+","+arg(args,2).toString()+")";return {};
                },c);
            },{state,request,name}));
            auto outputs=JSValue::array();for(int i=0;i<request.get("calls").toNumber();++i)call(outputs,"push",{call(node,"generate",{b,request.get("output")})});result.set("values",outputs);result.set("data",normal(data,state));result.set("context",normal(context,state));
        }else if(kind=="integration"){
            auto outputs=JSValue::array();for(const auto* stage:{"setup","analyze","generate"}){b.set("buildStage",stage);call(outputs,"push",{normal(call(node,"build",{b,request.get("output")}),state)});}result.set("values",outputs);result.set("flow",normal(b.get("flow"),state));result.set("uniforms",b.get("uniforms").get("fragment").get("length"));
        }else if(kind=="super"){
            auto alternate=JSValue::object(originalParent);getter(alternate,request.get("method").string().c_str(),JSValue::function([](const JSValue& self,const Args&,const Args& c)->JSValue{event(c[0],{"super:get",self});return JSValue::function([](const JSValue& self,const Args& args,const Args& c)->JSValue{event(c[0],{"super:call",self,array(args)});return "bool";},c);},{state}));auto mutableProto=proto;mutableProto.setPrototype(alternate);result.set("value",normal(call(node,request.get("method").string().c_str(),{b}),state));
        }
    }catch(const JSTypeError&){result.set("error","TypeError");}catch(const std::exception&){result.set("error","Error");}
    auto mutableProto=proto;mutableProto.setPrototype(originalParent);auto mutableConstructor=constructor;mutableConstructor.setPrototype(ctorParent);result.set("events",state.get("events"));ThreeUtils::setConsoleFunction({});return result;
}
}
int main(){try{const std::string input(std::istreambuf_iterator<char>(std::cin),{});const auto requests=Animation_detail::parseJSON(input);auto out=JSValue::array();for(unsigned i=0;i<requests.length();++i)call(out,"push",{run(requests.get(std::to_string(i)))});std::cout<<out.toJSON()->dump();return 0;}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}}
