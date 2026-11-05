#pragma once
#include "nodes/materialx/MaterialXNoise.h"
#include "nodes/materialx/MaterialXNodes.h"
#include "nodes/materialx/MaterialXCore.h"
#include "nodes/materialx/MaterialXColor.h"
#include "nodes/materialx/MaterialXColorTransform.h"
#include "nodes/core/NodeBuilder.h"
#include "nodes/core/StackNode.h"
#include "nodes/code/ExpressionNode.h"
#include "nodes/tsl/TSLBase.h"
#include "nodes/lighting/LightingNodeRuntime.h"
#include "core/JSPromise.h"
#include "core/JSGlobal.h"
#include "animation/AnimationUtils.h"
#include <algorithm>
#include <cmath>
#include <functional>
#include <limits>

namespace MaterialXNativeFixtures {
using namespace LightingNodeRuntime;
inline JSValue object(){return JSValue::object(nullptr);}
inline JSValue call(const JSValue& value,const char* key,const Args& args={}){return value.get(key).call(value,args);}
inline void push(const JSValue& a,const JSValue& v){call(a,"push",{v});}
inline JSValue array(const Args& values={}){auto out=JSValue::array();for(const auto& value:values)push(out,value);return out;}
inline JSValue node(const std::string& type,const Args& args={}){return nativeTSLBaseBinding(type).call({},args);}
inline JSValue module(const std::string& name){return name=="MaterialXNoise"?nativeMaterialXNoiseModule():nativeMaterialXNodesModule();}
inline JSValue binding(const std::string& name,const std::string& key){return name=="MaterialXNoise"?nativeMaterialXNoisePrivate(key):nativeMaterialXNodesPrivate(key);}
inline Args values(const JSValue& a){Args out;for(unsigned i=0;i<a.get("length").toNumber();++i)out.push_back(a.get(JSValue(i)));return out;}
inline JSValue decode(const JSValue& v){
    if(!v.isObject())return v;
    if(v.hasOwn("$undefined"))return {};
    if(v.hasOwn("$number")){const auto s=v.get("$number").toString();if(s=="NaN")return std::numeric_limits<double>::quiet_NaN();if(s=="Infinity")return std::numeric_limits<double>::infinity();if(s=="-Infinity")return -std::numeric_limits<double>::infinity();if(s=="-0")return -0.;}
    if(v.hasOwn("node")){Args args;for(const auto& value:values(v.get("args")))args.push_back(decode(value));const auto out=node(v.get("node").toString(),args);return v.hasOwn("variable")?call(out,"toVar",{v.get("variable")}):out;}
    auto out=v.type()==JSValue::Type::Array?array():object();for(const auto& key:v.keys())out.set(key,decode(v.get(key)));return out;
}
inline void initialize(){nativeTSLBase();nativeStackNodeConstructor();nativeNodeBuilderConstructor();nativeExpressionNodeBinding("expression");nativeMaterialXNoiseModule();nativeMaterialXNodesModule();}
struct StackScope{JSValue old,stack;StackScope(){initialize();old=nativeGetCurrentStack();stack=nativeStackNodeConstructor().construct();nativeSetCurrentStack(stack);}~StackScope(){nativeSetCurrentStack(old);}};
inline JSValue builder(){auto renderer=object(),debug=object(),diagnostics=object();diagnostics.set("keywords",false);debug.set("diagnostics",diagnostics);renderer.set("debug",debug);renderer.set("coordinateSystem",2001);auto b=nativeNodeBuilderConstructor().construct({nullptr,renderer,nullptr});b.set("shaderStage","fragment");return b;}
// Every module declaration is embedded from the pinned source manifest. Tests do
// not depend on the working directory or read the implementation's source table.
#include "MaterialXNativeDeclarations.inc"
inline JSValue declarations(){return Animation_detail::parseJSON(declarationsJSON);}
struct Tag{JSValue value;std::string module,name;};
inline std::vector<Tag> tags(){std::vector<Tag> result;const auto all=declarations();for(const auto* m:{"MaterialXNoise","MaterialXNodes"})for(const auto& d:values(all.get(m)))if(d.get("kind").strictEquals("Fn")){const auto f=binding(m,d.get("name").toString());result.push_back({f.get("shaderNode"),m,d.get("name").toString()});}for(const auto* name:{"mx_hsvtorgb","mx_rgbtohsv","mx_srgb_texture_to_lin_rec709"})result.push_back({binding("MaterialXNodes",name).get("shaderNode"),"MaterialXNodes",name});return result;}
class Graph {
    Args seen_;JSValue objects_=array();std::vector<Tag> tags_=tags();
    JSValue encode(const JSValue& value){
        if(value.isUndefined())return array({"undefined"});if(value.isCallable())return array({"function"});if(value.isSymbol())return array({"symbol"});
        if(value.type()==JSValue::Type::Number){const auto n=value.toNumber();if(std::isnan(n))return array({"number","NaN"});if(!std::isfinite(n))return array({"number",n>0?"Infinity":"-Infinity"});if(n==0&&std::signbit(n))return array({"number","-0"});}
        if(!value.isObject())return value;
        for(const auto& t:tags_)if(value.sameValue(t.value))return array({"shader",t.module,t.name});
        for(unsigned i=0;i<seen_.size();++i)if(value.sameValue(seen_[i]))return array({"ref",i});
        const auto index=seen_.size();seen_.push_back(value);auto row=object(),properties=array();row.set("kind",value.type()==JSValue::Type::Array?"array":"object");row.set("properties",properties);push(objects_,row);
        if(value.type()==JSValue::Type::Array)row.set("length",value.get("length"));if(value.get("isNode").strictEquals(true))row.set("type",value.get("type"));
        auto keys=value.keys();std::sort(keys.begin(),keys.end());for(const auto& key:keys){if(key=="id"||key=="uuid"||key=="stackTrace"||key=="_cacheKey"||key=="_cacheKeyVersion")continue;push(properties,array({key,encode(value.get(key))}));}
        return array({"ref",static_cast<double>(index)});
    }
public:JSValue operator()(const JSValue& value){auto out=object();out.set("root",encode(value));out.set("objects",objects_);return out;}
};
inline JSValue functionMetadata(const JSValue& f){auto row=object();row.set("name",f.get("name"));row.set("length",f.get("length"));row.set("source",call(f,"toString"));return row;}
inline JSValue fullMetadata(const JSValue& f){auto row=functionMetadata(f);if(f.get("isFn").strictEquals(true)){const auto shader=f.get("shaderNode"),callback=shader.get("jsFunc");row.set("isFn",f.get("isFn"));row.set("nodeType",shader.get("nodeType"));row.set("layout",shader.get("layout"));row.set("callbackName",callback.get("name"));row.set("callbackLength",callback.get("length"));row.set("callbackSource",call(callback,"toString"));}return row;}
inline JSValue metadata(const std::string& m){
    auto out=object(),exports=array(),entries=array();for(const auto& key:module(m).keys())push(exports,key);out.set("exports",exports);out.set("entries",entries);
    for(const auto& d:values(declarations().get(m))){const auto f=binding(m,d.get("name").toString());auto row=object(),meta=functionMetadata(f);row.set("name",d.get("name"));row.set("kind",d.get("kind"));row.set("public",d.get("exported"));if(d.get("kind").strictEquals("Fn")){const auto shader=f.get("shaderNode"),callback=shader.get("jsFunc");meta.set("isFn",f.get("isFn"));meta.set("nodeType",shader.get("nodeType"));meta.set("layout",shader.get("layout"));meta.set("callbackName",callback.get("name"));meta.set("callbackLength",callback.get("length"));meta.set("callbackSource",call(callback,"toString"));}row.set("metadata",meta);push(entries,row);}
    if(m=="MaterialXNodes"){auto reexports=array();for(const auto* name:{"mx_rotate2d","mx_rotate3d","mx_hsvtorgb","mx_rgbtohsv","mx_srgb_texture_to_lin_rec709"}){const std::string n=name;const auto original=n=="mx_rotate2d"||n=="mx_rotate3d"?nativeMaterialXCoreBinding(n):n=="mx_srgb_texture_to_lin_rec709"?nativeMaterialXColorTransformBinding(n):nativeMaterialXColorBinding(n);auto row=object();row.set("name",n);row.set("same",module(m).get(n).sameValue(original));row.set("metadata",fullMetadata(original));push(reexports,row);}out.set("reexports",reexports);}
    return Graph()(out);
}
// Execute deferred control callbacks independently. We preserve the original
// bounds/conditions in the graph and inspect all branch bodies, including nested
// loops. Calls to module Fn definitions stay as actual call nodes; each complete
// definition is tested in its own case, avoiding exponential shader expansion.
inline JSValue controls(const JSValue& root,const JSValue& b){
    auto records=array();Args seen;const auto tagged=tags();
    std::function<void(const JSValue&)> walk=[&](const JSValue& value){
        if(!value.isObject()||value.isCallable())return;for(const auto& tag:tagged)if(value.sameValue(tag.value))return;for(const auto& old:seen)if(old.sameValue(value))return;seen.push_back(value);
        JSValue record;
        if(value.get("type").strictEquals("LoopNode")){
            const auto params=value.get("params");const auto fn=params.get(JSValue(params.length()-1));const auto body=fn.get("isFn").toBoolean()?fn.get("shaderNode").get("jsFunc"):fn;auto inputs=object();
            for(unsigned i=0;i+1<params.length();++i){const auto param=params.get(JSValue(i));const bool configuration=param.isObject()&&!param.get("isNode").strictEquals(true);const auto n=configuration?param.get("name"):JSValue{},t=configuration?param.get("type"):JSValue{};const std::string name=n.toBoolean()?n.toString():std::string(1,char('i'+i));const std::string type=t.toBoolean()?t.toString():"int";inputs.set(name,nativeExpressionNodeBinding("expression").call({}, {name,type}));}
            StackScope scope;record=object();record.set("kind","Loop");record.set("source",call(body,"toString"));record.set("inputs",inputs);record.set("result",body.call({}, {inputs,b}));record.set("stack",scope.stack.get("nodes"));push(records,record);
        }else if(value.get("jsFunc").isCallable()){
            const auto body=value.get("jsFunc");StackScope scope;const auto inputs=array();record=object();record.set("kind","Shader");record.set("source",call(body,"toString"));record.set("inputs",inputs);record.set("result",body.call({}, {inputs,b}));record.set("stack",scope.stack.get("nodes"));push(records,record);
        }
        auto keys=value.keys();std::sort(keys.begin(),keys.end());for(const auto& key:keys)if(key!="stackTrace")walk(value.get(key));if(!record.isUndefined())walk(record);
    };walk(root);return records;
}
inline JSValue coercion(const JSValue& events,const std::string& side,const JSValue& variant,double value){
    auto result=object();JSPropertyDescriptor descriptor;descriptor.configurable=true;
    descriptor.getter=JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue{
        push(c[0],"get:"+c[1].toString()+":valueOf");
        return JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue{const auto side=c[1].toString();push(c[0],"call:"+side+":valueOf");if(c[2].strictEquals(side+"CoerceThrow"))throw JSThrownValue(side+":valueOf");return c[3];},c);
    },{events,side,variant,value});result.defineProperty("valueOf",descriptor);return result;
}
inline JSValue globalOrder(const JSValue& variant){
    StackScope scope;const auto originalMath=nativeGlobalBinding("Math"),originalNumber=nativeGlobalBinding("Number"),events=array();
    auto wrappedMath=JSValue::object(originalMath),wrappedNumber=JSValue::object(originalNumber);const bool coercing=variant.strictEquals("coerce")||variant.strictEquals("leftCoerceThrow")||variant.strictEquals("rightCoerceThrow");
    JSPropertyDescriptor abs;abs.configurable=true;abs.getter=JSValue::function(+[](const JSValue& self,const Args&,const Args& c)->JSValue{
        push(c[0],array({"get:abs",self.sameValue(c[1])}));if(c[2].strictEquals("getAbsThrow"))throw JSThrownValue("get:abs");
        return JSValue::function(+[](const JSValue& self,const Args& args,const Args& c)->JSValue{
            push(c[0],array({"call:abs",self.sameValue(c[1]),arg(args,0)}));if(c[2].strictEquals("callAbsThrow"))throw JSThrownValue("call:abs");
            if(c[2].strictEquals("reenter")){nativeMaterialXNodesBinding("mx_add").call({}, {node("float",{.2}),node("float",{.3})});push(c[0],"reenter");}
            return c[3].toBoolean()?coercion(c[0],"left",c[2],1):c[4].get("abs").call(self,{arg(args,0)});
        },c);
    },{events,wrappedMath,variant,coercing,originalMath});wrappedMath.defineProperty("abs",abs);
    JSPropertyDescriptor epsilon;epsilon.configurable=true;epsilon.getter=JSValue::function(+[](const JSValue& self,const Args&,const Args& c)->JSValue{push(c[0],array({"get:EPSILON",self.sameValue(c[1])}));if(c[2].strictEquals("epsilonThrow"))throw JSThrownValue("get:EPSILON");return c[3].toBoolean()?coercion(c[0],"right",c[2],.5):c[4].get("EPSILON");},{events,wrappedNumber,variant,coercing,originalNumber});wrappedNumber.defineProperty("EPSILON",epsilon);
    auto global=nativeGlobalObject();JSValue result,value;global.set("Math",wrappedMath);global.set("Number",wrappedNumber);
    try{value=nativeMaterialXNodesBinding("mx_place2d").call({}, {node("vec2",{.2,.3}),node("vec2",{.1,.2}),node("vec2",{2,3}),node("float",{.5}),node("vec2",{.4,.5}),1});result="success";}
    catch(...){result=array({"throw",nativeExceptionValue(std::current_exception())});}
    global.set("Math",originalMath);global.set("Number",originalNumber);auto out=object();out.set("events",events);out.set("result",result);
    if(result.strictEquals("success")){auto root=object();root.set("result",value);root.set("stack",scope.stack.get("nodes"));out.set("graph",Graph()(root));}return out;
}
inline JSValue run(const JSValue& request){
    std::string diagnosticStep="entry";
    try{
        initialize();
        const auto m=request.get("module").toString();if(request.get("mode").strictEquals("metadata"))return metadata(m);
        if(request.get("mode").strictEquals("globalOrder"))return globalOrder(request.get("variant"));
        StackScope scope;diagnosticStep="builder";auto b=builder();diagnosticStep="decode";Args args;for(const auto& value:values(request.get("args")))args.push_back(decode(value));const auto inputs=array(args),f=binding(m,request.get("fn").toString());auto out=object();out.set("inputs",inputs);
        if(request.get("mode").strictEquals("overload")){
            const auto overload=f.call({},args),candidate=call(overload,"getCandidateFn",{b});JSValue name;
            for(const auto& d:values(declarations().get(m)))if(binding(m,d.get("name").toString()).sameValue(candidate)){name=d.get("name");break;}
            out.set("candidate",name);out.set("returnType",call(overload,"getNodeType",{b}));out.set("setup",call(overload,"setup",{b}));out.set("cached",overload.get("_candidateFn").sameValue(candidate));return Graph()(out);
        }
        diagnosticStep="body";const auto result=request.get("kind").strictEquals("Fn")?f.get("shaderNode").get("jsFunc").call({}, {inputs,b}):f.call({},args);out.set("result",result);out.set("stack",scope.stack.get("nodes"));diagnosticStep="controls";out.set("controls",controls(out,b));
        if(request.get("build").toBoolean()){
            auto build=object(),steps=array();for(const auto* stage:{"setup","analyze","generate"}){b.set("buildStage",stage);push(steps,call(result,"build",{b,"float"}));}build.set("steps",steps);build.set("flow",b.get("flow"));out.set("build",build);
        }
        diagnosticStep="graph";return Graph()(out);
    }catch(...){const auto error=nativeExceptionValue(std::current_exception());auto out=object(),info=object();info.set("name",error.isObject()?error.get("name"):JSValue("thrown-value"));if(request.get("diagnostic").toBoolean()){info.set("message",error.isObject()?error.get("message"):error);info.set("step",diagnosticStep);}out.set("error",info);return out;}
}
}
