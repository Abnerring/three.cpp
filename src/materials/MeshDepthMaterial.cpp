#include "materials/MeshDepthMaterial.h"

MeshDepthMaterial::MeshDepthMaterial(const JsonValue& parameters) {
    type = "MeshDepthMaterial";
    setValues(parameters);
}
void MeshDepthMaterial::visitProperties(const PropertyVisitor& visitor) {
    Material::visitProperties(visitor);
    visitor("isMeshDepthMaterial", &isMeshDepthMaterial);
    visitor("depthPacking", &depthPacking);
    visitor("map", &map);
    visitor("alphaMap", &alphaMap);
    visitor("displacementMap", &displacementMap);
    visitor("displacementScale", &displacementScale);
    visitor("displacementBias", &displacementBias);
    visitor("wireframe", &wireframe);
    visitor("wireframeLinewidth", &wireframeLinewidth);
}
Material& MeshDepthMaterial::copy(const Material& source) {
    const auto& s = dynamic_cast<const MeshDepthMaterial&>(source);
    Material::copy(source);
    depthPacking = s.depthPacking;
    map = s.map;
    alphaMap = s.alphaMap;
    displacementMap = s.displacementMap;
    displacementScale = s.displacementScale;
    displacementBias = s.displacementBias;
    wireframe = s.wireframe;
    wireframeLinewidth = s.wireframeLinewidth;
    return *this;
}
std::shared_ptr<Material> MeshDepthMaterial::clone() const {
    auto result = std::make_shared<MeshDepthMaterial>();
    result->copy(*this);
    return result;
}

// Managed source-class authority. Original typed implementation above remains byte-exact.
#include "core/JSModuleObject.h"
#include "materials/MaterialClassRuntime.h"
#include "materials/Material.h"
#include "constants.h"
namespace MeshDepthMaterialManaged {
using namespace MaterialClassRuntime;
#include "MeshDepthMaterialManagedSource.inc"
#include "MeshDepthMaterialManagedStatements.inc"
JSWeakValue& lexical(){static thread_local JSWeakValue value;return value;}
}
JSValue nativeMeshDepthMaterialConstructor(){
using namespace MeshDepthMaterialManaged;static thread_local JSWeakValue weak;if(const auto cached=weak.lock();cached&&cached->isConstructible())return *cached;static thread_local bool active=false;JSModuleInitializationScope scope(active,"MeshDepthMaterial");
auto imports=JSValue::object(nullptr);own(imports,"Material",nativeMaterialConstructor());
own(imports,"BasicDepthPacking",JSValue(BasicDepthPacking));
const auto base=imports.get("Material");auto holder=JSValue::object(nullptr);own(holder,"imports",imports);
auto ctor=JSValue::builtinConstructor(+[](const JSValue& target,const Args& args,const Args& captures)->JSValue{const auto holder=captures[0];const auto parent=holder.get("constructor").getPrototype();auto self=parent.construct({},target);return MeshDepthMaterialManaged::constructor(self,args,holder.get("imports"),JSValue{});},{holder},"MeshDepthMaterial",1);own(holder,"constructor",ctor);ctor.setPrototype(base);auto prototype=ctor.get("prototype");prototype.setPrototype(base.get("prototype"));ctor.setFunctionSourceText(MeshDepthMaterialSource::Class);
{auto method=JSValue::function(+[](const JSValue& self,const Args& args,const Args& captures)->JSValue{return MeshDepthMaterialManaged::copy(self,args,captures[0],captures[1]);},{imports,prototype},"copy",1);method.setFunctionSourceText(MeshDepthMaterialSource::copy);auto descriptor=prototype.getOwnPropertyDescriptor("copy").value_or(JSPropertyDescriptor{});descriptor.configurable=true;descriptor.value=method;descriptor.writable=true;prototype.defineProperty("copy",descriptor);}
ctor.retainForHeapLifetime();weak=JSWeakValue(ctor);return ctor;}
JSValue nativeMeshDepthMaterialModule(){using namespace MeshDepthMaterialManaged;static thread_local JSWeakValue weak;if(const auto cached=weak.lock();cached&&cached->isObject())return *cached;auto bindings=JSValue::object(nullptr);own(bindings,"MeshDepthMaterial",nativeMeshDepthMaterialConstructor());auto facade=JSValue::object();own(facade,"MeshDepthMaterial",bindings.get("MeshDepthMaterial"));bindings.retainForHeapLifetime();lexical()=JSWeakValue(bindings);facade.retainForHeapLifetime();weak=JSWeakValue(facade);return facade;}
JSValue nativeMeshDepthMaterialBinding(const std::string& name){const auto keeper=nativeMeshDepthMaterialModule();return MeshDepthMaterialManaged::lexical().lock()->get(name);}
