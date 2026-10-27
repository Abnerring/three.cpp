#include "nodes/display/ColorSpaceNode.h"
#include "nodes/display/ColorSpaceFunctions.h"
#include "nodes/utils/UtilityNodeRuntime.h"
#include "nodes/tsl/TSLCore.h"
#include "math/ColorManagement.h"
#include "math/Matrix3.h"
#include "constants.h"
#include <algorithm>

namespace {
using namespace UtilityNodeRuntime;
JSValue method(const JSValue& receiver,const Args& args,const Args& captures){
    const auto self=receiver,name=captures[0],builder=arg(args,0),bindings=captures[2],management=bindings.get("management");
    if(name.strictEquals("resolveColorSpace")){
        const auto space=arg(args,1);if(space.strictEquals("WorkingColorSpace"))return management.get("workingColorSpace");
        if(space.strictEquals("OutputColorSpace")){const auto value=builder.get("context").get("outputColorSpace");return value.toBoolean()?value:builder.get("renderer").get("outputColorSpace");}
        return space;
    }
    if(name.strictEquals("setup")){
        const auto color=self.get("colorNode");const auto resolveSource=invoke(self,"resolveColorSpace");const auto source=resolveSource({builder,self.get("source")});
        const auto resolveTarget=invoke(self,"resolveColorSpace");const auto target=resolveTarget({builder,self.get("target")});auto output=color;
        if(management.get("enabled").strictEquals(false)||source.strictEquals(target)||!source.toBoolean()||!target.toBoolean())return output;
        const auto vec4=bindings.get("vec4");
        if(invoke(management,"getTransfer")({source}).strictEquals(SRGBTransfer)){
            const auto rgb=bindings.get("EOTF").call({}, {output.get("rgb")});const auto alpha=output.get("a");output=vec4.call({}, {rgb,alpha});
        }
        const auto sourcePrimaries=invoke(management,"getPrimaries")({source});const auto targetPrimaries=invoke(management,"getPrimaries")({target});
        if(!sourcePrimaries.strictEquals(targetPrimaries)){
            const auto getMatrix=invoke(management,"_getMatrix");const auto matrix=bindings.get("Matrix3").construct();const auto converted=getMatrix({matrix,source,target});
            const auto matrixNode=bindings.get("mat3").call({}, {converted});const auto multiply=invoke(matrixNode,"mul");const auto rgb=multiply({output.get("rgb")});const auto alpha=output.get("a");output=vec4.call({}, {rgb,alpha});
        }
        if(invoke(management,"getTransfer")({target}).strictEquals(SRGBTransfer)){
            const auto rgb=bindings.get("OETF").call({}, {output.get("rgb")});const auto alpha=output.get("a");output=vec4.call({}, {rgb,alpha});
        }
        return output;
    }
    throw std::logic_error("Unknown ColorSpaceNode method");
}
JSWeakValue& lexical(){static thread_local JSWeakValue weak;return weak;}
}
JSValue nativeColorSpaceNodeConstructor(){
    using namespace UtilityNodeRuntime;
    static thread_local JSWeakValue weak;if(const auto value=weak.lock();value&&value->isConstructible())return *value;
    const auto tsl=nativeTSLCore(),management=nativeColorManagement(),functions=nativeColorSpaceFunctions();auto bindings=JSValue::object(nullptr);
    own(bindings,"management",management);own(bindings,"vec4",nativeTSLCoreBinding("vec4"));own(bindings,"mat3",nativeTSLCoreBinding("mat3"));own(bindings,"Matrix3",nativeMatrix3Constructor());
    own(bindings,"EOTF",nativeColorSpaceFunctionsBinding("sRGBTransferEOTF"));own(bindings,"OETF",nativeColorSpaceFunctionsBinding("sRGBTransferOETF"));
    auto ctor=makeConstructor("ColorSpaceNode",3,+[](const JSValue& target,const Args& args,const Args& captures)->JSValue{
        auto self=superConstruct(target,captures,{"vec4"});self.set("colorNode",arg(args,0));self.set("source",arg(args,1));self.set("target",arg(args,2));return self;
    },{{"resolveColorSpace",2},{"setup",1}},method,{bindings});weak=JSWeakValue(ctor);return ctor;
}
JSValue nativeColorSpaceNodeModule(){
    using namespace UtilityNodeRuntime;
    static thread_local JSWeakValue weak;if(const auto value=weak.lock();value&&value->isObject())return *value;
    static thread_local bool initializing=false;JSModuleInitializationScope scope(initializing,"ColorSpaceNode exports");
    const auto constructor=nativeColorSpaceNodeConstructor(),tsl=nativeTSLCore(),nodeObject=nativeTSLCoreBinding("nodeObject");auto bindings=JSValue::object(nullptr);own(bindings,"default",constructor);
    for(const auto* name:{"workingToColorSpace","colorSpaceToWorking","convertColorSpace"})own(bindings,name,JSValue::function(+[](const JSValue&,const Args& args,const Args& captures)->JSValue{
        const auto node=captures[1].call({}, {arg(args,0)}),name=captures[2];
        if(name.strictEquals("workingToColorSpace"))return captures[0].construct({node,"WorkingColorSpace",arg(args,1)});
        if(name.strictEquals("colorSpaceToWorking"))return captures[0].construct({node,arg(args,1),"WorkingColorSpace"});
        return captures[0].construct({node,arg(args,1),arg(args,2)});
    },{constructor,nodeObject,name},name,std::string_view(name)=="convertColorSpace"?3:2));
    for(const auto* name:{"workingToColorSpace","colorSpaceToWorking"})nativeTSLCoreBinding("addMethodChaining").call({}, {name,bindings.get(name)});
    auto module=JSValue::object();auto names=bindings.ownPropertyNames();std::sort(names.begin(),names.end());for(const auto& name:names)own(module,name.c_str(),bindings.get(name));
    bindings.retainForHeapLifetime();lexical()=JSWeakValue(bindings);module.retainForHeapLifetime();weak=JSWeakValue(module);return module;
}
JSValue nativeColorSpaceNodeBinding(const std::string& name){const auto keeper=nativeColorSpaceNodeModule();return lexical().lock()->get(name);}
ColorSpaceNode::ColorSpaceNode(const JSValue& color,const JSValue& source,const JSValue& target):object_(constructor().construct({color,source,target})){}
JSValue ColorSpaceNode::resolveColorSpace(const JSValue& builder,const JSValue& space)const{return object_.get("resolveColorSpace").call(object_,{builder,space});}
JSValue ColorSpaceNode::setup(const JSValue& builder)const{return object_.get("setup").call(object_,{builder});}
