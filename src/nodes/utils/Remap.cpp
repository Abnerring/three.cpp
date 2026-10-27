#include "nodes/utils/Remap.h"
#include "nodes/utils/UtilityNodeRuntime.h"
#include "nodes/tsl/TSLCore.h"

double Remap::value(double node, double inLow, double inHigh, double outLow, double outHigh) {
    // Normalise to nought and one, then stretch and shift.
    const double t = (node - inLow) / (inHigh - inLow);
    return t * (outHigh - outLow) + outLow;
}

double Remap::clamped(double node, double inLow, double inHigh, double outLow, double outHigh) {
    double t = (node - inLow) / (inHigh - inLow);
    // The clamp is applied to the normalised value, before the stretch.
    if (t < 0.0) t = 0.0;
    if (t > 1.0) t = 1.0;
    return t * (outHigh - outLow) + outLow;
}

bool Remap::runsClampStep(bool definedResult) {
    return definedResult;
}

namespace { JSWeakValue& remapLexical(){static thread_local JSWeakValue weak;return weak;} }
JSValue nativeRemapModule(){
    using namespace UtilityNodeRuntime;static thread_local JSWeakValue weak;if(const auto value=weak.lock();value&&value->isObject())return *value;
    static thread_local bool initializing=false;JSModuleInitializationScope scope(initializing,"Remap exports");
    const auto tsl=nativeTSLCore(),floatType=nativeTSLCoreBinding("float"),boolType=nativeTSLCoreBinding("bool"),defined=nativeTSLCoreBinding("defined");auto bindings=JSValue::object();
    const auto body=JSValue::function(+[](const JSValue&,const Args& args,const Args& c)->JSValue{
        auto iterator=getJSIterator(arg(args,0));JSValue node,low,high,outLow,outHigh,clamp;
        const auto next=[&]()->JSValue{if(iterator.done)return {};const auto value=stepJSIterator(iterator);return value?*value:JSValue{};};
        try{
            node=next();low=next();high=next();outLow=next();if(outLow.isUndefined())outLow=c[0].call({}, {0});
            outHigh=next();if(outHigh.isUndefined())outHigh=c[0].call({}, {1});clamp=next();if(clamp.isUndefined())clamp=c[1].call({}, {false});
        }catch(...){if(!iterator.done)closeJSIterator(iterator,std::current_exception());throw;}
        if(!iterator.done)closeJSIterator(iterator);
        const auto difference=invoke(node,"sub")({low});const auto divide=invoke(difference,"div");const auto range=invoke(high,"sub")({low});auto t=divide({range});
        if(c[2].call({}, {clamp}).toBoolean())t=invoke(t,"clamp")();
        const auto multiply=invoke(t,"mul");const auto outputRange=invoke(outHigh,"sub")({outLow});const auto product=multiply({outputRange});return invoke(product,"add")({outLow});
    },{floatType,boolType,defined},"",1);
    const auto remap=nativeTSLCoreBinding("Fn").call({}, {body});own(bindings,"remap",remap);
    own(bindings,"remapClamp",JSValue::constructor(+[](const JSValue&,const Args& args,const Args& c)->JSValue{
        const auto outLow=arg(args,3).isUndefined()?c[1].call({}, {0}):arg(args,3);const auto outHigh=arg(args,4).isUndefined()?c[1].call({}, {1}):arg(args,4);
        return c[0].call({}, {arg(args,0),arg(args,1),arg(args,2),outLow,outHigh,true});
    },{remap,floatType},"remapClamp",3,JSValue::ConstructorKind::Callable));
    const auto chaining=nativeTSLCoreBinding("addMethodChaining");for(const auto* name:{"remap","remapClamp"})chaining.call({}, {name,bindings.get(name)});
    auto module=JSValue::object();for(const auto& key:bindings.ownPropertyNames())own(module,key.c_str(),bindings.get(key));bindings.retainForHeapLifetime();remapLexical()=JSWeakValue(bindings);module.retainForHeapLifetime();weak=JSWeakValue(module);return module;
}
JSValue nativeRemapBinding(const std::string& name){const auto keeper=nativeRemapModule();return remapLexical().lock()->get(name);}
