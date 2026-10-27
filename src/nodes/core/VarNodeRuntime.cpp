// Managed runtime algorithms from pinned src/nodes/core/VarNode.js.
#include "nodes/core/VarNode.h"
#include "nodes/core/Node.h"
#include "nodes/tsl/TSLCore.h"
#include "core/JSModuleObject.h"
#include "utils.h"

namespace {
using Args=std::vector<JSValue>;
JSValue arg(const Args& args,std::size_t i,const JSValue& fallback={}){return i<args.size()&&!args[i].isUndefined()?args[i]:fallback;}
struct Invocation{JSValue object,method;JSValue operator()(const Args& args={})const{return method.call(object,args);}};
Invocation invoke(const JSValue& object,const char* key){return {object,object.get(key)};}
JSValue method(const JSValue& receiver,const Args& args,const Args& captures){
    auto self=receiver;const auto name=captures[0].string();const auto builder=arg(args,0);
    if(name=="isCacheable")return false;
    if(name=="setIntent"){self.set("intent",builder);return self;}
    if(name=="getIntent")return self.get("intent");
    if(name=="isIntent"){const auto data=invoke(builder,"getDataFromNode")({self});if(data.get("forceDeclaration").strictEquals(true))return false;return self.get("intent");}
    if(name=="getMemberType")return invoke(self.get("node"),"getMemberType")({builder,arg(args,1)});
    if(name=="getElementType")return invoke(self.get("node"),"getElementType")({builder});
    if(name=="generateNodeType")return invoke(self.get("node"),"getNodeType")({builder});
    if(name=="getArrayCount")return invoke(self.get("node"),"getArrayCount")({builder});
    if(name=="isAssign")return invoke(builder,"getDataFromNode")({self}).get("assign");
    if(name=="_hasStack")return !invoke(builder,"getDataFromNode")({self}).get("stack").isUndefined();
    if(name=="build"){
        const auto shared=invoke(self,"getShared")({builder});
        if(!self.sameValue(shared))return invoke(shared,"build")(args);
        if(invoke(self,"_hasStack")({builder}).strictEquals(false)&&builder.get("buildStage").strictEquals("setup")){
            const auto loop=builder.get("context").get("nodeLoop");
            if((loop.toBoolean()||builder.get("context").get("nodeBlock").toBoolean())&&builder.get("flowBlock").isNull())invoke(invoke(builder,"getBaseStack")(),"addToStack")({self});
        }else if(self.get("intent").strictEquals(true)&&builder.get("context").get("nodeLoop").toBoolean()&&builder.get("buildStage").strictEquals("analyze")&&invoke(self.get("node"),"isCacheable")({builder}).strictEquals(false)&&invoke(builder,"isDeterministic")({self.get("node")}).strictEquals(false)){
            auto data=invoke(builder,"getDataFromNode")({self});
            const auto declaration=invoke(builder.get("activeStacks"),"indexOf")({data.get("stack")});
            const auto indexOf=invoke(builder.get("activeStacks"),"indexOf");const auto loopData=invoke(builder,"getDataFromNode")({builder.get("context").get("nodeLoop")});
            const auto loop=indexOf({loopData.get("stack")});
            if(!declaration.strictEquals(-1)){const auto loopNumber=loop.toNumber();const auto declarationNumber=declaration.toNumber();if(loopNumber>=declarationNumber)data.set("forceDeclaration",true);}
        }
        if(invoke(self,"isIntent")({builder}).toBoolean()&&!invoke(self,"isAssign")({builder}).strictEquals(true))return invoke(self.get("node"),"build")(args);
        return captures[1].getPrototype().get("build",self).call(self,args);
    }
    if(name=="generate"){
        const auto node=self.get("node");const auto variableName=self.get("name");const auto readOnly=self.get("readOnly");
        const auto nodeType=invoke(self,"getNodeType")({builder});const auto comparable=nodeType.isObject()?nodeType.toPrimitive():nodeType;
        if(comparable.strictEquals("void")){
            if(!invoke(self,"isIntent")({builder}).strictEquals(true))ThreeUtils::error({"TSL: \".toVar()\" can not be used with void type.",self.get("stackTrace")});
            return invoke(node,"build")({builder});
        }
        const auto vectorType=invoke(builder,"getVectorType")({nodeType});const auto snippet=invoke(node,"build")({builder,vectorType});
        const auto getVar=invoke(builder,"getVarFromNode");const auto nodeVar=getVar({self,variableName,vectorType,{},readOnly,self.get("intent")});
        const auto property=invoke(builder,"getPropertyName")({nodeVar});auto prefix=property;
        if(nodeVar.get("readOnly").toBoolean()){
            const auto count=invoke(node,"getArrayCount")({builder});
            if(invoke(builder,"isDeterministic")({node}).toBoolean())prefix=invoke(builder,"generateConstStatement")({nodeVar.get("type"),property,count});
            else prefix=invoke(builder,"generateLetStatement")({nodeVar.get("type"),property,count});
        }else if(nodeVar.get("local").toBoolean())prefix=invoke(builder,"generateVarStatement")({nodeVar.get("type"),property,nodeVar.get("count")});
        const auto add=invoke(builder,"addLineFlowCode");const auto left=prefix.toString();const auto right=snippet.toString();add({left+" = "+right,self});return property;
    }
    throw std::logic_error("Unknown VarNode method");
}
}
JSValue nativeVarNodeConstructor(){
    static thread_local JSWeakValue weak;if(const auto ctor=weak.lock();ctor&&ctor->isConstructible())return *ctor;
    static thread_local bool initializing=false;JSModuleInitializationScope scope(initializing,"VarNode");const auto base=nativeNodeConstructor();auto holder=JSValue::object(nullptr);
    auto ctor=JSValue::builtinConstructor([](const JSValue& newTarget,const Args& args,const Args& captures)->JSValue{
        auto self=captures[0].get("constructor").getPrototype().construct({},newTarget);self.set("node",arg(args,0));self.set("name",arg(args,1,nullptr));self.set("global",true);self.set("isVarNode",true);self.set("readOnly",arg(args,2,false));self.set("parents",true);self.set("intent",false);return self;
    },{holder},"VarNode",1);holder.set("constructor",ctor);ctor.setPrototype(base);auto proto=ctor.get("prototype");proto.setPrototype(base.get("prototype"));
    JSPropertyDescriptor type;type.getter=JSValue::function([](const JSValue&,const Args&,const Args&)->JSValue{return "VarNode";},{},"get type",0);type.configurable=true;ctor.defineProperty("type",type);
    for(const auto& [name,arity]:std::initializer_list<std::pair<const char*,unsigned>>{{"isCacheable",0},{"setIntent",1},{"isIntent",1},{"getIntent",0},{"getMemberType",2},{"getElementType",1},{"generateNodeType",1},{"getArrayCount",1},{"isAssign",1},{"build",0},{"generate",1},{"_hasStack",1}}){JSPropertyDescriptor d;d.value=JSValue::function(method,{name,proto},name,arity);d.writable=d.configurable=true;proto.defineProperty(name,d);}
    ctor.retainForHeapLifetime();weak=JSWeakValue(ctor);return ctor;
}
namespace {JSWeakValue& varBindings(){static thread_local JSWeakValue weak;return weak;}}
JSValue nativeVarNodeModule(){
    static thread_local JSWeakValue weak;if(const auto module=weak.lock();module&&module->isObject())return *module;
    static thread_local bool initializing=false;JSModuleInitializationScope scope(initializing,"VarNode exports");const auto tsl=nativeTSLCore(),ctor=nativeVarNodeConstructor();
    const auto createVar=nativeTSLCoreBinding("nodeProxy").call({}, {ctor});auto module=JSValue::object();
    const auto factory=[&](const char* name,JSValue flag)->JSValue{return JSValue::function([](const JSValue&,const Args& args,const Args& cap)->JSValue{
        JSValue value;
        if(cap[1].strictEquals("intent")){value=cap[0].call({}, {arg(args,0)});value=invoke(value,"setIntent")({true});}
        else if(cap[1].strictEquals(true))value=cap[0].call({}, {arg(args,0),arg(args,1,nullptr),true});
        else value=cap[0].call({}, {arg(args,0),arg(args,1,nullptr)});return invoke(value,"toStack")();
    },{createVar,flag},name,1);};
    const auto constant=factory("Const",true),variable=factory("Var",false),intent=factory("VarIntent","intent");
    for(const auto& [name,value]:std::initializer_list<std::pair<const char*,JSValue>>{{"Const",constant},{"Var",variable},{"VarIntent",intent},{"default",ctor}}){JSPropertyDescriptor d;d.value=value;d.writable=d.enumerable=d.configurable=true;module.defineProperty(name,d);}
    nativeTSLCoreBinding("addMethodChaining").call({}, {"toVar",variable});nativeTSLCoreBinding("addMethodChaining").call({}, {"toConst",constant});nativeTSLCoreBinding("addMethodChaining").call({}, {"toVarIntent",intent});
    auto bindings=JSValue::object(nullptr);for(const auto& name:module.ownPropertyNames())bindings.set(name,module.get(name));
    bindings.retainForHeapLifetime();varBindings()=JSWeakValue(bindings);
    module.retainForHeapLifetime();weak=JSWeakValue(module);return module;
}
JSValue nativeVarNodeBinding(const std::string& name){const auto keeper=nativeVarNodeModule();return varBindings().lock()->get(name);}
VarNode::VarNode(const JSValue& node,const JSValue& name,const JSValue& readOnly):object_(constructor().construct({node,name,readOnly})){}
JSValue VarNode::build(const Args& args)const{return invoke(object_,"build")(args);}
JSValue VarNode::generate(const JSValue& builder)const{return invoke(object_,"generate")({builder});}
