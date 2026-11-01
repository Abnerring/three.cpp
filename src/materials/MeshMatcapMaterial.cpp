#include "materials/MeshMatcapMaterial.h"

MeshMatcapMaterial::MeshMatcapMaterial(const JsonValue& parameters) {
    type = "MeshMatcapMaterial";
    setValues(parameters);
}
void MeshMatcapMaterial::visitProperties(const PropertyVisitor& visitor) {
    Material::visitProperties(visitor);
    visitor("isMeshMatcapMaterial", &isMeshMatcapMaterial);
    visitor("defines", &defines);
    visitor("color", &color);
    visitor("matcap", &matcap);
    visitor("map", &map);
    visitor("bumpMap", &bumpMap);
    visitor("bumpScale", &bumpScale);
    visitor("normalMap", &normalMap);
    visitor("normalMapType", &normalMapType);
    visitor("normalScale", &normalScale);
    visitor("displacementMap", &displacementMap);
    visitor("displacementScale", &displacementScale);
    visitor("displacementBias", &displacementBias);
    visitor("alphaMap", &alphaMap);
    visitor("wireframe", &wireframe);
    visitor("wireframeLinewidth", &wireframeLinewidth);
    visitor("flatShading", &flatShading);
    visitor("fog", &fog);
}
Material& MeshMatcapMaterial::copy(const Material& source) {
    const auto& s = dynamic_cast<const MeshMatcapMaterial&>(source);
    Material::copy(source);
    defines = JSValue::fromJSON(JsonValue::Object{{"MATCAP", ""}});
    color.copy( s.color );
    matcap = s.matcap;
    map = s.map;
    bumpMap = s.bumpMap;
    bumpScale = s.bumpScale;
    normalMap = s.normalMap;
    normalMapType = s.normalMapType;
    normalScale.copy( s.normalScale );
    displacementMap = s.displacementMap;
    displacementScale = s.displacementScale;
    displacementBias = s.displacementBias;
    alphaMap = s.alphaMap;
    wireframe = s.wireframe;
    wireframeLinewidth = s.wireframeLinewidth;
    flatShading = s.flatShading;
    fog = s.fog;
    return *this;
}
std::shared_ptr<Material> MeshMatcapMaterial::clone() const {
    auto result = std::make_shared<MeshMatcapMaterial>();
    result->copy(*this);
    return result;
}

// Managed source-class authority. Original typed implementation above remains byte-exact.
#include "core/JSModuleObject.h"
#include "materials/MaterialClassRuntime.h"
#include "constants.h"
#include "materials/Material.h"
#include "math/Vector2.h"
#include "math/Color.h"
namespace MeshMatcapMaterialManaged {
using namespace MaterialClassRuntime;
#include "MeshMatcapMaterialManagedSource.inc"
#include "MeshMatcapMaterialManagedStatements.inc"
JSWeakValue& lexical(){static thread_local JSWeakValue value;return value;}
}
JSValue nativeMeshMatcapMaterialConstructor(){
using namespace MeshMatcapMaterialManaged;static thread_local JSWeakValue weak;if(const auto cached=weak.lock();cached&&cached->isConstructible())return *cached;static thread_local bool active=false;JSModuleInitializationScope scope(active,"MeshMatcapMaterial");
auto imports=JSValue::object(nullptr);own(imports,"TangentSpaceNormalMap",JSValue(TangentSpaceNormalMap));
own(imports,"Material",nativeMaterialConstructor());
own(imports,"Vector2",nativeVector2Constructor());
own(imports,"Color",nativeColorConstructor());
const auto base=imports.get("Material");auto holder=JSValue::object(nullptr);own(holder,"imports",imports);
auto ctor=JSValue::builtinConstructor(+[](const JSValue& target,const Args& args,const Args& captures)->JSValue{const auto holder=captures[0];const auto parent=holder.get("constructor").getPrototype();auto self=parent.construct({},target);return MeshMatcapMaterialManaged::constructor(self,args,holder.get("imports"),JSValue{});},{holder},"MeshMatcapMaterial",1);own(holder,"constructor",ctor);ctor.setPrototype(base);auto prototype=ctor.get("prototype");prototype.setPrototype(base.get("prototype"));ctor.setFunctionSourceText(MeshMatcapMaterialSource::Class);
{auto method=JSValue::function(+[](const JSValue& self,const Args& args,const Args& captures)->JSValue{return MeshMatcapMaterialManaged::copy(self,args,captures[0],captures[1]);},{imports,prototype},"copy",1);method.setFunctionSourceText(MeshMatcapMaterialSource::copy);auto descriptor=prototype.getOwnPropertyDescriptor("copy").value_or(JSPropertyDescriptor{});descriptor.configurable=true;descriptor.value=method;descriptor.writable=true;prototype.defineProperty("copy",descriptor);}
ctor.retainForHeapLifetime();weak=JSWeakValue(ctor);return ctor;}
JSValue nativeMeshMatcapMaterialModule(){using namespace MeshMatcapMaterialManaged;static thread_local JSWeakValue weak;if(const auto cached=weak.lock();cached&&cached->isObject())return *cached;auto bindings=JSValue::object(nullptr);own(bindings,"MeshMatcapMaterial",nativeMeshMatcapMaterialConstructor());auto facade=JSValue::object();own(facade,"MeshMatcapMaterial",bindings.get("MeshMatcapMaterial"));bindings.retainForHeapLifetime();lexical()=JSWeakValue(bindings);facade.retainForHeapLifetime();weak=JSWeakValue(facade);return facade;}
JSValue nativeMeshMatcapMaterialBinding(const std::string& name){const auto keeper=nativeMeshMatcapMaterialModule();return MeshMatcapMaterialManaged::lexical().lock()->get(name);}
