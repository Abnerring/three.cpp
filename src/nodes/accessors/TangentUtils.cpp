#include "nodes/accessors/TangentUtils.h"

#include <cmath>

double TangentUtils::determinant(double tDotT, double bDotB) {
    // The larger of the two squared lengths, not their product.
    return tDotT > bDotB ? tDotT : bDotB;
}

bool TangentUtils::degenerate(double determinantValue) {
    // Only exactly nought is degenerate; a tiny but non zero determinant still gets a scale.
    return determinantValue == 0.0;
}

double TangentUtils::scale(double tDotT, double bDotB) {
    const double det = determinant(tDotT, bDotB);
    // A determinant of nought gives nought rather than the inverse square root of nought, which
    // would be an infinity.
    if (degenerate(det)) return 0.0;
    return 1.0 / std::sqrt(det);
}

#include "nodes/accessors/UV.h"
#include "nodes/accessors/Position.h"
#include "nodes/accessors/Normal.h"
#include "nodes/tsl/TSLFunctionRuntime.h"
namespace { JSWeakValue& lexical(){static thread_local JSWeakValue weak;return weak;} }
JSValue nativeTangentUtilsModule(){
 using namespace TSLFunctionRuntime;
 static thread_local JSWeakValue weak;if(const auto v=weak.lock();v&&v->isObject())return *v;
 static thread_local bool initializing=false;JSModuleInitializationScope scope(initializing,"TangentUtils module");
 const auto uvModule=nativeUVModule(),position=nativePositionModule(),normal=nativeNormalModule();
 const auto uv=nativeUVBinding("uv").call(),positionView=nativePositionBinding("positionView"),N=nativeNormalBinding("normalView");
 const auto q0=invoke(positionView,"dFdx")(),q1=invoke(positionView,"dFdy")(),st0=invoke(uv,"dFdx")(),st1=invoke(uv,"dFdy")();
 const auto q1perp=invoke(q1,"cross")({N}),q0perp=invoke(N,"cross")({q0});
 const auto T=invoke(invoke(q1perp,"mul")({st0.get("x")}),"add")({invoke(q0perp,"mul")({st1.get("x")})});
 const auto B=invoke(invoke(q1perp,"mul")({st0.get("y")}),"add")({invoke(q0perp,"mul")({st1.get("y")})});
 const auto det=invoke(invoke(T,"dot")({T}),"max")({invoke(B,"dot")({B})});
 const auto scale=invoke(invoke(det,"equal")({0}),"select")({0,invoke(det,"inverseSqrt")()});
 auto bindings=JSValue::object(nullptr);own(bindings,"tangentViewFrame",invoke(invoke(T,"mul")({scale}),"toVar")({"tangentViewFrame"}));
 own(bindings,"bitangentViewFrame",invoke(invoke(B,"mul")({scale}),"toVar")({"bitangentViewFrame"}));
 auto module=JSValue::object();own(module,"bitangentViewFrame",bindings.get("bitangentViewFrame"));own(module,"tangentViewFrame",bindings.get("tangentViewFrame"));
 bindings.retainForHeapLifetime();lexical()=JSWeakValue(bindings);module.retainForHeapLifetime();weak=JSWeakValue(module);return module;
}
JSValue nativeTangentUtilsBinding(const std::string& name){const auto keeper=nativeTangentUtilsModule();return lexical().lock()->get(name);}
