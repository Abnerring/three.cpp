#include "materials/MeshPhysicalMaterial.h"

MeshPhysicalMaterial::MeshPhysicalMaterial(const JsonValue& parameters) {
    defines = JSValue::fromJSON(JsonValue::Object{{"STANDARD", ""}, {"PHYSICAL", ""}});
    type = "MeshPhysicalMaterial";
    setValues(parameters);
}
void MeshPhysicalMaterial::visitProperties(const PropertyVisitor& visitor) {
    MeshStandardMaterial::visitProperties(visitor);
    visitor("isMeshPhysicalMaterial", &isMeshPhysicalMaterial);
    visitor("anisotropyRotation", &anisotropyRotation);
    visitor("anisotropyMap", &anisotropyMap);
    visitor("clearcoatMap", &clearcoatMap);
    visitor("clearcoatRoughness", &clearcoatRoughness);
    visitor("clearcoatRoughnessMap", &clearcoatRoughnessMap);
    visitor("clearcoatNormalScale", &clearcoatNormalScale);
    visitor("clearcoatNormalMap", &clearcoatNormalMap);
    visitor("diffuseRoughnessMap", &diffuseRoughnessMap);
    visitor("ior", &ior);
    visitor("reflectivity", &reflectivity);
    visitor("iridescenceMap", &iridescenceMap);
    visitor("iridescenceIOR", &iridescenceIOR);
    visitor("iridescenceThicknessRange", &iridescenceThicknessRange);
    visitor("iridescenceThicknessMap", &iridescenceThicknessMap);
    visitor("sheenColor", &sheenColor);
    visitor("sheenColorMap", &sheenColorMap);
    visitor("sheenRoughness", &sheenRoughness);
    visitor("sheenRoughnessMap", &sheenRoughnessMap);
    visitor("transmissionMap", &transmissionMap);
    visitor("thickness", &thickness);
    visitor("thicknessMap", &thicknessMap);
    visitor("attenuationDistance", &attenuationDistance);
    visitor("attenuationColor", &attenuationColor);
    visitor("specularIntensity", &specularIntensity);
    visitor("specularIntensityMap", &specularIntensityMap);
    visitor("specularColor", &specularColor);
    visitor("specularColorMap", &specularColorMap);
    visitor("_anisotropy", &anisotropy.value);
    visitor("anisotropy", &anisotropy);
    visitor("_clearcoat", &clearcoat.value);
    visitor("clearcoat", &clearcoat);
    visitor("_diffuseRoughness", &diffuseRoughness.value);
    visitor("diffuseRoughness", &diffuseRoughness);
    visitor("_dispersion", &dispersion.value);
    visitor("dispersion", &dispersion);
    visitor("_iridescence", &iridescence.value);
    visitor("iridescence", &iridescence);
    visitor("_retroreflectivity", &retroreflectivity.value);
    visitor("retroreflectivity", &retroreflectivity);
    visitor("_sheen", &sheen.value);
    visitor("sheen", &sheen);
    visitor("_transmission", &transmission.value);
    visitor("transmission", &transmission);
}
Material& MeshPhysicalMaterial::copy(const Material& source) {
    const auto& s = dynamic_cast<const MeshPhysicalMaterial&>(source);
    MeshStandardMaterial::copy(source);
    defines = JSValue::fromJSON(JsonValue::Object{{"STANDARD", ""}, {"PHYSICAL", ""}});
    anisotropy = s.anisotropy;
    anisotropyRotation = s.anisotropyRotation;
    anisotropyMap = s.anisotropyMap;
    clearcoat = s.clearcoat;
    clearcoatMap = s.clearcoatMap;
    clearcoatRoughness = s.clearcoatRoughness;
    clearcoatRoughnessMap = s.clearcoatRoughnessMap;
    clearcoatNormalMap = s.clearcoatNormalMap;
    clearcoatNormalScale.copy( s.clearcoatNormalScale );
    diffuseRoughness = s.diffuseRoughness;
    diffuseRoughnessMap = s.diffuseRoughnessMap;
    dispersion = s.dispersion;
    ior = s.ior;
    iridescence = s.iridescence;
    iridescenceMap = s.iridescenceMap;
    iridescenceIOR = s.iridescenceIOR;
    const auto sourceRange = s.iridescenceThicknessRange;
    auto copiedRange = JSValue::array(sourceRange.length());
    for (std::uint32_t i = 0; i < copiedRange.length(); ++i)
        copiedRange.set(std::to_string(i), sourceRange.get(std::to_string(i)));
    iridescenceThicknessRange = std::move(copiedRange);
    iridescenceThicknessMap = s.iridescenceThicknessMap;
    retroreflectivity = s.retroreflectivity;
    sheen = s.sheen;
    sheenColor.copy( s.sheenColor );
    sheenColorMap = s.sheenColorMap;
    sheenRoughness = s.sheenRoughness;
    sheenRoughnessMap = s.sheenRoughnessMap;
    transmission = s.transmission;
    transmissionMap = s.transmissionMap;
    thickness = s.thickness;
    thicknessMap = s.thicknessMap;
    attenuationDistance = s.attenuationDistance;
    attenuationColor.copy( s.attenuationColor );
    specularIntensity = s.specularIntensity;
    specularIntensityMap = s.specularIntensityMap;
    specularColor.copy( s.specularColor );
    specularColorMap = s.specularColorMap;
    return *this;
}
std::shared_ptr<Material> MeshPhysicalMaterial::clone() const {
    auto result = std::make_shared<MeshPhysicalMaterial>();
    result->copy(*this);
    return result;
}

