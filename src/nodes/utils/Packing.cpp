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
#include <algorithm>
namespace {
using namespace TSLFunctionRuntime;
JSWeakValue& lexical(){static thread_local JSWeakValue weak;return weak;}
JSValue packingCall(const JSValue&,const Args& args,const Args& c){
 const auto name=c[0],table=c[1],node=arg(args,0);
 if(name.strictEquals("directionToColor")){ThreeUtils::warnOnce({"TSL: \"directionToColor()\" has been renamed to \"packNormalToRGB()\"."});return table.get("packNormalToRGB").call({}, {node});}
 if(name.strictEquals("colorToDirection")){ThreeUtils::warnOnce({"TSL: \"colorToDirection()\" has been renamed to \"unpackRGBToNormal()\"."});return table.get("unpackRGBToNormal").call({}, {node});}
 if(name.strictEquals("packNormalToRGB"))return invoke(invoke(table.get("nodeObject").call({}, {node}),"mul")({.5}),"add")({.5});
 if(name.strictEquals("unpackRGBToNormal"))return invoke(invoke(table.get("nodeObject").call({}, {node}),"mul")({2}),"sub")({1});
 const auto vec3=table.get("vec3"),sqrt=table.get("sqrt"),saturate=table.get("saturate");const auto subtract=invoke(table.get("float").call({}, {1}),"sub");
 const auto z=sqrt.call({}, {saturate.call({}, {subtract({table.get("dot").call({}, {node,node})})})});return vec3.call({}, {node,z});
}
}
JSValue nativePackingModule(){
 using namespace TSLFunctionRuntime;static thread_local JSWeakValue weak;if(const auto v=weak.lock();v&&v->isObject())return *v;
 static thread_local bool initializing=false;JSModuleInitializationScope scope(initializing,"Packing module");const auto tsl=nativeTSLBase(),math=nativeMathNodeModule();
 auto table=JSValue::object(nullptr),bindings=JSValue::object(nullptr);
 for(const auto* name:{"nodeObject","vec3","float"})own(table,name,nativeTSLBaseBinding(name));for(const auto* name:{"dot","sqrt","saturate"})own(table,name,nativeMathNodeBinding(name));
 const char* names[]={"packNormalToRGB","unpackRGBToNormal","unpackNormal","directionToColor","colorToDirection"};
 const char* sources[]={PackingSource::packNormalToRGBSource,PackingSource::unpackRGBToNormalSource,PackingSource::unpackNormalSource,PackingSource::directionToColorSource,PackingSource::colorToDirectionSource};
 for(unsigned i=0;i<5;++i){auto fn=JSValue::function(packingCall,{names[i],table},names[i],1);fn.setFunctionSourceText(sources[i]);own(table,names[i],fn);own(bindings,names[i],fn);}
 auto module=JSValue::object();auto keys=bindings.ownPropertyNames();std::sort(keys.begin(),keys.end());for(const auto& name:keys)own(module,name.c_str(),bindings.get(name));
 bindings.retainForHeapLifetime();lexical()=JSWeakValue(bindings);module.retainForHeapLifetime();weak=JSWeakValue(module);return module;
}
JSValue nativePackingBinding(const std::string& name){const auto keeper=nativePackingModule();return lexical().lock()->get(name);}
