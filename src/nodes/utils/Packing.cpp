#include "nodes/utils/Packing.h"

#include <cmath>

double Packing::packPart(double direction) {
    return direction * 0.5 + 0.5;
}

double Packing::unpackPart(double colour) {
    // Twice the value less one, which undoes the packing for a value inside the range.
    return colour * 2.0 - 1.0;
}

double Packing::saturate(double value) {
    // A clamp, so negative values and values above one are both brought to the ends.
    if (value < 0.0) return 0.0;
    if (value > 1.0) return 1.0;
    return value;
}

double Packing::reconstructedZ(double x, double y) {
    // One less the squared length, clamped, then rooted.
    const double remainder = 1.0 - (x * x + y * y);
    return std::sqrt(saturate(remainder));
}

std::string Packing::packRenameWarning() {
    return "TSL: \"directionToColor()\" has been renamed to \"packNormalToRGB()\".";
}

std::string Packing::unpackRenameWarning() {
    return "TSL: \"colorToDirection()\" has been renamed to \"unpackRGBToNormal()\".";
}

#include "nodes/tsl/TSLBase.h"
#include "nodes/math/MathNode.h"
#include "nodes/tsl/TSLFunctionRuntime.h"
#include "nodes/accessors/TangentNormalBumpSources.inc"
#include "utils.h"
#include "core/JSSourceTrace.h"
#include <algorithm>
namespace {
using namespace TSLFunctionRuntime;
JSWeakValue& lexical(){static thread_local JSWeakValue weak;return weak;}
JSValue packing0(const JSValue& nativeStatementTemp0,const Args& nativeStatementTemp1,const Args& nativeStatementTemp2){
JSSourceTrace::FrameScope sourceTraceFrame_packing0({"packNormalToRGB",{"file:///D:/Temp/surui/threejs-pinned-11738a9-20261101/src/nodes/utils/Packing.js",13,32,372,"05a9bb00b9a5eb6f983c3947f6da3cd7b1270e7ecbec2226722a43ea3e76acf6"},std::string{},"",false,false,false,{"file:///D:/Temp/surui/threejs-pinned-11738a9-20261101/src/nodes/utils/Packing.js",13,32,372,"05a9bb00b9a5eb6f983c3947f6da3cd7b1270e7ecbec2226722a43ea3e76acf6"}});
auto nativeStatementTemp3=nativeStatementTemp2[0];auto nativeStatementTemp4=nativeStatementTemp0;
JSValue node=arg(nativeStatementTemp1,0);
return ([&]()->JSValue{auto nativeStatementTemp9=([&]()->JSValue{auto nativeStatementTemp14=([&]()->JSValue{const auto nativeStatementTemp16=nativeStatementTemp3.get("nodeObject");const JSValue nativeStatementTemp17;Args nativeStatementTemp18;nativeStatementTemp18.push_back(node);JSSourceTrace::PositionScope sourceTracePosition_384({"file:///D:/Temp/surui/threejs-pinned-11738a9-20261101/src/nodes/utils/Packing.js",13,44,384,"05a9bb00b9a5eb6f983c3947f6da3cd7b1270e7ecbec2226722a43ea3e76acf6"});return nativeStatementTemp16.call(nativeStatementTemp17,nativeStatementTemp18);}());const auto nativeStatementTemp15=JSValue("mul");const auto nativeStatementTemp11=nativeStatementTemp14.get(nativeStatementTemp15);const auto nativeStatementTemp12=nativeStatementTemp14;Args nativeStatementTemp13;nativeStatementTemp13.push_back(JSValue(0.5));JSSourceTrace::PositionScope sourceTracePosition_384({"file:///D:/Temp/surui/threejs-pinned-11738a9-20261101/src/nodes/utils/Packing.js",13,63,403,"05a9bb00b9a5eb6f983c3947f6da3cd7b1270e7ecbec2226722a43ea3e76acf6"});return nativeStatementTemp11.call(nativeStatementTemp12,nativeStatementTemp13);}());const auto nativeStatementTemp10=JSValue("add");const auto nativeStatementTemp6=nativeStatementTemp9.get(nativeStatementTemp10);const auto nativeStatementTemp7=nativeStatementTemp9;Args nativeStatementTemp8;nativeStatementTemp8.push_back(JSValue(0.5));JSSourceTrace::PositionScope sourceTracePosition_384({"file:///D:/Temp/surui/threejs-pinned-11738a9-20261101/src/nodes/utils/Packing.js",13,74,414,"05a9bb00b9a5eb6f983c3947f6da3cd7b1270e7ecbec2226722a43ea3e76acf6"});return nativeStatementTemp6.call(nativeStatementTemp7,nativeStatementTemp8);}());
}

JSValue packing1(const JSValue& nativeStatementTemp19,const Args& nativeStatementTemp20,const Args& nativeStatementTemp21){
JSSourceTrace::FrameScope sourceTraceFrame_packing1({"unpackRGBToNormal",{"file:///D:/Temp/surui/threejs-pinned-11738a9-20261101/src/nodes/utils/Packing.js",23,34,639,"05a9bb00b9a5eb6f983c3947f6da3cd7b1270e7ecbec2226722a43ea3e76acf6"},std::string{},"",false,false,false,{"file:///D:/Temp/surui/threejs-pinned-11738a9-20261101/src/nodes/utils/Packing.js",23,34,639,"05a9bb00b9a5eb6f983c3947f6da3cd7b1270e7ecbec2226722a43ea3e76acf6"}});
auto nativeStatementTemp22=nativeStatementTemp21[0];auto nativeStatementTemp23=nativeStatementTemp19;
JSValue node=arg(nativeStatementTemp20,0);
return ([&]()->JSValue{auto nativeStatementTemp28=([&]()->JSValue{auto nativeStatementTemp33=([&]()->JSValue{const auto nativeStatementTemp35=nativeStatementTemp22.get("nodeObject");const JSValue nativeStatementTemp36;Args nativeStatementTemp37;nativeStatementTemp37.push_back(node);JSSourceTrace::PositionScope sourceTracePosition_651({"file:///D:/Temp/surui/threejs-pinned-11738a9-20261101/src/nodes/utils/Packing.js",23,46,651,"05a9bb00b9a5eb6f983c3947f6da3cd7b1270e7ecbec2226722a43ea3e76acf6"});return nativeStatementTemp35.call(nativeStatementTemp36,nativeStatementTemp37);}());const auto nativeStatementTemp34=JSValue("mul");const auto nativeStatementTemp30=nativeStatementTemp33.get(nativeStatementTemp34);const auto nativeStatementTemp31=nativeStatementTemp33;Args nativeStatementTemp32;nativeStatementTemp32.push_back(JSValue(2));JSSourceTrace::PositionScope sourceTracePosition_651({"file:///D:/Temp/surui/threejs-pinned-11738a9-20261101/src/nodes/utils/Packing.js",23,65,670,"05a9bb00b9a5eb6f983c3947f6da3cd7b1270e7ecbec2226722a43ea3e76acf6"});return nativeStatementTemp30.call(nativeStatementTemp31,nativeStatementTemp32);}());const auto nativeStatementTemp29=JSValue("sub");const auto nativeStatementTemp25=nativeStatementTemp28.get(nativeStatementTemp29);const auto nativeStatementTemp26=nativeStatementTemp28;Args nativeStatementTemp27;nativeStatementTemp27.push_back(JSValue(1));JSSourceTrace::PositionScope sourceTracePosition_651({"file:///D:/Temp/surui/threejs-pinned-11738a9-20261101/src/nodes/utils/Packing.js",23,76,681,"05a9bb00b9a5eb6f983c3947f6da3cd7b1270e7ecbec2226722a43ea3e76acf6"});return nativeStatementTemp25.call(nativeStatementTemp26,nativeStatementTemp27);}());
}

JSValue packing2(const JSValue& nativeStatementTemp38,const Args& nativeStatementTemp39,const Args& nativeStatementTemp40){
JSSourceTrace::FrameScope sourceTraceFrame_packing2({"unpackNormal",{"file:///D:/Temp/surui/threejs-pinned-11738a9-20261101/src/nodes/utils/Packing.js",34,29,1057,"05a9bb00b9a5eb6f983c3947f6da3cd7b1270e7ecbec2226722a43ea3e76acf6"},std::string{},"",false,false,false,{"file:///D:/Temp/surui/threejs-pinned-11738a9-20261101/src/nodes/utils/Packing.js",34,29,1057,"05a9bb00b9a5eb6f983c3947f6da3cd7b1270e7ecbec2226722a43ea3e76acf6"}});
auto nativeStatementTemp41=nativeStatementTemp40[0];auto nativeStatementTemp42=nativeStatementTemp38;
JSValue xy=arg(nativeStatementTemp39,0);
return ([&]()->JSValue{const auto nativeStatementTemp44=nativeStatementTemp41.get("vec3");const JSValue nativeStatementTemp45;Args nativeStatementTemp46;nativeStatementTemp46.push_back(xy);nativeStatementTemp46.push_back(([&]()->JSValue{const auto nativeStatementTemp47=nativeStatementTemp41.get("sqrt");const JSValue nativeStatementTemp48;Args nativeStatementTemp49;nativeStatementTemp49.push_back(([&]()->JSValue{const auto nativeStatementTemp50=nativeStatementTemp41.get("saturate");const JSValue nativeStatementTemp51;Args nativeStatementTemp52;nativeStatementTemp52.push_back(([&]()->JSValue{auto nativeStatementTemp56=([&]()->JSValue{const auto nativeStatementTemp58=nativeStatementTemp41.get("float");const JSValue nativeStatementTemp59;Args nativeStatementTemp60;nativeStatementTemp60.push_back(JSValue(1));JSSourceTrace::PositionScope sourceTracePosition_1093({"file:///D:/Temp/surui/threejs-pinned-11738a9-20261101/src/nodes/utils/Packing.js",34,65,1093,"05a9bb00b9a5eb6f983c3947f6da3cd7b1270e7ecbec2226722a43ea3e76acf6"});return nativeStatementTemp58.call(nativeStatementTemp59,nativeStatementTemp60);}());const auto nativeStatementTemp57=JSValue("sub");const auto nativeStatementTemp53=nativeStatementTemp56.get(nativeStatementTemp57);const auto nativeStatementTemp54=nativeStatementTemp56;Args nativeStatementTemp55;nativeStatementTemp55.push_back(([&]()->JSValue{const auto nativeStatementTemp61=nativeStatementTemp41.get("dot");const JSValue nativeStatementTemp62;Args nativeStatementTemp63;nativeStatementTemp63.push_back(xy);nativeStatementTemp63.push_back(xy);JSSourceTrace::PositionScope sourceTracePosition_1111({"file:///D:/Temp/surui/threejs-pinned-11738a9-20261101/src/nodes/utils/Packing.js",34,83,1111,"05a9bb00b9a5eb6f983c3947f6da3cd7b1270e7ecbec2226722a43ea3e76acf6"});return nativeStatementTemp61.call(nativeStatementTemp62,nativeStatementTemp63);}()));JSSourceTrace::PositionScope sourceTracePosition_1093({"file:///D:/Temp/surui/threejs-pinned-11738a9-20261101/src/nodes/utils/Packing.js",34,78,1106,"05a9bb00b9a5eb6f983c3947f6da3cd7b1270e7ecbec2226722a43ea3e76acf6"});return nativeStatementTemp53.call(nativeStatementTemp54,nativeStatementTemp55);}()));JSSourceTrace::PositionScope sourceTracePosition_1083({"file:///D:/Temp/surui/threejs-pinned-11738a9-20261101/src/nodes/utils/Packing.js",34,55,1083,"05a9bb00b9a5eb6f983c3947f6da3cd7b1270e7ecbec2226722a43ea3e76acf6"});return nativeStatementTemp50.call(nativeStatementTemp51,nativeStatementTemp52);}()));JSSourceTrace::PositionScope sourceTracePosition_1077({"file:///D:/Temp/surui/threejs-pinned-11738a9-20261101/src/nodes/utils/Packing.js",34,49,1077,"05a9bb00b9a5eb6f983c3947f6da3cd7b1270e7ecbec2226722a43ea3e76acf6"});return nativeStatementTemp47.call(nativeStatementTemp48,nativeStatementTemp49);}()));JSSourceTrace::PositionScope sourceTracePosition_1067({"file:///D:/Temp/surui/threejs-pinned-11738a9-20261101/src/nodes/utils/Packing.js",34,39,1067,"05a9bb00b9a5eb6f983c3947f6da3cd7b1270e7ecbec2226722a43ea3e76acf6"});return nativeStatementTemp44.call(nativeStatementTemp45,nativeStatementTemp46);}());
}

JSValue packing3(const JSValue& nativeStatementTemp64,const Args& nativeStatementTemp65,const Args& nativeStatementTemp66){
JSSourceTrace::FrameScope sourceTraceFrame_packing3({"directionToColor",{"file:///D:/Temp/surui/threejs-pinned-11738a9-20261101/src/nodes/utils/Packing.js",43,33,1347,"05a9bb00b9a5eb6f983c3947f6da3cd7b1270e7ecbec2226722a43ea3e76acf6"},std::string{},"",false,false,false,{"file:///D:/Temp/surui/threejs-pinned-11738a9-20261101/src/nodes/utils/Packing.js",43,33,1347,"05a9bb00b9a5eb6f983c3947f6da3cd7b1270e7ecbec2226722a43ea3e76acf6"}});
auto nativeStatementTemp67=nativeStatementTemp66[0];auto nativeStatementTemp68=nativeStatementTemp64;
JSValue node=arg(nativeStatementTemp65,0);
([&]()->JSValue{const auto nativeStatementTemp70=nativeStatementTemp67.get("warnOnce");const JSValue nativeStatementTemp71;Args nativeStatementTemp72;nativeStatementTemp72.push_back(JSValue("TSL: \"directionToColor()\" has been renamed to \"packNormalToRGB()\"."));JSSourceTrace::PositionScope sourceTracePosition_1365({"file:///D:/Temp/surui/threejs-pinned-11738a9-20261101/src/nodes/utils/Packing.js",45,2,1365,"05a9bb00b9a5eb6f983c3947f6da3cd7b1270e7ecbec2226722a43ea3e76acf6"});return nativeStatementTemp70.call(nativeStatementTemp71,nativeStatementTemp72);}());
return ([&]()->JSValue{const auto nativeStatementTemp73=nativeStatementTemp67.get("packNormalToRGB");const JSValue nativeStatementTemp74;Args nativeStatementTemp75;nativeStatementTemp75.push_back(node);JSSourceTrace::PositionScope sourceTracePosition_1478({"file:///D:/Temp/surui/threejs-pinned-11738a9-20261101/src/nodes/utils/Packing.js",47,9,1478,"05a9bb00b9a5eb6f983c3947f6da3cd7b1270e7ecbec2226722a43ea3e76acf6"});return nativeStatementTemp73.call(nativeStatementTemp74,nativeStatementTemp75);}());
return JSValue{};
}

JSValue packing4(const JSValue& nativeStatementTemp76,const Args& nativeStatementTemp77,const Args& nativeStatementTemp78){
JSSourceTrace::FrameScope sourceTraceFrame_packing4({"colorToDirection",{"file:///D:/Temp/surui/threejs-pinned-11738a9-20261101/src/nodes/utils/Packing.js",58,33,1722,"05a9bb00b9a5eb6f983c3947f6da3cd7b1270e7ecbec2226722a43ea3e76acf6"},std::string{},"",false,false,false,{"file:///D:/Temp/surui/threejs-pinned-11738a9-20261101/src/nodes/utils/Packing.js",58,33,1722,"05a9bb00b9a5eb6f983c3947f6da3cd7b1270e7ecbec2226722a43ea3e76acf6"}});
auto nativeStatementTemp79=nativeStatementTemp78[0];auto nativeStatementTemp80=nativeStatementTemp76;
JSValue node=arg(nativeStatementTemp77,0);
([&]()->JSValue{const auto nativeStatementTemp82=nativeStatementTemp79.get("warnOnce");const JSValue nativeStatementTemp83;Args nativeStatementTemp84;nativeStatementTemp84.push_back(JSValue("TSL: \"colorToDirection()\" has been renamed to \"unpackRGBToNormal()\"."));JSSourceTrace::PositionScope sourceTracePosition_1740({"file:///D:/Temp/surui/threejs-pinned-11738a9-20261101/src/nodes/utils/Packing.js",60,2,1740,"05a9bb00b9a5eb6f983c3947f6da3cd7b1270e7ecbec2226722a43ea3e76acf6"});return nativeStatementTemp82.call(nativeStatementTemp83,nativeStatementTemp84);}());
return ([&]()->JSValue{const auto nativeStatementTemp85=nativeStatementTemp79.get("unpackRGBToNormal");const JSValue nativeStatementTemp86;Args nativeStatementTemp87;nativeStatementTemp87.push_back(node);JSSourceTrace::PositionScope sourceTracePosition_1855({"file:///D:/Temp/surui/threejs-pinned-11738a9-20261101/src/nodes/utils/Packing.js",62,9,1855,"05a9bb00b9a5eb6f983c3947f6da3cd7b1270e7ecbec2226722a43ea3e76acf6"});return nativeStatementTemp85.call(nativeStatementTemp86,nativeStatementTemp87);}());
return JSValue{};
}

}
JSValue nativePackingModule(){
 using namespace TSLFunctionRuntime;static thread_local JSWeakValue weak;if(const auto v=weak.lock();v&&v->isObject())return *v;
 static thread_local bool initializing=false;JSModuleInitializationScope scope(initializing,"Packing module");const auto tsl=nativeTSLBase(),math=nativeMathNodeModule();
 auto table=JSValue::object(nullptr),bindings=JSValue::object(nullptr);
 for(const auto* name:{"nodeObject","vec3","float"})own(table,name,nativeTSLBaseBinding(name));for(const auto* name:{"dot","sqrt","saturate"})own(table,name,nativeMathNodeBinding(name));
 own(table,"warnOnce",ThreeUtils::nativeWarnOnceFunction());
 const JSValue::NativeFunction callbacks[]={packing0,packing1,packing2,packing3,packing4};
 const char* names[]={"packNormalToRGB","unpackRGBToNormal","unpackNormal","directionToColor","colorToDirection"};
 const char* sources[]={PackingSource::packNormalToRGBSource,PackingSource::unpackRGBToNormalSource,PackingSource::unpackNormalSource,PackingSource::directionToColorSource,PackingSource::colorToDirectionSource};
 for(unsigned i=0;i<5;++i){auto fn=JSValue::function(callbacks[i],{table},names[i],1);fn.setFunctionSourceText(sources[i]);own(table,names[i],fn);own(bindings,names[i],fn);}
 auto module=JSValue::object();auto keys=bindings.ownPropertyNames();std::sort(keys.begin(),keys.end());for(const auto& name:keys)own(module,name.c_str(),bindings.get(name));
 bindings.retainForHeapLifetime();lexical()=JSWeakValue(bindings);module.retainForHeapLifetime();weak=JSWeakValue(module);return module;
}
JSValue nativePackingBinding(const std::string& name){const auto keeper=nativePackingModule();return lexical().lock()->get(name);}