// Managed source-class authority. Original typed implementation above remains byte-exact.
#include "core/JSModuleObject.h"
#include "materials/MaterialClassRuntime.h"
#include "math/Vector2.h"
#include "materials/MeshStandardMaterial.h"
#include "math/Color.h"
#include "math/MathUtils.h"
namespace MeshPhysicalMaterialManaged {
using namespace MaterialClassRuntime;
#include "MeshPhysicalMaterialManagedSource.inc"
#include "MeshPhysicalMaterialManagedStatements.inc"
JSWeakValue& lexical(){static thread_local JSWeakValue value;return value;}
}
JSValue nativeMeshPhysicalMaterialConstructor(){
using namespace MeshPhysicalMaterialManaged;static thread_local JSWeakValue weak;if(const auto cached=weak.lock();cached&&cached->isConstructible())return *cached;static thread_local bool active=false;JSModuleInitializationScope scope(active,"MeshPhysicalMaterial");
auto imports=JSValue::object(nullptr);own(imports,"Vector2",nativeVector2Constructor());
own(imports,"MeshStandardMaterial",nativeMeshStandardMaterialConstructor());
own(imports,"Color",nativeColorConstructor());
own(imports,"clamp",nativeMathUtilsClamp());
const auto base=imports.get("MeshStandardMaterial");auto holder=JSValue::object(nullptr);own(holder,"imports",imports);
auto ctor=JSValue::builtinConstructor(+[](const JSValue& target,const Args& args,const Args& captures)->JSValue{const auto holder=captures[0];const auto parent=holder.get("constructor").getPrototype();auto self=parent.construct({},target);return MeshPhysicalMaterialManaged::constructor(self,args,holder.get("imports"),JSValue{});},{holder},"MeshPhysicalMaterial",1);own(holder,"constructor",ctor);ctor.setPrototype(base);auto prototype=ctor.get("prototype");prototype.setPrototype(base.get("prototype"));ctor.setFunctionSourceText(MeshPhysicalMaterialSource::Class);
{auto method=JSValue::function(+[](const JSValue& self,const Args& args,const Args& captures)->JSValue{return MeshPhysicalMaterialManaged::get_anisotropy(self,args,captures[0],captures[1]);},{imports,prototype},"get anisotropy",0);method.setFunctionSourceText(MeshPhysicalMaterialSource::get_anisotropy);auto descriptor=prototype.getOwnPropertyDescriptor("anisotropy").value_or(JSPropertyDescriptor{});descriptor.configurable=true;descriptor.getter=method;prototype.defineProperty("anisotropy",descriptor);}
{auto method=JSValue::function(+[](const JSValue& self,const Args& args,const Args& captures)->JSValue{return MeshPhysicalMaterialManaged::set_anisotropy(self,args,captures[0],captures[1]);},{imports,prototype},"set anisotropy",1);method.setFunctionSourceText(MeshPhysicalMaterialSource::set_anisotropy);auto descriptor=prototype.getOwnPropertyDescriptor("anisotropy").value_or(JSPropertyDescriptor{});descriptor.configurable=true;descriptor.setter=method;prototype.defineProperty("anisotropy",descriptor);}
{auto method=JSValue::function(+[](const JSValue& self,const Args& args,const Args& captures)->JSValue{return MeshPhysicalMaterialManaged::get_clearcoat(self,args,captures[0],captures[1]);},{imports,prototype},"get clearcoat",0);method.setFunctionSourceText(MeshPhysicalMaterialSource::get_clearcoat);auto descriptor=prototype.getOwnPropertyDescriptor("clearcoat").value_or(JSPropertyDescriptor{});descriptor.configurable=true;descriptor.getter=method;prototype.defineProperty("clearcoat",descriptor);}
{auto method=JSValue::function(+[](const JSValue& self,const Args& args,const Args& captures)->JSValue{return MeshPhysicalMaterialManaged::set_clearcoat(self,args,captures[0],captures[1]);},{imports,prototype},"set clearcoat",1);method.setFunctionSourceText(MeshPhysicalMaterialSource::set_clearcoat);auto descriptor=prototype.getOwnPropertyDescriptor("clearcoat").value_or(JSPropertyDescriptor{});descriptor.configurable=true;descriptor.setter=method;prototype.defineProperty("clearcoat",descriptor);}
{auto method=JSValue::function(+[](const JSValue& self,const Args& args,const Args& captures)->JSValue{return MeshPhysicalMaterialManaged::get_diffuseRoughness(self,args,captures[0],captures[1]);},{imports,prototype},"get diffuseRoughness",0);method.setFunctionSourceText(MeshPhysicalMaterialSource::get_diffuseRoughness);auto descriptor=prototype.getOwnPropertyDescriptor("diffuseRoughness").value_or(JSPropertyDescriptor{});descriptor.configurable=true;descriptor.getter=method;prototype.defineProperty("diffuseRoughness",descriptor);}
{auto method=JSValue::function(+[](const JSValue& self,const Args& args,const Args& captures)->JSValue{return MeshPhysicalMaterialManaged::set_diffuseRoughness(self,args,captures[0],captures[1]);},{imports,prototype},"set diffuseRoughness",1);method.setFunctionSourceText(MeshPhysicalMaterialSource::set_diffuseRoughness);auto descriptor=prototype.getOwnPropertyDescriptor("diffuseRoughness").value_or(JSPropertyDescriptor{});descriptor.configurable=true;descriptor.setter=method;prototype.defineProperty("diffuseRoughness",descriptor);}
{auto method=JSValue::function(+[](const JSValue& self,const Args& args,const Args& captures)->JSValue{return MeshPhysicalMaterialManaged::get_iridescence(self,args,captures[0],captures[1]);},{imports,prototype},"get iridescence",0);method.setFunctionSourceText(MeshPhysicalMaterialSource::get_iridescence);auto descriptor=prototype.getOwnPropertyDescriptor("iridescence").value_or(JSPropertyDescriptor{});descriptor.configurable=true;descriptor.getter=method;prototype.defineProperty("iridescence",descriptor);}
{auto method=JSValue::function(+[](const JSValue& self,const Args& args,const Args& captures)->JSValue{return MeshPhysicalMaterialManaged::set_iridescence(self,args,captures[0],captures[1]);},{imports,prototype},"set iridescence",1);method.setFunctionSourceText(MeshPhysicalMaterialSource::set_iridescence);auto descriptor=prototype.getOwnPropertyDescriptor("iridescence").value_or(JSPropertyDescriptor{});descriptor.configurable=true;descriptor.setter=method;prototype.defineProperty("iridescence",descriptor);}
{auto method=JSValue::function(+[](const JSValue& self,const Args& args,const Args& captures)->JSValue{return MeshPhysicalMaterialManaged::get_dispersion(self,args,captures[0],captures[1]);},{imports,prototype},"get dispersion",0);method.setFunctionSourceText(MeshPhysicalMaterialSource::get_dispersion);auto descriptor=prototype.getOwnPropertyDescriptor("dispersion").value_or(JSPropertyDescriptor{});descriptor.configurable=true;descriptor.getter=method;prototype.defineProperty("dispersion",descriptor);}
{auto method=JSValue::function(+[](const JSValue& self,const Args& args,const Args& captures)->JSValue{return MeshPhysicalMaterialManaged::set_dispersion(self,args,captures[0],captures[1]);},{imports,prototype},"set dispersion",1);method.setFunctionSourceText(MeshPhysicalMaterialSource::set_dispersion);auto descriptor=prototype.getOwnPropertyDescriptor("dispersion").value_or(JSPropertyDescriptor{});descriptor.configurable=true;descriptor.setter=method;prototype.defineProperty("dispersion",descriptor);}
{auto method=JSValue::function(+[](const JSValue& self,const Args& args,const Args& captures)->JSValue{return MeshPhysicalMaterialManaged::get_retroreflectivity(self,args,captures[0],captures[1]);},{imports,prototype},"get retroreflectivity",0);method.setFunctionSourceText(MeshPhysicalMaterialSource::get_retroreflectivity);auto descriptor=prototype.getOwnPropertyDescriptor("retroreflectivity").value_or(JSPropertyDescriptor{});descriptor.configurable=true;descriptor.getter=method;prototype.defineProperty("retroreflectivity",descriptor);}
{auto method=JSValue::function(+[](const JSValue& self,const Args& args,const Args& captures)->JSValue{return MeshPhysicalMaterialManaged::set_retroreflectivity(self,args,captures[0],captures[1]);},{imports,prototype},"set retroreflectivity",1);method.setFunctionSourceText(MeshPhysicalMaterialSource::set_retroreflectivity);auto descriptor=prototype.getOwnPropertyDescriptor("retroreflectivity").value_or(JSPropertyDescriptor{});descriptor.configurable=true;descriptor.setter=method;prototype.defineProperty("retroreflectivity",descriptor);}
{auto method=JSValue::function(+[](const JSValue& self,const Args& args,const Args& captures)->JSValue{return MeshPhysicalMaterialManaged::get_sheen(self,args,captures[0],captures[1]);},{imports,prototype},"get sheen",0);method.setFunctionSourceText(MeshPhysicalMaterialSource::get_sheen);auto descriptor=prototype.getOwnPropertyDescriptor("sheen").value_or(JSPropertyDescriptor{});descriptor.configurable=true;descriptor.getter=method;prototype.defineProperty("sheen",descriptor);}
{auto method=JSValue::function(+[](const JSValue& self,const Args& args,const Args& captures)->JSValue{return MeshPhysicalMaterialManaged::set_sheen(self,args,captures[0],captures[1]);},{imports,prototype},"set sheen",1);method.setFunctionSourceText(MeshPhysicalMaterialSource::set_sheen);auto descriptor=prototype.getOwnPropertyDescriptor("sheen").value_or(JSPropertyDescriptor{});descriptor.configurable=true;descriptor.setter=method;prototype.defineProperty("sheen",descriptor);}
{auto method=JSValue::function(+[](const JSValue& self,const Args& args,const Args& captures)->JSValue{return MeshPhysicalMaterialManaged::get_transmission(self,args,captures[0],captures[1]);},{imports,prototype},"get transmission",0);method.setFunctionSourceText(MeshPhysicalMaterialSource::get_transmission);auto descriptor=prototype.getOwnPropertyDescriptor("transmission").value_or(JSPropertyDescriptor{});descriptor.configurable=true;descriptor.getter=method;prototype.defineProperty("transmission",descriptor);}
{auto method=JSValue::function(+[](const JSValue& self,const Args& args,const Args& captures)->JSValue{return MeshPhysicalMaterialManaged::set_transmission(self,args,captures[0],captures[1]);},{imports,prototype},"set transmission",1);method.setFunctionSourceText(MeshPhysicalMaterialSource::set_transmission);auto descriptor=prototype.getOwnPropertyDescriptor("transmission").value_or(JSPropertyDescriptor{});descriptor.configurable=true;descriptor.setter=method;prototype.defineProperty("transmission",descriptor);}
{auto method=JSValue::function(+[](const JSValue& self,const Args& args,const Args& captures)->JSValue{return MeshPhysicalMaterialManaged::copy(self,args,captures[0],captures[1]);},{imports,prototype},"copy",1);method.setFunctionSourceText(MeshPhysicalMaterialSource::copy);auto descriptor=prototype.getOwnPropertyDescriptor("copy").value_or(JSPropertyDescriptor{});descriptor.configurable=true;descriptor.value=method;descriptor.writable=true;prototype.defineProperty("copy",descriptor);}
ctor.retainForHeapLifetime();weak=JSWeakValue(ctor);return ctor;}
JSValue nativeMeshPhysicalMaterialModule(){using namespace MeshPhysicalMaterialManaged;static thread_local JSWeakValue weak;if(const auto cached=weak.lock();cached&&cached->isObject())return *cached;auto bindings=JSValue::object(nullptr);own(bindings,"MeshPhysicalMaterial",nativeMeshPhysicalMaterialConstructor());auto facade=JSValue::object();own(facade,"MeshPhysicalMaterial",bindings.get("MeshPhysicalMaterial"));bindings.retainForHeapLifetime();lexical()=JSWeakValue(bindings);facade.retainForHeapLifetime();weak=JSWeakValue(facade);return facade;}
JSValue nativeMeshPhysicalMaterialBinding(const std::string& name){const auto keeper=nativeMeshPhysicalMaterialModule();return MeshPhysicalMaterialManaged::lexical().lock()->get(name);}
