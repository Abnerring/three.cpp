#include "materials/PointsMaterial.h"

PointsMaterial::PointsMaterial(const JsonValue& parameters) {
    type = "PointsMaterial";
    setValues(parameters);
}
void PointsMaterial::visitProperties(const PropertyVisitor& visitor) {
    Material::visitProperties(visitor);
    visitor("isPointsMaterial", &isPointsMaterial);
    visitor("color", &color);
    visitor("map", &map);
    visitor("alphaMap", &alphaMap);
    visitor("size", &size);
    visitor("sizeAttenuation", &sizeAttenuation);
    visitor("fog", &fog);
}
Material& PointsMaterial::copy(const Material& source) {
    const auto& s = dynamic_cast<const PointsMaterial&>(source);
    Material::copy(source);
    color.copy( s.color );
    map = s.map;
    alphaMap = s.alphaMap;
    size = s.size;
    sizeAttenuation = s.sizeAttenuation;
    fog = s.fog;
    return *this;
}
std::shared_ptr<Material> PointsMaterial::clone() const {
    auto result = std::make_shared<PointsMaterial>();
    result->copy(*this);
    return result;
}

// Managed source-class authority. Original typed implementation above remains byte-exact.
#include "core/JSModuleObject.h"
#include "materials/MaterialClassRuntime.h"
#include "materials/Material.h"
#include "math/Color.h"
namespace PointsMaterialManaged {
using namespace MaterialClassRuntime;
#include "PointsMaterialManagedSource.inc"
#include "PointsMaterialManagedStatements.inc"
JSWeakValue& lexical(){static thread_local JSWeakValue value;return value;}
}
JSValue nativePointsMaterialConstructor(){
using namespace PointsMaterialManaged;static thread_local JSWeakValue weak;if(const auto cached=weak.lock();cached&&cached->isConstructible())return *cached;static thread_local bool active=false;JSModuleInitializationScope scope(active,"PointsMaterial");
auto imports=JSValue::object(nullptr);own(imports,"Material",nativeMaterialConstructor());
own(imports,"Color",nativeColorConstructor());
const auto base=imports.get("Material");auto holder=JSValue::object(nullptr);own(holder,"imports",imports);
auto ctor=JSValue::builtinConstructor(+[](const JSValue& target,const Args& args,const Args& captures)->JSValue{const auto holder=captures[0];const auto parent=holder.get("constructor").getPrototype();auto self=parent.construct({},target);return PointsMaterialManaged::constructor(self,args,holder.get("imports"),JSValue{});},{holder},"PointsMaterial",1);own(holder,"constructor",ctor);ctor.setPrototype(base);auto prototype=ctor.get("prototype");prototype.setPrototype(base.get("prototype"));ctor.setFunctionSourceText(PointsMaterialSource::Class);
{auto method=JSValue::function(+[](const JSValue& self,const Args& args,const Args& captures)->JSValue{return PointsMaterialManaged::copy(self,args,captures[0],captures[1]);},{imports,prototype},"copy",1);method.setFunctionSourceText(PointsMaterialSource::copy);auto descriptor=prototype.getOwnPropertyDescriptor("copy").value_or(JSPropertyDescriptor{});descriptor.configurable=true;descriptor.value=method;descriptor.writable=true;prototype.defineProperty("copy",descriptor);}
ctor.retainForHeapLifetime();weak=JSWeakValue(ctor);return ctor;}
JSValue nativePointsMaterialModule(){using namespace PointsMaterialManaged;static thread_local JSWeakValue weak;if(const auto cached=weak.lock();cached&&cached->isObject())return *cached;auto bindings=JSValue::object(nullptr);own(bindings,"PointsMaterial",nativePointsMaterialConstructor());auto facade=JSValue::object();own(facade,"PointsMaterial",bindings.get("PointsMaterial"));bindings.retainForHeapLifetime();lexical()=JSWeakValue(bindings);facade.retainForHeapLifetime();weak=JSWeakValue(facade);return facade;}
JSValue nativePointsMaterialBinding(const std::string& name){const auto keeper=nativePointsMaterialModule();return PointsMaterialManaged::lexical().lock()->get(name);}
