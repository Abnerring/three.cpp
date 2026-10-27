#include "nodes/accessors/ClippingNode.h"
#include "nodes/core/Node.h"
#include "nodes/tsl/TSLBase.h"
#include "nodes/accessors/Position.h"
#include "nodes/core/PropertyNode.h"
#include "nodes/utils/LoopNode.h"
#include "nodes/math/MathNode.h"
#include "nodes/accessors/UniformArrayNode.h"
#include "nodes/accessors/BuiltinNode.h"
#include "nodes/core/UniformGroupNode.h"
#include "nodes/utils/UtilityNodeRuntime.h"
#include <algorithm>
namespace {
using namespace UtilityNodeRuntime;
#include "nodes/accessors/ClippingNodeSource.inc"
JSWeakValue& lexical(){static thread_local JSWeakValue weak;return weak;}
JSValue nodeVar(const JSValue& imports,const char* type,const Args& args,const char* name){return invoke(imports.get(type).call({},args),"toVar")({name});}
JSValue uniform(const JSValue& imports,const JSValue& planes){return invoke(imports.get("uniformArray").call({}, {planes}),"setGroup")({imports.get("renderGroup")});}
JSValue planeBody(const JSValue&,const Args& a,const Args& c){
    const auto index=arg(a,0).get("i"),mode=c[0],imports=c[1],planes=c[2];const auto plane=invoke(planes,"element")({index});
    if(mode.strictEquals("defaultUnion")){invoke(invoke(invoke(imports.get("positionView"),"dot")({plane.get("xyz")}),"greaterThan")({plane.get("w")}),"discard")();return {};}
    if(mode.strictEquals("defaultIntersection")){
        const auto assign=invoke(c[3],"assign");const auto dot=invoke(imports.get("positionView"),"dot")({plane.get("xyz")});const auto greater=invoke(dot,"greaterThan")({plane.get("w")});assign({invoke(greater,"and")({c[3]})});return {};
    }
    if(mode.strictEquals("hardware")){
        const auto distance=invoke(invoke(invoke(imports.get("positionView"),"dot")({plane.get("xyz")}),"sub")({plane.get("w")}),"negate")();const auto element=invoke(c[3],"element")({index});invoke(element,"assign")({distance});return {};
    }
    const auto distance=c[3],gradient=c[4],opacity=c[5];
    const auto assignDistance=invoke(distance,"assign");const auto dot=invoke(imports.get("positionView"),"dot")({plane.get("xyz")});assignDistance({invoke(invoke(dot,"negate")(),"add")({plane.get("w")})});
    const auto assignGradient=invoke(gradient,"assign");assignGradient({invoke(invoke(distance,"fwidth")(),"div")({2.0})});
    const auto multiply=invoke(opacity,"mulAssign");const auto smooth=imports.get("smoothstep");const auto negative=invoke(gradient,"negate")();auto value=smooth.call({}, {negative,gradient,distance});if(mode.strictEquals("alphaIntersection"))value=invoke(value,"oneMinus")();multiply({value});return {};
}
JSValue callback(const JSValue& imports,const char* mode,const JSValue& planes,const Args& extra,const char* source){Args captures{mode,imports,planes};captures.insert(captures.end(),extra.begin(),extra.end());auto fn=JSValue::function(planeBody,captures,"",1);fn.setFunctionSourceText(source);return fn;}
JSValue body(const JSValue&,const Args&,const Args& c){
    const auto kind=c[0],self=c[1],intersection=c[2],planes=c[3],imports=c[4];
    if(kind.strictEquals("hardware")){
        const auto clippingPlanes=uniform(imports,planes);const auto builtin=imports.get("builtin");const auto distance=builtin.call({}, {invoke(c[5],"getClipDistance")()});
        imports.get("Loop").call({}, {c[6],callback(imports,"hardware",clippingPlanes,{distance},ClippingNodeSource::arrow7)});return {};
    }
    if(kind.strictEquals("alpha")){
        const auto distance=nodeVar(imports,"float",{},"distanceToPlane"),gradient=nodeVar(imports,"float",{},"distanceToGradient"),opacity=nodeVar(imports,"float",{1},"clipOpacity");
        const auto count=planes.get("length");if(self.get("hardwareClipping").strictEquals(false)&&greater(count,0)){const auto clippingPlanes=uniform(imports,planes);imports.get("Loop").call({}, {count,callback(imports,"alphaUnion",clippingPlanes,{distance,gradient,opacity},ClippingNodeSource::arrow1)});}
        const auto intersections=intersection.get("length");if(greater(intersections,0)){
            const auto clippingPlanes=uniform(imports,intersection),intersectionOpacity=nodeVar(imports,"float",{1},"intersectionClipOpacity");
            imports.get("Loop").call({}, {intersections,callback(imports,"alphaIntersection",clippingPlanes,{distance,gradient,intersectionOpacity},ClippingNodeSource::arrow2)});
            const auto multiply=invoke(opacity,"mulAssign");multiply({invoke(intersectionOpacity,"oneMinus")()});
        }
        invoke(imports.get("diffuseColor").get("a"),"mulAssign")({opacity});invoke(invoke(imports.get("diffuseColor").get("a"),"equal")({0.0}),"discard")();return {};
    }
    const auto count=planes.get("length");if(self.get("hardwareClipping").strictEquals(false)&&greater(count,0)){const auto clippingPlanes=uniform(imports,planes);imports.get("Loop").call({}, {count,callback(imports,"defaultUnion",clippingPlanes,{},ClippingNodeSource::arrow4)});}
    const auto intersections=intersection.get("length");if(greater(intersections,0)){
        const auto clippingPlanes=uniform(imports,intersection),clipped=nodeVar(imports,"bool",{true},"clipped");imports.get("Loop").call({}, {intersections,callback(imports,"defaultIntersection",clippingPlanes,{clipped},ClippingNodeSource::arrow5)});invoke(clipped,"discard")();
    }return {};
}
JSValue method(const JSValue& receiver,const Args& a,const Args& c){
    auto self=receiver;const auto key=c[0],imports=c[2],builder=arg(a,0);
    if(key.strictEquals("isCacheable"))return false;
    if(key.strictEquals("setup")){
        c[1].getPrototype().get("setup",self).call(self,{builder});const auto context=builder.get("clippingContext"),intersection=context.get("intersectionPlanes"),planes=context.get("unionPlanes");self.set("hardwareClipping",builder.get("hardwareClipping"));
        if(self.get("scope").strictEquals(c[3].get("ALPHA_TO_COVERAGE")))return invoke(self,"setupAlphaToCoverage")({intersection,planes});
        if(self.get("scope").strictEquals(c[3].get("HARDWARE")))return invoke(self,"setupHardwareClipping")({planes,builder});return invoke(self,"setupDefault")({intersection,planes});
    }
    const bool hardware=key.strictEquals("setupHardwareClipping");const auto intersection=hardware?JSValue(nullptr):builder,planes=hardware?builder:arg(a,1);JSValue count;
    if(hardware){count=planes.get("length");invoke(arg(a,1),"enableHardwareClipping")({count});}
    auto fn=JSValue::function(body,{hardware?"hardware":key.strictEquals("setupAlphaToCoverage")?"alpha":"default",self,intersection,planes,imports,hardware?arg(a,1):JSValue{},count},"",0);
    fn.setFunctionSourceText(hardware?ClippingNodeSource::arrow6:key.strictEquals("setupAlphaToCoverage")?ClippingNodeSource::arrow0:ClippingNodeSource::arrow3);return imports.get("Fn").call({}, {fn}).call();
}
}
JSValue nativeClippingNodeModule(){
    using namespace UtilityNodeRuntime;static thread_local JSWeakValue weak;if(const auto m=weak.lock();m&&m->isObject())return *m;static thread_local bool initializing=false;JSModuleInitializationScope scope(initializing,"ClippingNode module");
    const auto base=nativeNodeConstructor(),tsl=nativeTSLBase(),position=nativePositionModule(),property=nativePropertyNodeModule(),loop=nativeLoopNodeModule(),math=nativeMathNodeModule(),array=nativeUniformArrayNodeModule(),builtin=nativeBuiltinNodeModule();const auto render=nativeUniformGroupNodeBinding("renderGroup");
    auto imports=JSValue::object(nullptr);for(const auto* n:{"Fn","bool","float"})own(imports,n,nativeTSLBaseBinding(n));own(imports,"positionView",nativePositionBinding("positionView"));own(imports,"diffuseColor",nativePropertyNodeBinding("diffuseColor"));own(imports,"Loop",nativeLoopNodeBinding("Loop"));own(imports,"smoothstep",nativeMathNodeBinding("smoothstep"));own(imports,"uniformArray",nativeUniformArrayNodeBinding("uniformArray"));own(imports,"builtin",nativeBuiltinNodeBinding("builtin"));own(imports,"renderGroup",render);auto holder=JSValue::object(nullptr);
    auto ctor=JSValue::builtinConstructor(+[](const JSValue& target,const Args& a,const Args& c)->JSValue{auto value=arg(a,0);if(value.isUndefined())value=c[0].get("constructor").get("DEFAULT");auto self=superConstruct(target,c);self.set("scope",value);return self;},{holder},"ClippingNode",0);holder.set("constructor",ctor);ctor.setPrototype(base);ctor.setFunctionSourceText(ClippingNodeSource::classSource);auto p=ctor.get("prototype");p.setPrototype(base.get("prototype"));auto getter=JSValue::function(+[](const JSValue&,const Args&,const Args&)->JSValue{return "ClippingNode";},{},"get type",0);getter.setFunctionSourceText(ClippingNodeSource::typeSource);JSPropertyDescriptor type;type.getter=getter;type.configurable=true;ctor.defineProperty("type",type);
    for(const auto& [name,arity]:std::initializer_list<std::pair<const char*,unsigned>>{{"isCacheable",0},{"setup",1},{"setupAlphaToCoverage",2},{"setupDefault",2},{"setupHardwareClipping",2}}){auto fn=JSValue::function(method,{name,p,imports,ctor},name,arity);const std::string k=name;fn.setFunctionSourceText(k=="isCacheable"?ClippingNodeSource::isCacheableSource:k=="setup"?ClippingNodeSource::setupSource:k=="setupAlphaToCoverage"?ClippingNodeSource::setupAlphaToCoverageSource:k=="setupDefault"?ClippingNodeSource::setupDefaultSource:ClippingNodeSource::setupHardwareClippingSource);JSPropertyDescriptor d;d.value=fn;d.writable=d.configurable=true;p.defineProperty(name,d);}
    own(ctor,"ALPHA_TO_COVERAGE","alphaToCoverage");own(ctor,"DEFAULT","default");own(ctor,"HARDWARE","hardware");auto bindings=JSValue::object(nullptr);own(bindings,"default",ctor);
    for(const auto* name:{"clipping","clippingAlpha","hardwareClipping"}){auto fn=JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue{if(c[0].strictEquals("clipping"))return c[1].construct();return c[1].construct({c[1].get(c[0].strictEquals("clippingAlpha")?"ALPHA_TO_COVERAGE":"HARDWARE")});},{name,ctor},name,0);fn.setFunctionSourceText(std::string(name)=="clipping"?ClippingNodeSource::arrow8:std::string(name)=="clippingAlpha"?ClippingNodeSource::arrow9:ClippingNodeSource::arrow10);own(bindings,name,fn);}
    auto module=JSValue::object();auto names=bindings.ownPropertyNames();std::sort(names.begin(),names.end());for(const auto& name:names)own(module,name.c_str(),bindings.get(name));bindings.retainForHeapLifetime();lexical()=JSWeakValue(bindings);module.retainForHeapLifetime();weak=JSWeakValue(module);return module;
}
JSValue nativeClippingNodeBinding(const std::string& name){const auto keeper=nativeClippingNodeModule();return lexical().lock()->get(name);}
JSValue nativeClippingNodeConstructor(){return nativeClippingNodeBinding("default");}
