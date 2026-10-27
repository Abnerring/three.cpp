#include "nodes/functions/material/getAlphaHashThreshold.h"
#include "nodes/tsl/TSLCore.h"
#include "nodes/tsl/TSLFunctionRuntime.h"
#include "nodes/math/MathNode.h"
#include "nodes/math/OperatorNode.h"
#include "nodes/math/ConditionalNode.h"
#include "nodes/core/AssignNode.h"
#include "nodes/core/VarNode.h"
#include "nodes/core/ContextNode.h"
#include "nodes/display/ColorSpaceFunctions.h"
#include "nodes/display/PremultiplyAlphaFunctions.h"

JSValue nativeAlphaHashThreshold() {
    using namespace TSLFunctionRuntime;
    static thread_local JSWeakValue weak;if(const auto value=weak.lock();value&&value->isCallable())return *value;
    static thread_local bool initializing=false;JSModuleInitializationScope scope(initializing,"getAlphaHashThreshold");
    // These are real, ported dependencies of the upstream TSLBase import.
    // The remaining TSLBase/NodeMaterial modules are still tracked separately.
    const auto tsl=nativeTSLCore(),ops=nativeOperatorNodeModule(),math=nativeMathNodeModule();
    nativeAssignNodeModule();nativeConditionalNodeModule();nativeContextNodeModule();nativeVarNodeModule();
    nativeColorSpaceFunctions();nativePremultiplyAlphaFunctions();
    const auto fn=nativeTSLCoreBinding("Fn");
    auto bindings=JSValue::object(nullptr);
    for(const auto* key:{"float","vec2","vec3"})bindings.set(key,nativeTSLCoreBinding(key));
    for(const auto* key:{"abs","ceil","clamp","dFdx","dFdy","exp2","floor","fract","length","log2","max","min","sin"})bindings.set(key,nativeMathNodeBinding(key));
    for(const auto* key:{"add","mul","sub"})bindings.set(key,nativeOperatorNodeBinding(key));
    const auto hash2D=shader(fn,+[](const JSValue&,const Args& args,const Args& captures)->JSValue {
        const auto value=first(arg(args,0)),b=captures[0];
        const auto fract=b.get("fract"),mul=b.get("mul"),sin=b.get("sin");
        const auto x=mul.call({}, {17.0,value.get("x")});const auto addX=invoke(x,"add");const auto y=mul.call({}, {0.1,value.get("y")});
        const auto sum=addX({y});const auto sine=sin.call({}, {sum});const auto scaled=mul.call({}, {1.0e4,sine});
        const auto times=invoke(scaled,"mul");const auto add=b.get("add"),abs=b.get("abs");
        const auto scaledY=mul.call({}, {13.0,value.get("y")});const auto shifted=invoke(scaledY,"add")({value.get("x")});
        const auto other=sin.call({}, {shifted});const auto magnitude=abs.call({}, {other});const auto factor=add.call({}, {0.1,magnitude});
        const auto product=times({factor});return fract.call({}, {product});
    },{bindings},"value","vec2","float");
    bindings.set("hash2D",hash2D);
    const auto hash3D=shader(fn,+[](const JSValue&,const Args& args,const Args& captures)->JSValue {
        const auto value=first(arg(args,0)),b=captures[0];const auto hash=b.get("hash2D"),vec2=b.get("vec2");
        const auto xy=hash.call({}, {value.get("xy")});const auto z=value.get("z");const auto point=vec2.call({}, {xy,z});return hash.call({}, {point});
    },{bindings},"value","vec3","float");
    bindings.set("hash3D",hash3D);
    auto result=shader(fn,+[](const JSValue&,const Args& args,const Args& captures)->JSValue {
        const auto position=first(arg(args,0)),b=captures[0];
        const auto max=b.get("max"),length=b.get("length"),dx=b.get("dFdx"),dy=b.get("dFdy");
        const auto derivativeX=dx.call({}, {position.get("xyz")});const auto lengthX=length.call({}, {derivativeX});
        const auto derivativeY=dy.call({}, {position.get("xyz")});const auto lengthY=length.call({}, {derivativeY});
        const auto maxDeriv=max.call({}, {lengthX,lengthY});
        const auto floatType=b.get("float");const auto one=floatType.call({}, {1});const auto divide=invoke(one,"div");
        const auto scale=floatType.call({}, {0.05});const auto scaled=invoke(scale,"mul")({maxDeriv});
        const auto quotient=divide({scaled});const auto pixScale=invoke(quotient,"toVar")({"pixScale"});
        const auto vec2=b.get("vec2"),exp2=b.get("exp2"),floor=b.get("floor"),log2=b.get("log2"),ceil=b.get("ceil");
        const auto logLow=log2.call({}, {pixScale});const auto low=floor.call({}, {logLow});const auto powerLow=exp2.call({}, {low});
        const auto logHigh=log2.call({}, {pixScale});const auto high=ceil.call({}, {logHigh});const auto powerHigh=exp2.call({}, {high});
        const auto pixScales=vec2.call({}, {powerLow,powerHigh});
        const auto hash3D=b.get("hash3D");
        const auto mulLow=invoke(pixScales.get("x"),"mul");const auto scaledLow=mulLow({position.get("xyz")});
        const auto lowFloor=floor.call({}, {scaledLow});const auto lowHash=hash3D.call({}, {lowFloor});
        const auto mulHigh=invoke(pixScales.get("y"),"mul");const auto scaledHigh=mulHigh({position.get("xyz")});
        const auto highFloor=floor.call({}, {scaledHigh});const auto highHash=hash3D.call({}, {highFloor});
        const auto alpha=vec2.call({}, {lowHash,highHash});
        const auto lerpLog=log2.call({}, {pixScale});const auto lerp=b.get("fract").call({}, {lerpLog});
        const auto add=b.get("add"),mul=b.get("mul"),sub=b.get("sub");
        const auto inverse=invoke(lerp,"oneMinus")();const auto left=mul.call({}, {inverse,alpha.get("x")});const auto right=mul.call({}, {lerp,alpha.get("y")});const auto x=add.call({}, {left,right});
        const auto min=b.get("min");const auto inverse2=invoke(lerp,"oneMinus")();const auto a=min.call({}, {lerp,inverse2});
        const auto vec3=b.get("vec3");
        const auto square=invoke(x,"mul")({x});const auto divSquare=invoke(square,"div");const auto twiceA=mul.call({}, {2.0,a});
        const auto times=invoke(twiceA,"mul");const auto oneMinusA=sub.call({}, {1.0,a});const auto denominator=times({oneMinusA});const auto case0=divSquare({denominator});
        const auto difference=invoke(x,"sub");const auto halfA=mul.call({}, {0.5,a});const auto diff=difference({halfA});const auto divideDiff=invoke(diff,"div");
        const auto divisor=sub.call({}, {1.0,a});const auto case1=divideDiff({divisor});
        const auto inverseX=sub.call({}, {1.0,x});const auto squareInverse=invoke(inverseX,"mul");const auto inverseX2=sub.call({}, {1.0,x});
        const auto invertedSquare=squareInverse({inverseX2});const auto divideInverse=invoke(invertedSquare,"div");const auto twiceA2=mul.call({}, {2.0,a});const auto times2=invoke(twiceA2,"mul");
        const auto inverseA2=sub.call({}, {1.0,a});const auto denominator2=times2({inverseA2});const auto inverseQuotient=divideInverse({denominator2});const auto case2=sub.call({}, {1.0,inverseQuotient});
        const auto cases=vec3.call({}, {case0,case1,case2});
        const auto less=invoke(x,"lessThan");const auto inverseA=invoke(a,"oneMinus")();const auto condition=less({inverseA});const auto select=invoke(condition,"select");
        const auto condition2=invoke(x,"lessThan")({a});const auto select2=invoke(condition2,"select");const auto cx=cases.get("x"),cy=cases.get("y");const auto inner=select2({cx,cy});
        const auto cz=cases.get("z");const auto threshold=select({inner,cz});return b.get("clamp").call({}, {threshold,1.0e-6,1.0});
    },{bindings},"position","vec3","float");
    result.retainForHeapLifetime();weak=JSWeakValue(result);return result;
}
