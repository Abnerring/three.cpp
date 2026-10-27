#include "nodes/display/ColorSpaceFunctions.h"

#include <cmath>
#include "nodes/tsl/TSLCore.h"
#include "nodes/tsl/TSLFunctionRuntime.h"
#include "nodes/math/MathNode.h"

double ColorSpaceFunctions::mix(double powerPiece, double linePiece, double factor) {
    // The usual mix: the second piece as the flag rises and the first as it falls.
    return powerPiece * (1.0 - factor) + linePiece * factor;
}

double ColorSpaceFunctions::eotfFactor(double value) {
    // One when the colour is small enough for the straight line.
    return value <= kEotfThreshold ? 1.0 : 0.0;
}

double ColorSpaceFunctions::oetfFactor(double value) {
    return value <= kOetfThreshold ? 1.0 : 0.0;
}

double ColorSpaceFunctions::eotf(double value) {
    // The power piece first, the straight line second, so a small colour takes the line.
    const double powerPiece = std::pow(value * kEotfScale + kEotfOffset, kEotfExponent);
    const double linePiece = value * kEotfSlope;
    return mix(powerPiece, linePiece, eotfFactor(value));
}

double ColorSpaceFunctions::oetf(double value) {
    const double powerPiece = std::pow(value, kOetfExponent) * kOetfScale - kOetfOffset;
    const double linePiece = value * kOetfSlope;
    return mix(powerPiece, linePiece, oetfFactor(value));
}

bool ColorSpaceFunctions::exponentsAreReciprocal() {
    // The reciprocal of two point four is nought point four one six six six and so on, while
    // the constant holds five decimal places, so the two differ in the sixth.
    return kOetfExponent == 1.0 / kEotfExponent;
}

JSValue nativeColorSpaceFunctions() {
    using namespace TSLFunctionRuntime;
    static thread_local JSWeakValue weak;if(const auto value=weak.lock();value&&value->isObject())return *value;
    static thread_local bool initializing=false;JSModuleInitializationScope scope(initializing,"ColorSpaceFunctions");
    const auto math=nativeMathNodeModule(),tsl=nativeTSLCore();const auto fn=nativeTSLCoreBinding("Fn"),mix=nativeMathNodeBinding("mix");
    auto exports=JSValue::object();
    own(exports,"sRGBTransferEOTF",shader(fn,+[](const JSValue&,const Args& args,const Args& captures)->JSValue {
        const auto color=first(arg(args,0));
        const auto scaled=invoke(color,"mul")({0.9478672986});const auto offset=invoke(scaled,"add")({0.0521327014});const auto a=invoke(offset,"pow")({2.4});
        const auto b=invoke(color,"mul")({0.0773993808});const auto factor=invoke(color,"lessThanEqual")({0.04045});
        return captures[0].call({}, {a,b,factor});
    },{mix},"color","vec3","vec3"));
    own(exports,"sRGBTransferOETF",shader(fn,+[](const JSValue&,const Args& args,const Args& captures)->JSValue {
        const auto color=first(arg(args,0));
        const auto power=invoke(color,"pow")({0.41666});const auto scaled=invoke(power,"mul")({1.055});const auto a=invoke(scaled,"sub")({0.055});
        const auto b=invoke(color,"mul")({12.92});const auto factor=invoke(color,"lessThanEqual")({0.0031308});
        return captures[0].call({}, {a,b,factor});
    },{mix},"color","vec3","vec3"));
    exports.retainForHeapLifetime();weak=JSWeakValue(exports);return exports;
}
