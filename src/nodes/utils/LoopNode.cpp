#include "nodes/utils/LoopNode.h"

std::string LoopNode::varName(int index) {
    // The letter i plus the index, character by character.
    return std::string(1, static_cast<char>('i' + index));
}

std::vector<std::string> LoopNode::internalParams(bool firstIsFunction) {
    if (firstIsFunction) {
        // A constant true in front, so the callback becomes the body of an unbounded loop.
        return {"bool(true)", "body"};
    }
    return {"descriptors", "body"};
}

int LoopNode::descriptorCount(int paramCount) {
    // Everything but the last, so a one entry list has none.
    const int count = paramCount - 1;
    return count < 0 ? 0 : count;
}

std::string LoopNode::paramName(bool paramIsNode, bool hasName, const std::string& name,
                                int index) {
    if (!paramIsNode && hasName) return name;
    return varName(index);
}

std::string LoopNode::paramType(bool paramIsNode, bool hasType, const std::string& type) {
    if (!paramIsNode && hasType) return type;
    return kDefaultType;
}

bool LoopNode::bodyIsInvokedOnce() {
    return true;
}

bool LoopNode::isCacheable() {
    return false;
}

#include "nodes/core/Node.h"
#include "nodes/code/ExpressionNode.h"
#include "nodes/tsl/TSLBase.h"
#include "nodes/utils/UtilityNodeRuntime.h"
#include "core/JSRegExp.h"
#include "core/JSStringEncoding.h"
#include "core/JSGlobal.h"
#include "utils.h"
#include <algorithm>
namespace {
using namespace UtilityNodeRuntime;
#include "nodes/utils/LoopNodeSource.inc"
JSWeakValue& lexical(){static thread_local JSWeakValue weak;return weak;}
JSValue array(const Args& values){auto result=JSValue::array();for(std::size_t i=0;i<values.size();++i)own(result,std::to_string(i).c_str(),values[i]);return result;}
JSValue named(const JSValue& node,const JSValue& builder,const JSValue& index){return invoke(node,"getVarName")({index});}
JSValue delta(const JSValue& condition){return invoke(condition,"includes")({"<"}).toBoolean()?JSValue("+="):JSValue("-=");}
JSValue method(const JSValue& receiver,const Args& args,const Args& c){
    auto self=receiver;const auto key=c[0],imports=c[1],builder=arg(args,0);
    if(key.strictEquals("isCacheable"))return false;
    if(key.strictEquals("getVarName")){
        const auto string=nativeGlobalBinding("String");const auto fromCharCode=invoke(string,"fromCharCode");const auto code=invoke(JSValue("i"),"charCodeAt")({0});return fromCharCode({add(code,builder)});
    }
    if(key.strictEquals("_getInternalParams")){
        const auto params=self.get("params");if(params.get("0").isCallable()){const auto first=imports.get("bool").call({}, {true});return array({first,params.get("0")});}return params;
    }
    if(key.strictEquals("setup")){invoke(self,"getProperties")({builder});return {};}
    if(key.strictEquals("getProperties")){
        auto properties=invoke(builder,"getNodeProperties")({self});if(!properties.get("stackNode").isUndefined())return properties;
        auto inputs=JSValue::object();const auto params=invoke(self,"_getInternalParams")();const auto length=params.get("length").toNumber()-1;
        for(double i=0;i<length;++i){const auto param=params.get(JSValue(i));auto name=!param.get("isNode").strictEquals(true)?param.get("name"):JSValue(false);if(!name.toBoolean())name=named(self,builder,i);auto type=!param.get("isNode").strictEquals(true)?param.get("type"):JSValue(false);if(!type.toBoolean())type="int";const auto inputKey=name.toPropertyKey();const auto expression=imports.get("expression").call({}, {name,type});inputs.set(inputKey,expression);}
        const auto stack=invoke(builder,"addStack")();const auto fn=params.get(JSValue(params.get("length").toNumber()-1));const auto fnCall=fn.call(params,{inputs});
        auto context=JSValue::object();own(context,"nodeLoop",self);own(context,"nodeBlock",fnCall);properties.set("returnsNode",invoke(fnCall,"context")({context}));properties.set("stackNode",stack);
        const auto base=params.get("0");if(!base.get("isNode").strictEquals(true)&&base.get("update").isCallable()){
            const auto call=imports.get("Fn").call({}, {base.get("update")}).call({}, {inputs});auto updateContext=JSValue::object();own(updateContext,"nodeLoop",self);properties.set("updateNode",invoke(call,"context")({updateContext}));
        }
        invoke(builder,"removeStack")();return properties;
    }
    const auto properties=invoke(self,"getProperties")({builder}),params=invoke(self,"_getInternalParams")(),stack=properties.get("stackNode");
    const auto length=params.get("length").toNumber()-1;
    for(double i=0;i<length;++i){
        const auto param=params.get(JSValue(i));bool isWhile=false;JSValue start=nullptr,end=nullptr,name=nullptr,type=nullptr,condition=nullptr,update=nullptr;
        if(param.get("isNode").toBoolean()){
            if(invoke(param,"getNodeType")({builder}).strictEquals("bool")){isWhile=true;type="bool";end=invoke(param,"build")({builder,type});}
            else{type="int";name=named(self,builder,i);start="0";end=invoke(param,"build")({builder,type});condition="<";}
        }else{
            type=param.get("type");if(!type.toBoolean())type="int";name=param.get("name");if(!name.toBoolean())name=named(self,builder,i);start=param.get("start");end=param.get("end");condition=param.get("condition");update=param.get("update");
            if(start.type()==JSValue::Type::Number)start=invoke(builder,"generateConst")({type,start});else if(start.toBoolean()&&start.get("isNode").toBoolean())start=invoke(start,"build")({builder,type});
            if(end.type()==JSValue::Type::Number)end=invoke(builder,"generateConst")({type,end});else if(end.toBoolean()&&end.get("isNode").toBoolean())end=invoke(end,"build")({builder,type});
            if(!start.isUndefined()&&end.isUndefined()){start=add(start," - 1");end="0";condition=">=";}
            else if(!end.isUndefined()&&start.isUndefined()){start="0";condition="<";}
            if(condition.isUndefined()){const auto a=start.toNumber(),b=end.toNumber();condition=a>b?">=":"<";}
        }
        JSValue snippet;
        if(isWhile)snippet="while ( "+end.toString()+" )";
        else{
            const auto startSnippet=start,endSnippet=end;JSValue updateSnippet;
            if(!update.isUndefined()&&!update.isNull()){
                if(update.isCallable()){const auto flow=invoke(builder,"flowStagesNode")({properties.get("updateNode"),"void"});const auto replace=invoke(flow.get("code"),"replace");updateSnippet=replace({createNativeRegExp("\\t|;","g"),""});}
                else if(update.type()==JSValue::Type::Number){const auto first=add(add(name," "),delta(condition));const auto prefix=add(first," ");updateSnippet=add(prefix,invoke(builder,"generateConst")({type,update}));}
                else if(update.type()==JSValue::Type::String)updateSnippet=add(add(name," "),update);
                else if(update.get("isNode").toBoolean()){const auto first=add(add(name," "),delta(condition));const auto prefix=add(first," ");updateSnippet=add(prefix,invoke(update,"build")({builder}));}
                else{ThreeUtils::error({"TSL: 'Loop( { update: ... } )' is not a function, string or number.",self.get("stackTrace")});updateSnippet="break /* invalid update */";}
            }else{if(type.strictEquals("int")||type.strictEquals("uint"))update=invoke(condition,"includes")({"<"}).toBoolean()?"++":"--";else update=add(delta(condition)," 1.");updateSnippet=add(add(name," "),update);}
            const auto declaration=add(add(invoke(builder,"getVar")({type,name})," = "),startSnippet);const auto conditional=add(add(add(add(name," "),condition)," "),endSnippet);snippet="for ( "+declaration.toString()+"; "+conditional.toString()+"; "+updateSnippet.toString()+" )";
        }
        const auto addFlow=invoke(builder,"addFlowCode");const auto prefix=add(i==0?JSValue("\n"):JSValue(""),builder.get("tab"));const auto code=add(add(prefix,snippet)," {\n\n");invoke(addFlow({code}),"addFlowTab")();
    }
    const auto flowBlock=builder.get("flowBlock");auto block=JSValue::object();own(block,"parent",flowBlock);auto b=builder;b.set("flowBlock",block);
    const auto stackSnippet=invoke(stack,"build")({builder,"void"});invoke(properties.get("returnsNode"),"build")({builder,"void"});b.set("flowBlock",flowBlock);
    const auto removed=invoke(builder,"removeFlowTab")();const auto addFlow=invoke(removed,"addFlowCode");addFlow({add(add("\n",builder.get("tab")),stackSnippet)});
    const auto remaining=params.get("length").toNumber()-1;for(double i=0;i<remaining;++i){const auto addCode=invoke(builder,"addFlowCode");invoke(addCode({add(i==0?JSValue(""):builder.get("tab"),"}\n\n")}),"removeFlowTab")();}
    invoke(builder,"addFlowTab")();return {};
}
}
JSValue nativeLoopNodeModule(){
    using namespace UtilityNodeRuntime;static thread_local JSWeakValue weak;if(const auto m=weak.lock();m&&m->isObject())return *m;static thread_local bool initializing=false;JSModuleInitializationScope scope(initializing,"LoopNode module");
    const auto base=nativeNodeConstructor(),expression=nativeExpressionNodeModule(),tsl=nativeTSLBase();auto imports=JSValue::object(nullptr);own(imports,"expression",nativeExpressionNodeBinding("expression"));for(const auto* n:{"nodeArray","Fn","bool"})own(imports,n,nativeTSLBaseBinding(n));auto holder=JSValue::object(nullptr);
    auto ctor=JSValue::builtinConstructor(+[](const JSValue& target,const Args& a,const Args& c)->JSValue{const auto params=arg(a,0,JSValue::array());auto self=superConstruct(target,c,{"void"});self.set("params",params);return self;},{holder},"LoopNode",0);holder.set("constructor",ctor);ctor.setPrototype(base);ctor.setFunctionSourceText(LoopNodeSource::classSource);auto p=ctor.get("prototype");p.setPrototype(base.get("prototype"));
    auto getter=JSValue::function(+[](const JSValue&,const Args&,const Args&)->JSValue{return "LoopNode";},{},"get type",0);getter.setFunctionSourceText(LoopNodeSource::typeSource);JSPropertyDescriptor type;type.getter=getter;type.configurable=true;ctor.defineProperty("type",type);
    for(const auto& [key,arity]:std::initializer_list<std::pair<const char*,unsigned>>{{"isCacheable",0},{"getVarName",1},{"getProperties",1},{"_getInternalParams",0},{"setup",1},{"generate",1}}){auto fn=JSValue::function(method,{key,imports},key,arity);const std::string k=key;fn.setFunctionSourceText(k=="isCacheable"?LoopNodeSource::isCacheableSource:k=="getVarName"?LoopNodeSource::getVarNameSource:k=="getProperties"?LoopNodeSource::getPropertiesSource:k=="_getInternalParams"?LoopNodeSource::_getInternalParamsSource:k=="setup"?LoopNodeSource::setupSource:LoopNodeSource::generateSource);JSPropertyDescriptor d;d.value=fn;d.writable=d.configurable=true;p.defineProperty(key,d);}
    auto bindings=JSValue::object(nullptr);own(bindings,"default",ctor);for(const auto* name:{"Loop","Continue","Break"}){auto fn=JSValue::function(+[](const JSValue&,const Args& a,const Args& c)->JSValue{JSValue node;if(c[0].strictEquals("Loop")){const auto params=array(a);node=c[1].construct({c[2].get("nodeArray").call({}, {params,"int"})});}else node=c[2].get("expression").call({}, {c[0].strictEquals("Break")?JSValue("break"):JSValue("continue")});return invoke(node,"toStack")();},{name,ctor,imports},name,0);fn.setFunctionSourceText(std::string(name)=="Loop"?LoopNodeSource::arrow1:std::string(name)=="Continue"?LoopNodeSource::arrow2:LoopNodeSource::arrow3);own(bindings,name,fn);}
    auto module=JSValue::object();auto names=bindings.ownPropertyNames();std::sort(names.begin(),names.end());for(const auto& name:names)own(module,name.c_str(),bindings.get(name));bindings.retainForHeapLifetime();lexical()=JSWeakValue(bindings);module.retainForHeapLifetime();weak=JSWeakValue(module);return module;
}
JSValue nativeLoopNodeBinding(const std::string& name){const auto keeper=nativeLoopNodeModule();return lexical().lock()->get(name);}
JSValue nativeLoopNodeConstructor(){return nativeLoopNodeBinding("default");}
