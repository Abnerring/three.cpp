#include "nodes/gpgpu/ComputeNode.h"
#include "nodes/core/IndexNode.h"
#include "nodes/core/StackTrace.h"
#include "nodes/core/UniformNode.h"
#include "nodes/tsl/TSLCore.h"
#include "nodes/utils/UtilityNodeRuntime.h"
#include "utils.h"
namespace {
using namespace UtilityNodeRuntime;
JSWeakValue& computeBindings(){static thread_local JSWeakValue weak;return weak;}
JSValue computeMethod(const JSValue& receiver,const Args& args,const Args& captures){
    auto self=receiver;const auto name=captures[0],bindings=captures[2];const auto builder=arg(args,0);
    if(name.strictEquals("isCacheable"))return false;
    if(name.strictEquals("setName")){self.set("name",arg(args,0));return self;}
    if(name.strictEquals("label")){ThreeUtils::warn({"TSL: \"label()\" has been deprecated. Use \"setName()\" instead.",bindings.get("StackTrace").construct()});return invoke(self,"setName")({arg(args,0)});}
    if(name.strictEquals("onInit")){self.set("onInitFunction",arg(args,0));return self;}
    if(name.strictEquals("getUpdateBeforeType"))return builder.get("compute").strictEquals(self)?bindings.get("NodeUpdateType").get("NONE"):self.get("updateBeforeType");
    if(name.strictEquals("updateBefore")){const auto renderer=builder.get("renderer");invoke(renderer,"compute")({self});return {};}
    if(name.strictEquals("setup")){
        if(!self.get("count").isNull()&&self.get("countNode").isNull()){
            const auto uniform=bindings.get("uniform").call({}, {self.get("count"),"uint"});const auto update=invoke(uniform,"onObjectUpdate");
            const auto callback=JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue{return c[0].get("count");},{self},"",0);self.set("countNode",update({callback}));
        }
        const auto result=invoke(self.get("computeNode"),"build")({builder});
        if(result.toBoolean()){
            auto properties=invoke(builder,"getNodeProperties")({self});properties.set("outputComputeNode",result.get("outputNode"));auto value=result;value.set("outputNode",nullptr);
        }return result;
    }
    if(name.strictEquals("generate")){
        const auto stage=builder.get("shaderStage");if(stage.strictEquals("compute")){
            const auto snippet=invoke(self.get("computeNode"),"build")({builder,"void"});if(!snippet.strictEquals(""))invoke(builder,"addLineFlowCode")({snippet,self});
            if(!self.get("count").isNull()&&builder.get("allowEarlyReturns").strictEquals(true)){
                const auto countSnippet=invoke(self.get("countNode"),"build")({builder,"uint"});const auto indexSnippet=invoke(bindings.get("instanceIndex"),"build")({builder,"uint"});
                // Assignment resolves the left-hand flow object before the template.
                auto flow=builder.get("flow");const auto tab=builder.get("tab").toString(),index=indexSnippet.toString(),count=countSnippet.toString(),code=builder.get("flow").get("code").toString();flow.set("code",tab+"if ( "+index+" >= "+count+" ) { return; }\n\n"+code);
            }
        }else{
            const auto properties=invoke(builder,"getNodeProperties")({self}),output=properties.get("outputComputeNode");if(output.toBoolean())return invoke(output,"build")({builder,arg(args,1)});
        }return {};
    }throw std::logic_error("Unknown ComputeNode method");
}
}
JSValue nativeComputeNodeModule(){
    using namespace UtilityNodeRuntime;static thread_local JSWeakValue weak;if(const auto value=weak.lock();value&&value->isObject())return *value;
    static thread_local bool initializing=false;JSModuleInitializationScope scope(initializing,"ComputeNode module");const auto base=nativeNodeConstructor(),instance=nativeIndexNodeBinding("instanceIndex"),stack=nativeStackTraceConstructor(),uniformCtor=nativeUniformNodeConstructor(),uniform=nativeUniformFactory(),updates=nativeNodeConstantsBinding("NodeUpdateType"),tsl=nativeTSLCore();
    auto bindings=JSValue::object(nullptr);own(bindings,"instanceIndex",instance);own(bindings,"StackTrace",stack);own(bindings,"uniform",uniform);own(bindings,"NodeUpdateType",updates);
    auto holder=JSValue::object(nullptr);auto ctor=JSValue::builtinConstructor(+[](const JSValue& target,const Args& args,const Args& captures)->JSValue{
        auto self=superConstruct(target,captures,{"void"});self.set("isComputeNode",true);self.set("computeNode",arg(args,0));self.set("workgroupSize",arg(args,1));self.set("count",nullptr);self.set("dispatchSize",nullptr);self.set("version",1);self.set("name","");self.set("onInitFunction",nullptr);self.set("countNode",nullptr);self.set("updateBeforeType",captures[1].get("FRAME"));return self;
    },{holder,updates},"ComputeNode",2);holder.set("constructor",ctor);ctor.setPrototype(base);auto prototype=ctor.get("prototype");prototype.setPrototype(base.get("prototype"));
    JSPropertyDescriptor type;type.configurable=true;type.getter=JSValue::function(+[](const JSValue&,const Args&,const Args&)->JSValue{return "ComputeNode";},{},"get type",0);ctor.defineProperty("type",type);
    for(const auto& [name,arity]:std::initializer_list<std::pair<const char*,unsigned>>{{"isCacheable",0},{"setName",1},{"label",1},{"onInit",1},{"getUpdateBeforeType",1},{"updateBefore",1},{"setup",1},{"generate",2}}){JSPropertyDescriptor d;d.value=JSValue::function(computeMethod,{name,prototype,bindings},name,arity);d.writable=d.configurable=true;prototype.defineProperty(name,d);}ctor.retainForHeapLifetime();
    own(bindings,"default",ctor);
    const auto nodeObject=nativeTSLCoreBinding("nodeObject");
    const auto kernel=JSValue::function(+[](const JSValue&,const Args& args,const Args& c)->JSValue{
        auto size=arg(args,1);if(size.isUndefined()){size=JSValue::array();own(size,"0",64);}
        if(size.get("length").strictEquals(0)||greater(size.get("length"),3))ThreeUtils::error({"TSL: compute() workgroupSize must have 1, 2, or 3 elements",c[2].construct()});
        for(double i=0;less(i,size.get("length"));++i){
            const auto value=size.get(JSValue(i));if(value.type()!=JSValue::Type::Number||greaterEqual(0,value)||!std::isfinite(value.number())||std::floor(value.number())!=value.number())ThreeUtils::error({"TSL: compute() workgroupSize element at index [ "+JSValue(i).toString()+" ] must be a positive integer",c[2].construct()});
        }
        while(less(size.get("length"),3))invoke(size,"push")({1});const auto node=c[1].call({}, {arg(args,0)});return c[0].construct({node,size});
    },{ctor,nodeObject,stack},"computeKernel",1);own(bindings,"computeKernel",kernel);
    own(bindings,"compute",JSValue::function(+[](const JSValue&,const Args& args,const Args& c)->JSValue{auto node=c[0].call({}, {arg(args,0),arg(args,2)});const auto count=arg(args,1);node.set(count.type()==JSValue::Type::Number?"count":"dispatchSize",count);return node;},{kernel},"compute",3));
    const auto chain=nativeTSLCoreBinding("addMethodChaining");chain.call({}, {"compute",bindings.get("compute")});chain.call({}, {"computeKernel",kernel});auto module=JSValue::object();for(const auto* name:{"compute","computeKernel","default"})own(module,name,bindings.get(name));
    bindings.retainForHeapLifetime();computeBindings()=JSWeakValue(bindings);module.retainForHeapLifetime();weak=JSWeakValue(module);return module;
}
JSValue nativeComputeNodeBinding(const std::string& name){const auto keeper=nativeComputeNodeModule();return computeBindings().lock()->get(name);}
JSValue nativeComputeNodeConstructor(){return nativeComputeNodeBinding("default");}
ComputeNode::ComputeNode(const JSValue& node,const JSValue& size):object_(constructor().construct({node,size})){}
#define THREE_COMPUTE_METHOD(name) JSValue ComputeNode::name(const JSValue& value)const{return object_.get(#name).call(object_,{value});}
THREE_COMPUTE_METHOD(setName)
THREE_COMPUTE_METHOD(label)
THREE_COMPUTE_METHOD(onInit)
THREE_COMPUTE_METHOD(getUpdateBeforeType)
THREE_COMPUTE_METHOD(updateBefore)
THREE_COMPUTE_METHOD(setup)
#undef THREE_COMPUTE_METHOD
JSValue ComputeNode::generate(const JSValue& builder,const JSValue& output)const{return object_.get("generate").call(object_,{builder,output});}
