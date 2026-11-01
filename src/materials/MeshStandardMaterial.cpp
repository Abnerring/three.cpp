#include "materials/MeshStandardMaterial.h"

MeshStandardMaterial::MeshStandardMaterial(const JsonValue& parameters) {
    type = "MeshStandardMaterial";
    setValues(parameters);
}
void MeshStandardMaterial::visitProperties(const PropertyVisitor& visitor) {
    Material::visitProperties(visitor);
    visitor("isMeshStandardMaterial", &isMeshStandardMaterial);
    visitor("defines", &defines);
    visitor("color", &color);
    visitor("roughness", &roughness);
    visitor("metalness", &metalness);
    visitor("map", &map);
    visitor("lightMap", &lightMap);
    visitor("lightMapIntensity", &lightMapIntensity);
    visitor("aoMap", &aoMap);
    visitor("aoMapIntensity", &aoMapIntensity);
    visitor("emissive", &emissive);
    visitor("emissiveIntensity", &emissiveIntensity);
    visitor("emissiveMap", &emissiveMap);
    visitor("bumpMap", &bumpMap);
    visitor("bumpScale", &bumpScale);
    visitor("normalMap", &normalMap);
    visitor("normalMapType", &normalMapType);
    visitor("normalScale", &normalScale);
    visitor("displacementMap", &displacementMap);
    visitor("displacementScale", &displacementScale);
    visitor("displacementBias", &displacementBias);
    visitor("roughnessMap", &roughnessMap);
    visitor("metalnessMap", &metalnessMap);
    visitor("alphaMap", &alphaMap);
    visitor("envMap", &envMap);
    visitor("envMapRotation", &envMapRotation);
    visitor("envMapIntensity", &envMapIntensity);
    visitor("wireframe", &wireframe);
    visitor("wireframeLinewidth", &wireframeLinewidth);
    visitor("wireframeLinecap", &wireframeLinecap);
    visitor("wireframeLinejoin", &wireframeLinejoin);
    visitor("flatShading", &flatShading);
    visitor("fog", &fog);
}
Material& MeshStandardMaterial::copy(const Material& source) {
    const auto& s = dynamic_cast<const MeshStandardMaterial&>(source);
    Material::copy(source);
    defines = JSValue::fromJSON(JsonValue::Object{{"STANDARD", ""}});
    color.copy( s.color );
    roughness = s.roughness;
    metalness = s.metalness;
    map = s.map;
    lightMap = s.lightMap;
    lightMapIntensity = s.lightMapIntensity;
    aoMap = s.aoMap;
    aoMapIntensity = s.aoMapIntensity;
    emissive.copy( s.emissive );
    emissiveMap = s.emissiveMap;
    emissiveIntensity = s.emissiveIntensity;
    bumpMap = s.bumpMap;
    bumpScale = s.bumpScale;
    normalMap = s.normalMap;
    normalMapType = s.normalMapType;
    normalScale.copy( s.normalScale );
    displacementMap = s.displacementMap;
    displacementScale = s.displacementScale;
    displacementBias = s.displacementBias;
    roughnessMap = s.roughnessMap;
    metalnessMap = s.metalnessMap;
    alphaMap = s.alphaMap;
    envMap = s.envMap;
    envMapRotation.copy( s.envMapRotation );
    envMapIntensity = s.envMapIntensity;
    wireframe = s.wireframe;
    wireframeLinewidth = s.wireframeLinewidth;
    wireframeLinecap = s.wireframeLinecap;
    wireframeLinejoin = s.wireframeLinejoin;
    flatShading = s.flatShading;
    fog = s.fog;
    return *this;
}
std::shared_ptr<Material> MeshStandardMaterial::clone() const {
    auto result = std::make_shared<MeshStandardMaterial>();
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
#include "math/Euler.h"
namespace MeshStandardMaterialManaged {
using namespace MaterialClassRuntime;
#include "MeshStandardMaterialManagedSource.inc"
#include "MeshStandardMaterialManagedStatements.inc"
JSWeakValue& lexical(){static thread_local JSWeakValue value;return value;}
}
JSValue nativeMeshStandardMaterialConstructor(){
using namespace MeshStandardMaterialManaged;static thread_local JSWeakValue weak;if(const auto cached=weak.lock();cached&&cached->isConstructible())return *cached;static thread_local bool active=false;JSModuleInitializationScope scope(active,"MeshStandardMaterial");
auto imports=JSValue::object(nullptr);own(imports,"TangentSpaceNormalMap",JSValue(TangentSpaceNormalMap));
own(imports,"Material",nativeMaterialConstructor());
own(imports,"Vector2",nativeVector2Constructor());
own(imports,"Color",nativeColorConstructor());
own(imports,"Euler",nativeEulerConstructor());
const auto base=imports.get("Material");auto holder=JSValue::object(nullptr);own(holder,"imports",imports);
auto ctor=JSValue::builtinConstructor(+[](const JSValue& target,const Args& args,const Args& captures)->JSValue{const auto holder=captures[0];const auto parent=holder.get("constructor").getPrototype();auto self=parent.construct({},target);return MeshStandardMaterialManaged::constructor(self,args,holder.get("imports"),JSValue{});},{holder},"MeshStandardMaterial",1);own(holder,"constructor",ctor);ctor.setPrototype(base);auto prototype=ctor.get("prototype");prototype.setPrototype(base.get("prototype"));ctor.setFunctionSourceText(MeshStandardMaterialSource::Class);
{auto method=JSValue::function(+[](const JSValue& self,const Args& args,const Args& captures)->JSValue{return MeshStandardMaterialManaged::copy(self,args,captures[0],captures[1]);},{imports,prototype},"copy",1);method.setFunctionSourceText(MeshStandardMaterialSource::copy);auto descriptor=prototype.getOwnPropertyDescriptor("copy").value_or(JSPropertyDescriptor{});descriptor.configurable=true;descriptor.value=method;descriptor.writable=true;prototype.defineProperty("copy",descriptor);}
ctor.retainForHeapLifetime();weak=JSWeakValue(ctor);return ctor;}
JSValue nativeMeshStandardMaterialModule(){using namespace MeshStandardMaterialManaged;static thread_local JSWeakValue weak;if(const auto cached=weak.lock();cached&&cached->isObject())return *cached;auto bindings=JSValue::object(nullptr);own(bindings,"MeshStandardMaterial",nativeMeshStandardMaterialConstructor());auto facade=JSValue::object();own(facade,"MeshStandardMaterial",bindings.get("MeshStandardMaterial"));bindings.retainForHeapLifetime();lexical()=JSWeakValue(bindings);facade.retainForHeapLifetime();weak=JSWeakValue(facade);return facade;}
JSValue nativeMeshStandardMaterialBinding(const std::string& name){const auto keeper=nativeMeshStandardMaterialModule();return MeshStandardMaterialManaged::lexical().lock()->get(name);}
