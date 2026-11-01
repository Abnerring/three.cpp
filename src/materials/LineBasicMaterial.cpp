#include "materials/LineBasicMaterial.h"

LineBasicMaterial::LineBasicMaterial(const JsonValue& parameters) {
    type = "LineBasicMaterial";
    setValues(parameters);
}
void LineBasicMaterial::visitProperties(const PropertyVisitor& visitor) {
    Material::visitProperties(visitor);
    visitor("isLineBasicMaterial", &isLineBasicMaterial);
    visitor("color", &color);
    visitor("map", &map);
    visitor("linewidth", &linewidth);
    visitor("linecap", &linecap);
    visitor("linejoin", &linejoin);
    visitor("fog", &fog);
}
Material& LineBasicMaterial::copy(const Material& source) {
    const auto& s = dynamic_cast<const LineBasicMaterial&>(source);
    Material::copy(source);
    color.copy( s.color );
    map = s.map;
    linewidth = s.linewidth;
    linecap = s.linecap;
    linejoin = s.linejoin;
    fog = s.fog;
    return *this;
}
std::shared_ptr<Material> LineBasicMaterial::clone() const {
    auto result = std::make_shared<LineBasicMaterial>();
    result->copy(*this);
    return result;
}

// Managed source-class authority. Original typed implementation above remains byte-exact.
#include "core/JSModuleObject.h"
#include "materials/MaterialClassRuntime.h"
#include "materials/Material.h"
#include "math/Color.h"
namespace LineBasicMaterialManaged {
using namespace MaterialClassRuntime;
#include "LineBasicMaterialManagedSource.inc"
#include "LineBasicMaterialManagedStatements.inc"
JSWeakValue& lexical(){static thread_local JSWeakValue value;return value;}
}
JSValue nativeLineBasicMaterialConstructor(){
using namespace LineBasicMaterialManaged;static thread_local JSWeakValue weak;if(const auto cached=weak.lock();cached&&cached->isConstructible())return *cached;static thread_local bool active=false;JSModuleInitializationScope scope(active,"LineBasicMaterial");
auto imports=JSValue::object(nullptr);own(imports,"Material",nativeMaterialConstructor());
own(imports,"Color",nativeColorConstructor());
const auto base=imports.get("Material");auto holder=JSValue::object(nullptr);own(holder,"imports",imports);
auto ctor=JSValue::builtinConstructor(+[](const JSValue& target,const Args& args,const Args& captures)->JSValue{const auto holder=captures[0];const auto parent=holder.get("constructor").getPrototype();auto self=parent.construct({},target);return LineBasicMaterialManaged::constructor(self,args,holder.get("imports"),JSValue{});},{holder},"LineBasicMaterial",1);own(holder,"constructor",ctor);ctor.setPrototype(base);auto prototype=ctor.get("prototype");prototype.setPrototype(base.get("prototype"));ctor.setFunctionSourceText(LineBasicMaterialSource::Class);
{auto method=JSValue::function(+[](const JSValue& self,const Args& args,const Args& captures)->JSValue{return LineBasicMaterialManaged::copy(self,args,captures[0],captures[1]);},{imports,prototype},"copy",1);method.setFunctionSourceText(LineBasicMaterialSource::copy);auto descriptor=prototype.getOwnPropertyDescriptor("copy").value_or(JSPropertyDescriptor{});descriptor.configurable=true;descriptor.value=method;descriptor.writable=true;prototype.defineProperty("copy",descriptor);}
ctor.retainForHeapLifetime();weak=JSWeakValue(ctor);return ctor;}
JSValue nativeLineBasicMaterialModule(){using namespace LineBasicMaterialManaged;static thread_local JSWeakValue weak;if(const auto cached=weak.lock();cached&&cached->isObject())return *cached;auto bindings=JSValue::object(nullptr);own(bindings,"LineBasicMaterial",nativeLineBasicMaterialConstructor());auto facade=JSValue::object();own(facade,"LineBasicMaterial",bindings.get("LineBasicMaterial"));bindings.retainForHeapLifetime();lexical()=JSWeakValue(bindings);facade.retainForHeapLifetime();weak=JSWeakValue(facade);return facade;}
JSValue nativeLineBasicMaterialBinding(const std::string& name){const auto keeper=nativeLineBasicMaterialModule();return LineBasicMaterialManaged::lexical().lock()->get(name);}
