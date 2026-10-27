// Ported from the pinned three.js ToneMappingFunctions.js; see THREEJS-LICENSE.
#include "nodes/display/ToneMappingFunctions.h"
#include "nodes/tsl/TSLBase.h"
#include "nodes/tsl/TSLFunctionRuntime.h"
#include "nodes/math/MathNode.h"
#include "nodes/math/OperatorNode.h"
#include "nodes/math/ConditionalNode.h"

namespace {
using namespace TSLFunctionRuntime;
JSWeakValue& lexical(){static thread_local JSWeakValue weak;return weak;}
Args unpack(const JSValue& input,unsigned count){
    auto iterator=getJSIterator(input);Args result;
    for(unsigned i=0;i<count;++i){const auto value=iterator.done?std::optional<JSValue>{}:stepJSIterator(iterator);result.push_back(value?*value:JSValue{});}
    if(!iterator.done)closeJSIterator(iterator);return result;
}
JSValue matrix(const JSValue& b,const Args& a,const Args& c,const Args& d){
    const auto x=b.get("vec3").call({},a),y=b.get("vec3").call({},c),z=b.get("vec3").call({},d);return b.get("mat3").call({}, {x,y,z});
}
JSValue body(const JSValue&,const Args& args,const Args& captures){
    const auto name=captures[0],b=captures[1];const auto inputs=unpack(arg(args,0),name.strictEquals("RRTAndODTFit")||name.strictEquals("agxDefaultContrastApprox")?1:2);
    auto color=inputs[0];const auto exposure=inputs.size()>1?inputs[1]:JSValue{};
    if(name.strictEquals("linearToneMapping"))return invoke(invoke(color,"mul")({exposure}),"clamp")();
    if(name.strictEquals("reinhardToneMapping")){
        color=invoke(color,"mul")({exposure});const auto divide=invoke(color,"div");const auto denominator=invoke(color,"add")({1});return invoke(divide({denominator}),"clamp")();
    }
    if(name.strictEquals("cineonToneMapping")){
        color=invoke(color,"mul")({exposure});color=invoke(invoke(color,"sub")({.004}),"max")({0});
        const auto aMul=invoke(color,"mul");const auto aInner=invoke(color,"mul")({6.2});const auto a=aMul({invoke(aInner,"add")({.5})});
        const auto bMul=invoke(color,"mul");const auto bInner=invoke(color,"mul")({6.2});const auto denominator=invoke(bMul({invoke(bInner,"add")({1.7})}),"add")({.06});
        return invoke(invoke(a,"div")({denominator}),"pow")({2.2});
    }
    if(name.strictEquals("RRTAndODTFit")){
        const auto aMul=invoke(color,"mul");const auto aAdd=invoke(color,"add")({.0245786});const auto a=invoke(aMul({aAdd}),"sub")({.000090537});
        const auto bMul=invoke(color,"mul");const auto bAdd=invoke(color,"add")({.4329510});const auto inner=invoke(bAdd,"mul")({.983729});const auto denominator=invoke(bMul({inner}),"add")({.238081});return invoke(a,"div")({denominator});
    }
    if(name.strictEquals("acesFilmicToneMapping")){
        const auto input=b.get("mat3").call({}, {.59719,.35458,.04823,.07600,.90834,.01566,.02840,.13383,.83777});
        const auto output=b.get("mat3").call({}, {1.60475,-.53108,-.07367,-.10208,1.10813,-.00605,-.00327,-.07276,1.07602});
        color=invoke(invoke(color,"mul")({exposure}),"div")({.6});color=invoke(input,"mul")({color});color=b.get("RRTAndODTFit").call({}, {color});color=invoke(output,"mul")({color});return invoke(color,"clamp")();
    }
    if(name.strictEquals("agxDefaultContrastApprox")){
        const auto x=invoke(b.get("vec3").call({}, {color}),"toVar")();
        const auto x2=invoke(b.get("vec3").call({}, {invoke(x,"mul")({x})}),"toVar")();
        const auto x4=invoke(b.get("vec3").call({}, {invoke(x2,"mul")({x2})}),"toVar")();
        const auto firstMul=invoke(b.get("float").call({}, {15.5}),"mul");const auto square=invoke(x4,"mul")({x2});const auto first=firstMul({square});
        const auto subtract=invoke(first,"sub");const auto x5=invoke(x4,"mul")({x});const auto second=b.get("mul").call({}, {40.14,x5});
        const auto add=invoke(subtract({second}),"add");
        const auto third=b.get("mul").call({}, {31.96,x4});const auto subThird=invoke(third,"sub");const auto cube=invoke(x2,"mul")({x});const auto fourth=b.get("mul").call({}, {6.868,cube});const auto addThird=invoke(subThird({fourth}),"add");
        const auto fifth=b.get("mul").call({}, {.4298,x2});const auto addFifth=invoke(fifth,"add");const auto sixth=b.get("mul").call({}, {.1191,x});const auto final=invoke(sixth,"sub")({.00232});
        return add({addThird({addFifth({final})})});
    }
    if(name.strictEquals("agxToneMapping")){
        const auto tone=invoke(b.get("vec3").call({}, {color}),"toVar")();
        const auto inset=matrix(b,{.856627153315983,.137318972929847,.11189821299995},{.0951212405381588,.761241990602591,.0767994186031903},{.0482516061458583,.101439036467562,.811302368396859});
        const auto outset=matrix(b,{1.1271005818144368,-.1413297634984383,-.14132976349843826},{-.11060664309660323,1.157823702216272,-.11060664309660294},{-.016493938717834573,-.016493938717834257,1.2519364065950405});
        const auto minEv=b.get("float").call({}, {-12.47393}),maxEv=b.get("float").call({}, {4.026069});invoke(tone,"mulAssign")({exposure});
        {const auto assign=invoke(tone,"assign");assign({invoke(b.get("LINEAR_SRGB_TO_LINEAR_REC2020"),"mul")({tone})});}
        {const auto assign=invoke(tone,"assign");assign({invoke(inset,"mul")({tone})});}
        {const auto assign=invoke(tone,"assign");assign({b.get("max").call({}, {tone,1e-10})});}
        {const auto assign=invoke(tone,"assign");assign({b.get("log2").call({}, {tone})});}
        {const auto assign=invoke(tone,"assign");const auto diff=invoke(tone,"sub")({minEv});const auto divide=invoke(diff,"div");const auto span=invoke(maxEv,"sub")({minEv});assign({divide({span})});}
        {const auto assign=invoke(tone,"assign");assign({b.get("clamp").call({}, {tone,0,1})});}
        {const auto assign=invoke(tone,"assign");assign({b.get("agxDefaultContrastApprox").call({}, {tone})});}
        {const auto assign=invoke(tone,"assign");assign({invoke(outset,"mul")({tone})});}
        {const auto assign=invoke(tone,"assign");const auto zero=b.get("vec3").call({}, {0});const auto positive=b.get("max").call({}, {zero,tone});const auto power=b.get("vec3").call({}, {2.2});assign({b.get("pow").call({}, {positive,power})});}
        {const auto assign=invoke(tone,"assign");assign({invoke(b.get("LINEAR_REC2020_TO_LINEAR_SRGB"),"mul")({tone})});}
        {const auto assign=invoke(tone,"assign");assign({b.get("clamp").call({}, {tone,0,1})});}return tone;
    }
    if(name.strictEquals("neutralToneMapping")){
        const auto start=b.get("float").call({}, {.8-.04}),desaturation=b.get("float").call({}, {.15});color=invoke(color,"mul")({exposure});
        const auto red=color.get("r"),green=color.get("g"),blue=color.get("b");const auto inner=b.get("min").call({}, {green,blue});const auto x=b.get("min").call({}, {red,inner});
        const auto condition=invoke(x,"lessThan")({.08});const auto subtract=invoke(x,"sub");const auto square=invoke(x,"mul")({x});const auto weighted=b.get("mul").call({}, {6.25,square});const auto offset=b.get("select").call({}, {condition,subtract({weighted}),.04});invoke(color,"subAssign")({offset});
        const auto r=color.get("r"),g=color.get("g"),bb=color.get("b");const auto innerPeak=b.get("max").call({}, {g,bb});const auto peak=b.get("max").call({}, {r,innerPeak});
        const auto test=invoke(peak,"lessThan")({start});const auto branch=JSValue::function(+[](const JSValue&,const Args&,const Args& captures)->JSValue{return captures[0];},{color},"",0);b.get("If").call({}, {test,branch});
        const auto d=b.get("sub").call({}, {1,start});const auto product=invoke(d,"mul")({d});const auto divide=invoke(product,"div");const auto addPeak=invoke(peak,"add");const auto difference=invoke(d,"sub")({start});const auto denominator=addPeak({difference});const auto newPeak=b.get("sub").call({}, {1,divide({denominator})});
        const auto scale=invoke(color,"mulAssign");scale({invoke(newPeak,"div")({peak})});const auto mulDesaturation=invoke(desaturation,"mul");const auto range=invoke(peak,"sub")({newPeak});const auto ratio=invoke(mulDesaturation({range}),"add")({1});const auto inverse=b.get("div").call({}, {1,ratio});const auto factor=b.get("sub").call({}, {1,inverse});
        const auto target=b.get("vec3").call({}, {newPeak});return b.get("mix").call({}, {color,target,factor});
    }
    throw std::logic_error("Unknown ToneMappingFunctions body");
}
JSValue makeShader(const JSValue& b,const char* name,bool single=false){
    auto layout=JSValue::object();own(layout,std::string_view(name)=="agxDefaultContrastApprox"?"x":"color","vec3");if(!single)own(layout,"exposure","float");own(layout,"return","vec3");return b.get("Fn").call({}, {JSValue::function(body,{name,b},"",1),layout});
}
}
JSValue nativeToneMappingFunctions(){
    using namespace TSLFunctionRuntime;
    static thread_local JSWeakValue weak;if(const auto value=weak.lock();value&&value->isObject())return *value;
    static thread_local bool initializing=false;JSModuleInitializationScope scope(initializing,"ToneMappingFunctions");
    const auto tsl=nativeTSLBase(),conditional=nativeConditionalNodeModule(),math=nativeMathNodeModule(),ops=nativeOperatorNodeModule();auto b=JSValue::object(nullptr);
    for(const auto* key:{"Fn","float","mat3","vec3","If"})own(b,key,nativeTSLBaseBinding(key));own(b,"select",nativeConditionalNodeBinding("select"));
    for(const auto* key:{"clamp","log2","max","min","pow","mix"})own(b,key,nativeMathNodeBinding(key));for(const auto* key:{"mul","sub","div"})own(b,key,nativeOperatorNodeBinding(key));
    for(const auto* name:{"linearToneMapping","reinhardToneMapping","cineonToneMapping"})own(b,name,makeShader(b,name));own(b,"RRTAndODTFit",makeShader(b,"RRTAndODTFit",true));own(b,"acesFilmicToneMapping",makeShader(b,"acesFilmicToneMapping"));
    own(b,"LINEAR_REC2020_TO_LINEAR_SRGB",matrix(b,{1.6605,-.1246,-.0182},{-.5876,1.1329,-.1006},{-.0728,-.0083,1.1187}));
    own(b,"LINEAR_SRGB_TO_LINEAR_REC2020",matrix(b,{.6274,.0691,.0164},{.3293,.9195,.0880},{.0433,.0113,.8956}));
    own(b,"agxDefaultContrastApprox",makeShader(b,"agxDefaultContrastApprox",true));own(b,"agxToneMapping",makeShader(b,"agxToneMapping"));own(b,"neutralToneMapping",makeShader(b,"neutralToneMapping"));
    auto bindings=JSValue::object(nullptr),module=JSValue::object();for(const auto* name:{"acesFilmicToneMapping","agxToneMapping","cineonToneMapping","linearToneMapping","neutralToneMapping","reinhardToneMapping"}){own(bindings,name,b.get(name));own(module,name,b.get(name));}
    bindings.retainForHeapLifetime();lexical()=JSWeakValue(bindings);module.retainForHeapLifetime();weak=JSWeakValue(module);return module;
}
JSValue nativeToneMappingFunctionsBinding(const std::string& name){const auto keeper=nativeToneMappingFunctions();return lexical().lock()->get(name);}
