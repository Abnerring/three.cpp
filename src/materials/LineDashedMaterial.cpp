#include "materials/LineDashedMaterial.h"

LineDashedMaterial::LineDashedMaterial(const JsonValue& parameters) {
    type = "LineDashedMaterial";
    setValues(parameters);
}
void LineDashedMaterial::visitProperties(const PropertyVisitor& visitor) {
    LineBasicMaterial::visitProperties(visitor);
    visitor("isLineDashedMaterial", &isLineDashedMaterial);
    visitor("scale", &scale);
    visitor("dashSize", &dashSize);
    visitor("gapSize", &gapSize);
}
Material& LineDashedMaterial::copy(const Material& source) {
    const auto& s = dynamic_cast<const LineDashedMaterial&>(source);
    LineBasicMaterial::copy(source);
    scale = s.scale;
    dashSize = s.dashSize;
    gapSize = s.gapSize;
    return *this;
}
std::shared_ptr<Material> LineDashedMaterial::clone() const {
    auto result = std::make_shared<LineDashedMaterial>();
    result->copy(*this);
    return result;
}

// Managed source-class authority. Original typed implementation above remains byte-exact.
#include "core/JSModuleObject.h"
#include "materials/MaterialClassRuntime.h"
#include "materials/LineBasicMaterial.h"
namespace LineDashedMaterialManaged {
using namespace MaterialClassRuntime;
#include "LineDashedMaterialManagedSource.inc"
#include "LineDashedMaterialManagedStatements.inc"
JSWeakValue& lexical(){static thread_local JSWeakValue value;return value;}
}
JSValue nativeLineDashedMaterialConstructor(){
using namespace LineDashedMaterialManaged;static thread_local JSWeakValue weak;if(const auto cached=weak.lock();cached&&cached->isConstructible())return *cached;static thread_local bool active=false;JSModuleInitializationScope scope(active,"LineDashedMaterial");
auto imports=JSValue::object(nullptr);own(imports,"LineBasicMaterial",nativeLineBasicMaterialConstructor());
const auto base=imports.get("LineBasicMaterial");auto holder=JSValue::object(nullptr);own(holder,"imports",imports);
auto ctor=JSValue::builtinConstructor(+[](const JSValue& target,const Args& args,const Args& captures)->JSValue{const auto holder=captures[0];const auto parent=holder.get("constructor").getPrototype();auto self=parent.construct({},target);return LineDashedMaterialManaged::constructor(self,args,holder.get("imports"),JSValue{});},{holder},"LineDashedMaterial",1);own(holder,"constructor",ctor);ctor.setPrototype(base);auto prototype=ctor.get("prototype");prototype.setPrototype(base.get("prototype"));ctor.setFunctionSourceText(LineDashedMaterialSource::Class);
{auto method=JSValue::function(+[](const JSValue& self,const Args& args,const Args& captures)->JSValue{return LineDashedMaterialManaged::copy(self,args,captures[0],captures[1]);},{imports,prototype},"copy",1);method.setFunctionSourceText(LineDashedMaterialSource::copy);auto descriptor=prototype.getOwnPropertyDescriptor("copy").value_or(JSPropertyDescriptor{});descriptor.configurable=true;descriptor.value=method;descriptor.writable=true;prototype.defineProperty("copy",descriptor);}
ctor.retainForHeapLifetime();weak=JSWeakValue(ctor);return ctor;}
JSValue nativeLineDashedMaterialModule(){using namespace LineDashedMaterialManaged;static thread_local JSWeakValue weak;if(const auto cached=weak.lock();cached&&cached->isObject())return *cached;auto bindings=JSValue::object(nullptr);own(bindings,"LineDashedMaterial",nativeLineDashedMaterialConstructor());auto facade=JSValue::object();own(facade,"LineDashedMaterial",bindings.get("LineDashedMaterial"));bindings.retainForHeapLifetime();lexical()=JSWeakValue(bindings);facade.retainForHeapLifetime();weak=JSWeakValue(facade);return facade;}
JSValue nativeLineDashedMaterialBinding(const std::string& name){const auto keeper=nativeLineDashedMaterialModule();return LineDashedMaterialManaged::lexical().lock()->get(name);}
