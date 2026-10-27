#include "nodes/display/PremultiplyAlphaFunctions.h"
#include "nodes/tsl/TSLCore.h"
#include "nodes/tsl/TSLFunctionRuntime.h"

PremultiplyAlpha::Colour PremultiplyAlpha::premultiply(const Colour& colour) {
    // Multiplication by nought is nought, so no case is needed.
    Colour out;
    out.r = colour.r * colour.a;
    out.g = colour.g * colour.a;
    out.b = colour.b * colour.a;
    out.a = colour.a;
    return out;
}

bool PremultiplyAlpha::needsGuard(double alpha) {
    return alpha == 0.0;
}

PremultiplyAlpha::Colour PremultiplyAlpha::unpremultiply(const Colour& colour) {
    // The guard answers with four noughts, not with the original colour and not with its alpha
    // kept beside nought parts.
    if (needsGuard(colour.a)) {
        Colour out;
        return out;
    }
    Colour out;
    out.r = colour.r / colour.a;
    out.g = colour.g / colour.a;
    out.b = colour.b / colour.a;
    out.a = colour.a;
    return out;
}

namespace {JSWeakValue& premultiplyBindings(){static thread_local JSWeakValue weak;return weak;}}
JSValue nativePremultiplyAlphaFunctions() {
    using namespace TSLFunctionRuntime;
    static thread_local JSWeakValue weak;if(const auto value=weak.lock();value&&value->isObject())return *value;
    static thread_local bool initializing=false;JSModuleInitializationScope scope(initializing,"PremultiplyAlphaFunctions");
    const auto tsl=nativeTSLCore();const auto fn=nativeTSLCoreBinding("Fn"),vec4=nativeTSLCoreBinding("vec4");auto exports=JSValue::object();
    own(exports,"premultiplyAlpha",shader(fn,+[](const JSValue&,const Args& args,const Args& captures)->JSValue {
        const auto color=first(arg(args,0));const auto multiply=invoke(color.get("rgb"),"mul");const auto rgb=multiply({color.get("a")});
        const auto alpha=color.get("a");return captures[0].call({}, {rgb,alpha});
    },{vec4},"color","vec4","vec4"));
    own(exports,"unpremultiplyAlpha",shader(fn,+[](const JSValue&,const Args& args,const Args& captures)->JSValue {
        const auto color=first(arg(args,0));const auto test=invoke(color.get("a"),"equal")({0});const auto select=invoke(test,"select");
        const auto zero=captures[0].call({}, {0});const auto divide=invoke(color.get("rgb"),"div");const auto rgb=divide({color.get("a")});const auto alpha=color.get("a");
        const auto value=captures[0].call({}, {rgb,alpha});return select({zero,value});
    },{vec4},"color","vec4","vec4"));
    auto bindings=JSValue::object(nullptr);for(const auto& name:exports.ownPropertyNames())own(bindings,name.c_str(),exports.get(name));
    bindings.retainForHeapLifetime();premultiplyBindings()=JSWeakValue(bindings);
    exports.retainForHeapLifetime();weak=JSWeakValue(exports);return exports;
}
JSValue nativePremultiplyAlphaFunctionsBinding(const std::string& name){const auto keeper=nativePremultiplyAlphaFunctions();return premultiplyBindings().lock()->get(name);}
