#include "nodes/display/ViewportDepthNode.h"
#include "nodes/display/ViewportNodeSources.inc"
#include <string_view>

std::vector<std::string> ViewportDepthNode::scopes() {
    return {kDepthBase, kDepth, kLinearDepth};
}

bool ViewportDepthNode::answersWithFragDepth(const std::string& scope) {
    // Only the base scope.
    return scope == kDepthBase;
}

ViewportDepthNode::Result ViewportDepthNode::setupResult(const std::string& scope, bool hasValue,
                                                          bool isPerspectiveCamera) {
    if (scope == kDepthBase) {
        // Without a value there is nothing to assign, so the answer is nothing at all.
        return hasValue ? Result::AssignedBase : Result::Nothing;
    }
    if (scope == kDepth) {
        // The value is ignored here whatever it holds.
        return isPerspectiveCamera ? Result::PerspectiveDepth : Result::OrthographicDepth;
    }
    if (scope == kLinearDepth) {
        if (hasValue) {
            return isPerspectiveCamera ? Result::LinearFromValue : Result::ValueAsItStands;
        }
        return Result::LinearFromPosition;
    }
    return Result::Nothing;
}

int ViewportDepthNode::conversionsForLinearValue(bool isPerspectiveCamera) {
    // Two for a perspective camera, none for an orthographic one.
    return isPerspectiveCamera ? 2 : 0;
}

bool ViewportDepthNode::valueIsUsed(const std::string& scope) {
    // The depth scope ignores it; the other two use it when it is there.
    return scope != kDepth;
}

bool ViewportDepthNode::isCacheable() {
    return false;
}

