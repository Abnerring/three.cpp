#include "materials/SpriteMaterial.h"

SpriteMaterial::SpriteMaterial(const JsonValue& parameters) {
    type = "SpriteMaterial";
    transparent = true;
    setValues(parameters);
}
void SpriteMaterial::visitProperties(const PropertyVisitor& visitor) {
    Material::visitProperties(visitor);
    visitor("isSpriteMaterial", &isSpriteMaterial);
    visitor("color", &color);
    visitor("map", &map);
    visitor("alphaMap", &alphaMap);
    visitor("rotation", &rotation);
    visitor("sizeAttenuation", &sizeAttenuation);
    visitor("fog", &fog);
}
Material& SpriteMaterial::copy(const Material& source) {
    const auto& s = dynamic_cast<const SpriteMaterial&>(source);
    Material::copy(source);
    color.copy( s.color );
    map = s.map;
    alphaMap = s.alphaMap;
    rotation = s.rotation;
    sizeAttenuation = s.sizeAttenuation;
    fog = s.fog;
    return *this;
}
std::shared_ptr<Material> SpriteMaterial::clone() const {
    auto result = std::make_shared<SpriteMaterial>();
    result->copy(*this);
    return result;
}

// Managed source-class authority. Original typed implementation above remains byte-exact.
#include "core/JSModuleObject.h"
#include "materials/MaterialClassRuntime.h"
#include "materials/Material.h"
#include "math/Color.h"
namespace SpriteMaterialManaged {
using namespace MaterialClassRuntime;
#include "SpriteMaterialManagedSource.inc"
#include "SpriteMaterialManagedStatements.inc"
JSWeakValue& lexical(){static thread_local JSWeakValue value;return value;}
}
JSValue nativeSpriteMaterialConstructor(){
using namespace SpriteMaterialManaged;static thread_local JSWeakValue weak;if(const auto cached=weak.lock();cached&&cached->isConstructible())return *cached;static thread_local bool active=false;JSModuleInitializationScope scope(active,"SpriteMaterial");
auto imports=JSValue::object(nullptr);own(imports,"Material",nativeMaterialConstructor());
own(imports,"Color",nativeColorConstructor());
const auto base=imports.get("Material");auto holder=JSValue::object(nullptr);own(holder,"imports",imports);
auto ctor=JSValue::builtinConstructor(+[](const JSValue& target,const Args& args,const Args& captures)->JSValue{const auto holder=captures[0];const auto parent=holder.get("constructor").getPrototype();auto self=parent.construct({},target);return SpriteMaterialManaged::constructor(self,args,holder.get("imports"),JSValue{});},{holder},"SpriteMaterial",1);own(holder,"constructor",ctor);ctor.setPrototype(base);auto prototype=ctor.get("prototype");prototype.setPrototype(base.get("prototype"));ctor.setFunctionSourceText(SpriteMaterialSource::Class);
{auto method=JSValue::function(+[](const JSValue& self,const Args& args,const Args& captures)->JSValue{return SpriteMaterialManaged::copy(self,args,captures[0],captures[1]);},{imports,prototype},"copy",1);method.setFunctionSourceText(SpriteMaterialSource::copy);auto descriptor=prototype.getOwnPropertyDescriptor("copy").value_or(JSPropertyDescriptor{});descriptor.configurable=true;descriptor.value=method;descriptor.writable=true;prototype.defineProperty("copy",descriptor);}
ctor.retainForHeapLifetime();weak=JSWeakValue(ctor);return ctor;}
JSValue nativeSpriteMaterialModule(){using namespace SpriteMaterialManaged;static thread_local JSWeakValue weak;if(const auto cached=weak.lock();cached&&cached->isObject())return *cached;auto bindings=JSValue::object(nullptr);own(bindings,"SpriteMaterial",nativeSpriteMaterialConstructor());auto facade=JSValue::object();own(facade,"SpriteMaterial",bindings.get("SpriteMaterial"));bindings.retainForHeapLifetime();lexical()=JSWeakValue(bindings);facade.retainForHeapLifetime();weak=JSWeakValue(facade);return facade;}
JSValue nativeSpriteMaterialBinding(const std::string& name){const auto keeper=nativeSpriteMaterialModule();return SpriteMaterialManaged::lexical().lock()->get(name);}
