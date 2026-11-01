#include "materials/ShadowMaterial.h"

ShadowMaterial::ShadowMaterial(const JsonValue& parameters) {
    type = "ShadowMaterial";
    transparent = true;
    setValues(parameters);
}
void ShadowMaterial::visitProperties(const PropertyVisitor& visitor) {
    Material::visitProperties(visitor);
    visitor("isShadowMaterial", &isShadowMaterial);
    visitor("color", &color);
    visitor("fog", &fog);
}
Material& ShadowMaterial::copy(const Material& source) {
    const auto& s = dynamic_cast<const ShadowMaterial&>(source);
    Material::copy(source);
    color.copy( s.color );
    fog = s.fog;
    return *this;
}
std::shared_ptr<Material> ShadowMaterial::clone() const {
    auto result = std::make_shared<ShadowMaterial>();
    result->copy(*this);
    return result;
}

// Managed source-class authority. Original typed implementation above remains byte-exact.
#include "core/JSModuleObject.h"
#include "materials/MaterialClassRuntime.h"
#include "materials/Material.h"
#include "math/Color.h"
namespace ShadowMaterialManaged {
using namespace MaterialClassRuntime;
#include "ShadowMaterialManagedSource.inc"
#include "ShadowMaterialManagedStatements.inc"
JSWeakValue& lexical(){static thread_local JSWeakValue value;return value;}
}
JSValue nativeShadowMaterialConstructor(){
using namespace ShadowMaterialManaged;static thread_local JSWeakValue weak;if(const auto cached=weak.lock();cached&&cached->isConstructible())return *cached;static thread_local bool active=false;JSModuleInitializationScope scope(active,"ShadowMaterial");
auto imports=JSValue::object(nullptr);own(imports,"Material",nativeMaterialConstructor());
own(imports,"Color",nativeColorConstructor());
const auto base=imports.get("Material");auto holder=JSValue::object(nullptr);own(holder,"imports",imports);
auto ctor=JSValue::builtinConstructor(+[](const JSValue& target,const Args& args,const Args& captures)->JSValue{const auto holder=captures[0];const auto parent=holder.get("constructor").getPrototype();auto self=parent.construct({},target);return ShadowMaterialManaged::constructor(self,args,holder.get("imports"),JSValue{});},{holder},"ShadowMaterial",1);own(holder,"constructor",ctor);ctor.setPrototype(base);auto prototype=ctor.get("prototype");prototype.setPrototype(base.get("prototype"));ctor.setFunctionSourceText(ShadowMaterialSource::Class);
{auto method=JSValue::function(+[](const JSValue& self,const Args& args,const Args& captures)->JSValue{return ShadowMaterialManaged::copy(self,args,captures[0],captures[1]);},{imports,prototype},"copy",1);method.setFunctionSourceText(ShadowMaterialSource::copy);auto descriptor=prototype.getOwnPropertyDescriptor("copy").value_or(JSPropertyDescriptor{});descriptor.configurable=true;descriptor.value=method;descriptor.writable=true;prototype.defineProperty("copy",descriptor);}
ctor.retainForHeapLifetime();weak=JSWeakValue(ctor);return ctor;}
JSValue nativeShadowMaterialModule(){using namespace ShadowMaterialManaged;static thread_local JSWeakValue weak;if(const auto cached=weak.lock();cached&&cached->isObject())return *cached;auto bindings=JSValue::object(nullptr);own(bindings,"ShadowMaterial",nativeShadowMaterialConstructor());auto facade=JSValue::object();own(facade,"ShadowMaterial",bindings.get("ShadowMaterial"));bindings.retainForHeapLifetime();lexical()=JSWeakValue(bindings);facade.retainForHeapLifetime();weak=JSWeakValue(facade);return facade;}
JSValue nativeShadowMaterialBinding(const std::string& name){const auto keeper=nativeShadowMaterialModule();return ShadowMaterialManaged::lexical().lock()->get(name);}