#include "nodes/tsl/TSLBase.h"
#include "nodes/accessors/Camera.h"
#include "nodes/accessors/Position.h"
#include "nodes/display/ViewportDepthTextureNode.h"
#include "nodes/utils/UtilityNodeRuntime.h"
#include <algorithm>
#include <numbers>
namespace {
using namespace UtilityNodeRuntime;
JSWeakValue& viewportDepthLexical(){static thread_local JSWeakValue weak;return weak;}
JSValue conversion(const JSValue&,const Args& args,const Args& captures){
    const auto name=captures[0],table=captures[1];auto value=arg(args,0),near=arg(args,1);const auto far=arg(args,2);
    if(name.strictEquals("viewZToOrthographicDepth")){
        const auto numerator=invoke(value,"add")({near});const auto divide=invoke(numerator,"div");return divide({invoke(near,"sub")({far})});
    }
    if(name.strictEquals("viewZToReversedOrthographicDepth")){
        const auto numerator=invoke(value,"add")({far});const auto divide=invoke(numerator,"div");return divide({invoke(far,"sub")({near})});
    }
    if(name.strictEquals("viewZToPerspectiveDepth")){
        const auto numerator=invoke(invoke(near,"add")({value}),"mul")({far});const auto divide=invoke(numerator,"div");
        return divide({invoke(invoke(far,"sub")({near}),"mul")({value})});
    }
    if(name.strictEquals("viewZToReversedPerspectiveDepth")){
        const auto multiply=invoke(near,"mul");const auto numerator=multiply({invoke(value,"add")({far})});const auto divide=invoke(numerator,"div"),denominator=invoke(value,"mul");
        return divide({denominator({invoke(near,"sub")({far})})});
    }
    if(name.strictEquals("viewZToLogarithmicDepth")){
        near=invoke(invoke(near,"max")({1e-6}),"toVar")();
        const auto numerator=table.get("log2").call({}, {invoke(invoke(value,"negate")(),"div")({near})});
        const auto denominator=table.get("log2").call({}, {invoke(far,"div")({near})});return invoke(numerator,"div")({denominator});
    }
    if(name.strictEquals("logarithmicDepthToViewZ")){
        const auto multiply=invoke(value,"mul");const auto exponent=multiply({table.get("log").call({}, {invoke(far,"div")({near})})});
        return invoke(invoke(invoke(table.get("float").call({}, {std::numbers::e}),"pow")({exponent}),"mul")({near}),"negate")();
    }
    throw std::logic_error("Unknown viewport depth conversion");
}
JSValue conversionBody(const JSValue&,const Args& args,const Args& captures){
    const auto name=captures[0];auto iterator=getJSIterator(arg(args,0));Args inputs;
    for(unsigned i=0;i<3;++i){const auto next=iterator.done?std::optional<JSValue>{}:stepJSIterator(iterator);inputs.push_back(next?*next:JSValue{});}if(!iterator.done)closeJSIterator(iterator);
    const auto depth=inputs[0],near=inputs[1],far=inputs[2],builder=arg(args,1);const bool reversed=builder.get("renderer").get("reversedDepthBuffer").strictEquals(true);
    if(name.strictEquals("orthographicDepthToViewZ"))return reversed?invoke(invoke(invoke(far,"sub")({near}),"mul")({depth}),"sub")({far}):invoke(invoke(invoke(near,"sub")({far}),"mul")({depth}),"sub")({near});
    const auto numerator=invoke(near,"mul")({far});const auto divide=invoke(numerator,"div");
    const auto difference=reversed?invoke(near,"sub")({far}):invoke(far,"sub")({near});
    return divide({invoke(invoke(difference,"mul")({depth}),"sub")({reversed?near:far})});
}
JSValue viewportDepthMethod(const JSValue& receiver,const Args& args,const Args& captures){
    auto self=receiver;const auto name=captures[0],prototype=captures[1],table=captures[2],ctor=table.get("constructor");
    if(name.strictEquals("isCacheable"))return false;
    if(name.strictEquals("generate")){
        const auto builder=arg(args,0),scope=self.get("scope");if(scope.strictEquals(ctor.get("DEPTH_BASE")))return invoke(builder,"getFragDepth")();
        return prototype.getPrototype().get("generate",self).call(self,{builder});
    }
    if(name.strictEquals("setup")){
        const auto camera=arg(args,0).get("camera"),scope=self.get("scope"),value=self.get("valueNode");auto node=JSValue(nullptr);
        if(scope.strictEquals(ctor.get("DEPTH_BASE"))){if(!value.isNull())node=invoke(table.get("depthBase").call(),"assign")({value});}
        else if(scope.strictEquals(ctor.get("DEPTH"))){
            if(camera.get("isPerspectiveCamera").toBoolean())node=table.get("viewZToPerspectiveDepth").call({}, {table.get("positionView").get("z"),table.get("cameraNear"),table.get("cameraFar")});
            else node=table.get("viewZToOrthographicDepth").call({}, {table.get("positionView").get("z"),table.get("cameraNear"),table.get("cameraFar")});
        }else if(scope.strictEquals(ctor.get("LINEAR_DEPTH"))){
            if(!value.isNull()){
                if(camera.get("isPerspectiveCamera").toBoolean()){
                    const auto viewZ=table.get("perspectiveDepthToViewZ").call({}, {value,table.get("cameraNear"),table.get("cameraFar")});
                    node=table.get("viewZToOrthographicDepth").call({}, {viewZ,table.get("cameraNear"),table.get("cameraFar")});
                }else node=value;
            }else node=table.get("viewZToOrthographicDepth").call({}, {table.get("positionView").get("z"),table.get("cameraNear"),table.get("cameraFar")});
        }
        return node;
    }
    throw std::logic_error("Unknown ViewportDepthNode method");
}
}
JSValue nativeViewportDepthNodeModule(){
    using namespace UtilityNodeRuntime;static thread_local JSWeakValue weak;if(const auto value=weak.lock();value&&value->isObject())return *value;
    static thread_local bool initializing=false;JSModuleInitializationScope scope(initializing,"ViewportDepthNode module");
    const auto node=nativeNodeConstructor(),tsl=nativeTSLBase(),camera=nativeCameraModule(),position=nativePositionModule(),depthTexture=nativeViewportDepthTextureNodeModule();
    auto table=JSValue::object(nullptr),bindings=JSValue::object(nullptr);
    for(const auto* name:{"float","Fn","log","log2","nodeImmutable","nodeProxy"})own(table,name,nativeTSLBaseBinding(name));
    own(table,"cameraNear",nativeCameraBinding("cameraNear"));own(table,"cameraFar",nativeCameraBinding("cameraFar"));own(table,"positionView",nativePositionBinding("positionView"));
    auto ctor=makeConstructor("ViewportDepthNode",1,+[](const JSValue& target,const Args& args,const Args& captures)->JSValue{
        auto self=superConstruct(target,captures,{"float"});self.set("scope",arg(args,0));self.set("valueNode",arg(args,1,nullptr));self.set("isViewportDepthNode",true);return self;
    },{{"isCacheable",0},{"generate",1},{"setup",1}},viewportDepthMethod,{table});
    ctor.setFunctionSourceText(ViewportDepthNodeSource::classSource);auto prototype=ctor.get("prototype");
    for(const auto* name:{"isCacheable","generate","setup"}){auto method=prototype.get(name);method.setFunctionSourceText(std::string_view(name)=="isCacheable"?ViewportDepthNodeSource::isCacheableSource:std::string_view(name)=="generate"?ViewportDepthNodeSource::generateSource:ViewportDepthNodeSource::setupSource);}
    auto typeGetter=ctor.getOwnPropertyDescriptor("type")->getter.value();typeGetter.setFunctionSourceText(ViewportDepthNodeSource::typeSource);
    own(table,"constructor",ctor);own(ctor,"DEPTH_BASE","depthBase");own(ctor,"DEPTH","depth");own(ctor,"LINEAR_DEPTH","linearDepth");own(bindings,"default",ctor);
    for(const auto* name:{"viewZToOrthographicDepth","viewZToReversedOrthographicDepth"}){auto fn=JSValue::function(conversion,{name,table},name,3);const std::string_view key(name);const auto source=key=="viewZToOrthographicDepth"?ViewportDepthNodeSource::viewZToOrthographicDepthSource:key=="viewZToReversedOrthographicDepth"?ViewportDepthNodeSource::viewZToReversedOrthographicDepthSource:key=="viewZToPerspectiveDepth"?ViewportDepthNodeSource::viewZToPerspectiveDepthSource:key=="viewZToReversedPerspectiveDepth"?ViewportDepthNodeSource::viewZToReversedPerspectiveDepthSource:key=="viewZToLogarithmicDepth"?ViewportDepthNodeSource::viewZToLogarithmicDepthSource:ViewportDepthNodeSource::logarithmicDepthToViewZSource;fn.setFunctionSourceText(source);own(bindings,name,fn);own(table,name,fn);}
    auto orthographicBody=JSValue::function(conversionBody,{"orthographicDepthToViewZ"},"",2);orthographicBody.setFunctionSourceText(ViewportDepthNodeSource::orthographicDepthToViewZCallbackSource);const auto orthographic=table.get("Fn").call({}, {orthographicBody});own(bindings,"orthographicDepthToViewZ",orthographic);own(table,"orthographicDepthToViewZ",orthographic);
    for(const auto* name:{"viewZToPerspectiveDepth","viewZToReversedPerspectiveDepth"}){auto fn=JSValue::function(conversion,{name,table},name,3);const std::string_view key(name);const auto source=key=="viewZToOrthographicDepth"?ViewportDepthNodeSource::viewZToOrthographicDepthSource:key=="viewZToReversedOrthographicDepth"?ViewportDepthNodeSource::viewZToReversedOrthographicDepthSource:key=="viewZToPerspectiveDepth"?ViewportDepthNodeSource::viewZToPerspectiveDepthSource:key=="viewZToReversedPerspectiveDepth"?ViewportDepthNodeSource::viewZToReversedPerspectiveDepthSource:key=="viewZToLogarithmicDepth"?ViewportDepthNodeSource::viewZToLogarithmicDepthSource:ViewportDepthNodeSource::logarithmicDepthToViewZSource;fn.setFunctionSourceText(source);own(bindings,name,fn);own(table,name,fn);}
    auto perspectiveBody=JSValue::function(conversionBody,{"perspectiveDepthToViewZ"},"",2);perspectiveBody.setFunctionSourceText(ViewportDepthNodeSource::perspectiveDepthToViewZCallbackSource);const auto perspective=table.get("Fn").call({}, {perspectiveBody});own(bindings,"perspectiveDepthToViewZ",perspective);own(table,"perspectiveDepthToViewZ",perspective);
    for(const auto* name:{"viewZToLogarithmicDepth","logarithmicDepthToViewZ"}){auto fn=JSValue::function(conversion,{name,table},name,3);const std::string_view key(name);const auto source=key=="viewZToOrthographicDepth"?ViewportDepthNodeSource::viewZToOrthographicDepthSource:key=="viewZToReversedOrthographicDepth"?ViewportDepthNodeSource::viewZToReversedOrthographicDepthSource:key=="viewZToPerspectiveDepth"?ViewportDepthNodeSource::viewZToPerspectiveDepthSource:key=="viewZToReversedPerspectiveDepth"?ViewportDepthNodeSource::viewZToReversedPerspectiveDepthSource:key=="viewZToLogarithmicDepth"?ViewportDepthNodeSource::viewZToLogarithmicDepthSource:ViewportDepthNodeSource::logarithmicDepthToViewZSource;fn.setFunctionSourceText(source);own(bindings,name,fn);own(table,name,fn);}
    own(table,"depthBase",table.get("nodeProxy").call({}, {ctor,ctor.get("DEPTH_BASE")}));
    own(bindings,"depth",table.get("nodeImmutable").call({}, {ctor,ctor.get("DEPTH")}));
    own(bindings,"linearDepth",invoke(table.get("nodeProxy").call({}, {ctor,ctor.get("LINEAR_DEPTH")}),"setParameterLength")({0,1}));
    own(bindings,"viewportLinearDepth",bindings.get("linearDepth").call({}, {nativeViewportDepthTextureNodeBinding("viewportDepthTexture").call()}));
    auto depth=bindings.get("depth"),assign=JSValue::function(+[](const JSValue&,const Args& args,const Args& captures)->JSValue{return captures[0].get("depthBase").call({}, {arg(args,0)});},{table},"",1);assign.setFunctionSourceText(ViewportDepthNodeSource::depthAssignSource);depth.set("assign",assign);
    auto module=JSValue::object();auto names=bindings.ownPropertyNames();std::sort(names.begin(),names.end());for(const auto& name:names)own(module,name.c_str(),bindings.get(name));
    bindings.retainForHeapLifetime();viewportDepthLexical()=JSWeakValue(bindings);module.retainForHeapLifetime();weak=JSWeakValue(module);return module;
}
JSValue nativeViewportDepthNodeBinding(const std::string& name){const auto keeper=nativeViewportDepthNodeModule();return viewportDepthLexical().lock()->get(name);}
JSValue nativeViewportDepthNodeConstructor(){return nativeViewportDepthNodeBinding("default");}
ViewportDepthNode::ViewportDepthNode(const JSValue& scope,const JSValue& value):object_(constructor().construct({scope,value})){}
