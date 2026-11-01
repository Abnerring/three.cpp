#include "materials/MeshNormalMaterial.h"

MeshNormalMaterial::MeshNormalMaterial(const JsonValue& parameters) {
    type = "MeshNormalMaterial";
    setValues(parameters);
}
void MeshNormalMaterial::visitProperties(const PropertyVisitor& visitor) {
    Material::visitProperties(visitor);
    visitor("isMeshNormalMaterial", &isMeshNormalMaterial);
    visitor("bumpMap", &bumpMap);
    visitor("bumpScale", &bumpScale);
    visitor("normalMap", &normalMap);
    visitor("normalMapType", &normalMapType);
    visitor("normalScale", &normalScale);
    visitor("displacementMap", &displacementMap);
    visitor("displacementScale", &displacementScale);
    visitor("displacementBias", &displacementBias);
    visitor("wireframe", &wireframe);
    visitor("wireframeLinewidth", &wireframeLinewidth);
    visitor("flatShading", &flatShading);
}
Material& MeshNormalMaterial::copy(const Material& source) {
    const auto& s = dynamic_cast<const MeshNormalMaterial&>(source);
    Material::copy(source);
    bumpMap = s.bumpMap;
    bumpScale = s.bumpScale;
    normalMap = s.normalMap;
    normalMapType = s.normalMapType;
    normalScale.copy( s.normalScale );
    displacementMap = s.displacementMap;
    displacementScale = s.displacementScale;
    displacementBias = s.displacementBias;
    wireframe = s.wireframe;
    wireframeLinewidth = s.wireframeLinewidth;
    flatShading = s.flatShading;
    return *this;
}
std::shared_ptr<Material> MeshNormalMaterial::clone() const {
    auto result = std::make_shared<MeshNormalMaterial>();
    result->copy(*this);
    return result;
}

// Managed source-class authority. Original typed implementation above remains byte-exact.
#include "core/JSModuleObject.h"
#include "materials/MaterialClassRuntime.h"
#include "constants.h"
#include "materials/Material.h"
#include "math/Vector2.h"
namespace MeshNormalMaterialManaged {
using namespace MaterialClassRuntime;
#include "MeshNormalMaterialManagedSource.inc"
#include "MeshNormalMaterialManagedStatements.inc"
JSWeakValue& lexical(){static thread_local JSWeakValue value;return value;}
}
JSValue nativeMeshNormalMaterialConstructor(){
using namespace MeshNormalMaterialManaged;static thread_local JSWeakValue weak;if(const auto cached=weak.lock();cached&&cached->isConstructible())return *cached;static thread_local bool active=false;JSModuleInitializationScope scope(active,"MeshNormalMaterial");
auto imports=JSValue::object(nullptr);own(imports,"TangentSpaceNormalMap",JSValue(TangentSpaceNormalMap));
own(imports,"Material",nativeMaterialConstructor());
own(imports,"Vector2",nativeVector2Constructor());
const auto base=imports.get("Material");auto holder=JSValue::object(nullptr);own(holder,"imports",imports);
auto ctor=JSValue::builtinConstructor(+[](const JSValue& target,const Args& args,const Args& captures)->JSValue{const auto holder=captures[0];const auto parent=holder.get("constructor").getPrototype();auto self=parent.construct({},target);return MeshNormalMaterialManaged::constructor(self,args,holder.get("imports"),JSValue{});},{holder},"MeshNormalMaterial",1);own(holder,"constructor",ctor);ctor.setPrototype(base);auto prototype=ctor.get("prototype");prototype.setPrototype(base.get("prototype"));ctor.setFunctionSourceText(MeshNormalMaterialSource::Class);
{auto method=JSValue::function(+[](const JSValue& self,const Args& args,const Args& captures)->JSValue{return MeshNormalMaterialManaged::copy(self,args,captures[0],captures[1]);},{imports,prototype},"copy",1);method.setFunctionSourceText(MeshNormalMaterialSource::copy);auto descriptor=prototype.getOwnPropertyDescriptor("copy").value_or(JSPropertyDescriptor{});descriptor.configurable=true;descriptor.value=method;descriptor.writable=true;prototype.defineProperty("copy",descriptor);}
ctor.retainForHeapLifetime();weak=JSWeakValue(ctor);return ctor;}
JSValue nativeMeshNormalMaterialModule(){using namespace MeshNormalMaterialManaged;static thread_local JSWeakValue weak;if(const auto cached=weak.lock();cached&&cached->isObject())return *cached;auto bindings=JSValue::object(nullptr);own(bindings,"MeshNormalMaterial",nativeMeshNormalMaterialConstructor());auto facade=JSValue::object();own(facade,"MeshNormalMaterial",bindings.get("MeshNormalMaterial"));bindings.retainForHeapLifetime();lexical()=JSWeakValue(bindings);facade.retainForHeapLifetime();weak=JSWeakValue(facade);return facade;}
JSValue nativeMeshNormalMaterialBinding(const std::string& name){const auto keeper=nativeMeshNormalMaterialModule();return MeshNormalMaterialManaged::lexical().lock()->get(name);}
