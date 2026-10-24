#include "animation/AnimationUtils.h"
#include "core/JSMap.h"
#include "nodes/core/Node.h"
#include "nodes/core/StackNode.h"
#include "utils.h"
#include <iostream>

namespace {
using Args=std::vector<JSValue>;
JSValue arg(const Args& args,std::size_t i){return i<args.size()?args[i]:JSValue{};}
JSValue call(const JSValue& self,const char* name,const Args& args={}){return self.get(name).call(self,args);}
JSValue array(const Args& args={}){auto out=JSValue::array();for(const auto& value:args)call(out,"push",{value});return out;}
void getter(JSValue object,const char* key,const JSValue& fn){JSPropertyDescriptor d;d.getter=fn;d.enumerable=d.configurable=true;object.defineProperty(key,d);}
JSValue normal(const JSValue& value,const JSValue& state){
    if(value.isUndefined())return "#undefined";if(value.isCallable())return "#function";
    if(value.isObject()){
        if(value.strictEquals(state.get("stack")))return "#stack";
        if(value.strictEquals(state.get("builder")))return "#builder";
        if(!value.get("fixtureName").isUndefined())return "@"+value.get("fixtureName").toString();
        if(value.type()==JSValue::Type::Array){auto out=JSValue::array();for(unsigned i=0;i<value.length();++i)call(out,"push",{normal(value.get(std::to_string(i)),state)});return out;}
        return "#object";
    }return value;
}
void event(const JSValue& state,const Args& items){auto row=JSValue::array();for(const auto& item:items)call(row,"push",{normal(item,state)});call(state.get("events"),"push",{row});}
JSValue makeNode(const JSValue& spec,const JSValue& state){
    auto object=JSValue::object();object.set("fixtureName",spec.get("name"));object.set("isNode",spec.hasOwn("isNode")?spec.get("isNode"):JSValue(true));
    if(spec.get("trace").toBoolean())getter(object,"isVarNode",JSValue::function([](const JSValue&,const Args&,const Args& c)->JSValue{event(c[0],{"get",c[1].get("name"),"isVarNode"});return c[1].get("isVarNode");},{state,spec}));
    else object.set("isVarNode",spec.get("isVarNode"));
    for(const auto* name:{"isIntent","isAssign","build","getElementType","getNodeType","getMemberType"}){
        auto function=JSValue::function([](const JSValue& self,const Args& args,const Args& c)->JSValue{
            const auto state=c[0],spec=c[1];const auto name=c[2].string();
            Args row{name,spec.get("name"),array(args)};
            if(name=="build"){row.push_back(nativeGetCurrentStack());row.push_back(state.get("stack").get("_currentNode"));}
            event(state,row);
            if(spec.get("throw").strictEquals(name))throw JSTypeError("fixture");
            if(name=="isIntent")return spec.get("intent");if(name=="isAssign")return spec.get("assign");
            if(name=="getElementType")return spec.hasOwn("elementType")?spec.get("elementType"):JSValue("element");
            if(name=="getNodeType")return spec.hasOwn("nodeType")?spec.get("nodeType"):JSValue("float");
            if(name=="getMemberType")return "member";
            if(spec.get("changeBase").toBoolean())StackNode::prototype().setPrototype(state.get("alternateBase"));
            if(spec.hasOwn("changeStage")){auto builder=state.get("builder");builder.set("buildStage",spec.get("changeStage"));}
            if(spec.get("insert").toBoolean()&&!self.get("inserted").toBoolean()){
                auto receiver=self;receiver.set("inserted",true);const auto stack=state.get("stack"),nodes=state.get("children");
                for(const auto& key:{"insertBefore","insertAfter"}){const auto target=spec.get(key);if(!target.isUndefined())call(stack,key==std::string("insertBefore")?"addToStackBefore":"addToStack",{nodes.get(target.toString())});}
            }
            return "built:"+spec.get("name").toString();
        },{state,spec,name},name,0);
        if(spec.get("trace").toBoolean())getter(object,name,JSValue::function([](const JSValue&,const Args&,const Args& c)->JSValue{event(c[0],{"get",c[1],c[2]});return c[3];},{state,spec.get("name"),name,function}));
        else object.set(name,function);
    }return object;
}
JSValue run(const JSValue& request){
    auto state=JSValue::object();state.set("events",array());auto stack=nativeStackNodeConstructor().construct();stack.set("fixtureName","root");state.set("stack",stack);
    auto previous=JSValue::object();previous.set("fixtureName","previous");nativeSetCurrentStack(previous);
    auto builder=JSValue::object();builder.set("buildStage",request.hasOwn("stage")?request.get("stage"):JSValue("setup"));builder.set("shaderStage","vertex");state.set("builder",builder);
    auto children=JSValue::object(),specs=JSValue::object();state.set("children",children);state.set("specs",specs);const auto list=request.get("nodes");
    if(list.type()==JSValue::Type::Array)for(unsigned i=0;i<list.length();++i){const auto spec=list.get(std::to_string(i));children.set(spec.get("name").toString(),makeNode(spec,state));specs.set(spec.get("name").toString(),spec);}
    const auto kind=request.get("kind").string();auto result=JSValue::object();
    if(kind=="constructor"){
        auto parent=JSValue::object(),object=nativeStackNodeConstructor().construct({parent});auto fields=JSValue::array();for(const auto& key:object.ownPropertyNames())call(fields,"push",{key});
        result.set("fields",fields);result.set("parent",object.get("parent").strictEquals(parent));result.set("defaultParent",stack.get("parent"));result.set("nodes",normal(stack.get("nodes"),state));
        result.set("isNode",stack.get("isNode"));result.set("isStackNode",stack.get("isStackNode"));result.set("type",stack.get("type"));result.set("isMap",isNativeMap(stack.get("_nodeDataLibrary")));
        result.set("defaults",array({stack.get("outputNode"),stack.get("_currentCond"),stack.get("_expressionNode"),stack.get("_currentNode")}));
        auto methods=JSValue::array();for(const auto* name:{"isCacheable","getElementType","generateNodeType","getMemberType","addToStack","addToStackBefore","setup","build"}){const auto fn=StackNode::prototype().get(name);call(methods,"push",{array({name,fn.get("length"),fn.isConstructible()})});}result.set("methods",methods);nativeSetCurrentStack(nullptr);return result;
    }
    const auto initial=request.get("initial");if(initial.type()==JSValue::Type::Array)for(unsigned i=0;i<initial.length();++i)call(stack.get("nodes"),"push",{children.get(initial.get(std::to_string(i)).toString())});
    if(request.hasOwn("current"))stack.set("_currentNode",children.get(request.get("current").toString()));
    if(request.hasOwn("output"))stack.set("outputNode",children.get(request.get("output").toString()));
    auto properties=JSValue::object();if(request.hasOwn("propertyOutput"))properties.set("outputNode",request.get("propertyOutput"));state.set("properties",properties);
    for(const auto* method:{"setActiveStack","removeActiveStack","getDataFromNode","getNodeProperties"})builder.set(method,JSValue::function([](const JSValue&,const Args& args,const Args& c)->JSValue{
        const auto state=c[0],request=c[1];const auto name=c[2].string();event(state,{name,array(args),nativeGetCurrentStack()});
        if(request.get("throwBuilder").strictEquals(name))throw JSTypeError("builder");
        if(name=="getNodeProperties")return state.get("properties");
        if(name=="getDataFromNode"){
            const auto spec=state.get("specs").get(arg(args,0).get("fixtureName").toString());auto out=JSValue::object();
            if(spec.hasOwn("parents")){auto stages=JSValue::object(),parents=JSValue::array();const auto values=spec.get("parents");for(unsigned i=0;i<values.length();++i){const auto parent=values.get(std::to_string(i));call(parents,"push",{parent.strictEquals("stack")?state.get("stack"):parent.strictEquals("false")?JSValue(false):JSValue::object()});}stages.set("vertex",parents);out.set("stages",stages);}return out;
        }return {};
    },{state,request,method}));
    auto basePrototype=nativeNodeConstructor().get("prototype");const auto originalBuild=*basePrototype.getOwnPropertyDescriptor("build");bool replaceBase=false;
    auto stackPrototype=StackNode::prototype();const auto originalParent=stackPrototype.getPrototype();
    try{
        if(request.hasOwn("baseOverride")){
            const auto mode=request.get("baseOverride").string();auto alternate=JSValue::object(basePrototype);
            const auto function=JSValue::function([](const JSValue& self,const Args& args,const Args& c)->JSValue{
                event(c[0],{"alternate.build",self,array(args)});if(c[1].strictEquals("method-throw"))throw JSTypeError("alternate");return "alternate-result";
            },{state,mode});
            if(mode=="getter"||mode=="getter-throw"||mode=="getter-noncallable"||mode=="ancestor-getter"){
                getter(alternate,"build",JSValue::function([](const JSValue& self,const Args&,const Args& c)->JSValue{
                    event(c[0],{"get:alternate.build",self});if(c[1].strictEquals("getter-throw"))throw JSTypeError("alternate getter");if(c[1].strictEquals("getter-noncallable"))return 17;return c[2];
                },{state,mode,function}));
            }else alternate.set("build",function);
            if(mode=="ancestor-getter")alternate=JSValue::object(alternate);
            if(mode=="null")alternate=nullptr;
            state.set("alternateBase",alternate);if(!request.get("deferBase").toBoolean())stackPrototype.setPrototype(alternate);
        }
        if(kind=="add"){
            ThreeUtils::setConsoleFunction(JSValue::function([](const JSValue&,const Args& args,const Args& c)->JSValue{event(c[0],{"error",arg(args,0),arg(args,1).get("startsWith").call(arg(args,1),{"THREE.TSL: Invalid node added to stack."})});return {};},{state}));
            auto returns=JSValue::array();const auto operations=request.get("operations");
            for(unsigned i=0;i<operations.length();++i){const auto op=operations.get(std::to_string(i));if(op.hasOwn("current")){stack.set("_currentNode",op.get("current").isNull()?JSValue(nullptr):children.get(op.get("current").toString()));continue;}
                Args args{children.get(op.get("node").toString())};if(op.hasOwn("index"))args.push_back(op.get("index").strictEquals("#undefined")?JSValue{}:op.get("index"));call(returns,"push",{call(stack,op.get("before").toBoolean()?"addToStackBefore":"addToStack",args).strictEquals(stack)});
            }result.set("returns",returns);ThreeUtils::setConsoleFunction({});
        }else if(kind=="types"){
            result.set("cacheable",call(stack,"isCacheable",{builder}));result.set("values",array({call(stack,"getElementType",{builder}),call(stack,"generateNodeType",{builder}),call(stack,"getMemberType",{builder,"field"})}));
        }else if(kind=="setup"){
            if(request.get("overrideChildren").toBoolean())stack.set("getChildren",JSValue::function([](const JSValue&,const Args&,const Args& c)->JSValue{event(c[0],{"getChildren"});return call(c[0].get("stack").get("nodes"),"values");},{state}));
            result.set("value",normal(call(stack,"setup",{builder}),state));auto values=JSValue::object();for(const auto& key:properties.ownPropertyNames())values.set(key,normal(properties.get(key),state));result.set("properties",values);
        }else if(kind=="build"){
            if(request.get("baseStub").toBoolean()){
                JSPropertyDescriptor d;d.value=JSValue::function([](const JSValue&,const Args& args,const Args& c)->JSValue{event(c[0],{"super.build",array(args)});return "base-result";},{state});d.writable=d.configurable=true;basePrototype.defineProperty("build",d);replaceBase=true;
            }
            Args args{builder,"float",17};result.set("value",normal(call(stack,"build",args),state));
        }
    }catch(const JSTypeError&){result.set("error","TypeError");}
    stackPrototype.setPrototype(originalParent);
    if(replaceBase)basePrototype.defineProperty("build",originalBuild);
    result.set("events",state.get("events"));result.set("nodes",normal(stack.get("nodes"),state));result.set("current",normal(stack.get("_currentNode"),state));result.set("currentStack",normal(nativeGetCurrentStack(),state));result.set("stage",builder.get("buildStage"));
    auto delta=JSValue::object();for(const auto& name:children.ownPropertyNames()){const auto item=call(stack.get("_nodeDataLibrary"),"get",{children.get(name)});if(!item.isUndefined())delta.set(name,item.get("delta"));}result.set("delta",delta);
    nativeSetCurrentStack(nullptr);ThreeUtils::setConsoleFunction({});return result;
}
}
int main(){try{const std::string text(std::istreambuf_iterator<char>(std::cin),{});const auto requests=Animation_detail::parseJSON(text);auto out=JSValue::array();for(unsigned i=0;i<requests.length();++i)call(out,"push",{run(requests.get(std::to_string(i)))});std::cout<<out.toJSON()->dump();return 0;}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}}
