#include "materials/MeshDistanceMaterial.h"

MeshDistanceMaterial::MeshDistanceMaterial(const JsonValue& parameters) {
    type = "MeshDistanceMaterial";
    setValues(parameters);
}
void MeshDistanceMaterial::visitProperties(const PropertyVisitor& visitor) {
    Material::visitProperties(visitor);
    visitor("isMeshDistanceMaterial", &isMeshDistanceMaterial);
    visitor("map", &map);
    visitor("alphaMap", &alphaMap);
    visitor("displacementMap", &displacementMap);
    visitor("displacementScale", &displacementScale);
    visitor("displacementBias", &displacementBias);
}
Material& MeshDistanceMaterial::copy(const Material& source) {
    const auto& s = dynamic_cast<const MeshDistanceMaterial&>(source);
    Material::copy(source);
    map = s.map;
    alphaMap = s.alphaMap;
    displacementMap = s.displacementMap;
    displacementScale = s.displacementScale;
    displacementBias = s.displacementBias;
    return *this;
}
std::shared_ptr<Material> MeshDistanceMaterial::clone() const {
    auto result = std::make_shared<MeshDistanceMaterial>();
    result->copy(*this);
    return result;
}

// Managed source-class authority. Original typed implementation above remains byte-exact.
#include "core/JSModuleObject.h"
#include "materials/MaterialClassRuntime.h"
#include "materials/Material.h"
namespace MeshDistanceMaterialManaged {
using namespace MaterialClassRuntime;
#include "MeshDistanceMaterialManagedSource.inc"
#include "MeshDistanceMaterialManagedStatements.inc"
JSWeakValue& lexical(){static thread_local JSWeakValue value;return value;}
}
JSValue nativeMeshDistanceMaterialConstructor(){
using namespace MeshDistanceMaterialManaged;static thread_local JSWeakValue weak;if(const auto cached=weak.lock();cached&&cached->isConstructible())return *cached;static thread_local bool active=false;JSModuleInitializationScope scope(active,"MeshDistanceMaterial");
auto imports=JSValue::object(nullptr);own(imports,"Material",nativeMaterialConstructor());
const auto base=imports.get("Material");auto holder=JSValue::object(nullptr);own(holder,"imports",imports);
auto ctor=JSValue::builtinConstructor(+[](const JSValue& target,const Args& args,const Args& captures)->JSValue{const auto holder=captures[0];const auto parent=holder.get("constructor").getPrototype();auto self=parent.construct({},target);return MeshDistanceMaterialManaged::constructor(self,args,holder.get("imports"),JSValue{});},{holder},"MeshDistanceMaterial",1);own(holder,"constructor",ctor);ctor.setPrototype(base);auto prototype=ctor.get("prototype");prototype.setPrototype(base.get("prototype"));ctor.setFunctionSourceText(MeshDistanceMaterialSource::Class);
{auto method=JSValue::function(+[](const JSValue& self,const Args& args,const Args& captures)->JSValue{return MeshDistanceMaterialManaged::copy(self,args,captures[0],captures[1]);},{imports,prototype},"copy",1);method.setFunctionSourceText(MeshDistanceMaterialSource::copy);auto descriptor=prototype.getOwnPropertyDescriptor("copy").value_or(JSPropertyDescriptor{});descriptor.configurable=true;descriptor.value=method;descriptor.writable=true;prototype.defineProperty("copy",descriptor);}
ctor.retainForHeapLifetime();weak=JSWeakValue(ctor);return ctor;}
JSValue nativeMeshDistanceMaterialModule(){using namespace MeshDistanceMaterialManaged;static thread_local JSWeakValue weak;if(const auto cached=weak.lock();cached&&cached->isObject())return *cached;auto bindings=JSValue::object(nullptr);own(bindings,"MeshDistanceMaterial",nativeMeshDistanceMaterialConstructor());auto facade=JSValue::object();own(facade,"MeshDistanceMaterial",bindings.get("MeshDistanceMaterial"));bindings.retainForHeapLifetime();lexical()=JSWeakValue(bindings);facade.retainForHeapLifetime();weak=JSWeakValue(facade);return facade;}
JSValue nativeMeshDistanceMaterialBinding(const std::string& name){const auto keeper=nativeMeshDistanceMaterialModule();return MeshDistanceMaterialManaged::lexical().lock()->get(name);}
