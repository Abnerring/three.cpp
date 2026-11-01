#include "materials/MeshLambertMaterial.h"

MeshLambertMaterial::MeshLambertMaterial(const JsonValue& parameters) {
    type = "MeshLambertMaterial";
    setValues(parameters);
}
void MeshLambertMaterial::visitProperties(const PropertyVisitor& visitor) {
    Material::visitProperties(visitor);
    visitor("isMeshLambertMaterial", &isMeshLambertMaterial);
    visitor("color", &color);
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
    visitor("specularMap", &specularMap);
    visitor("alphaMap", &alphaMap);
    visitor("envMap", &envMap);
    visitor("envMapRotation", &envMapRotation);
    visitor("combine", &combine);
    visitor("reflectivity", &reflectivity);
    visitor("envMapIntensity", &envMapIntensity);
    visitor("refractionRatio", &refractionRatio);
    visitor("wireframe", &wireframe);
    visitor("wireframeLinewidth", &wireframeLinewidth);
    visitor("wireframeLinecap", &wireframeLinecap);
    visitor("wireframeLinejoin", &wireframeLinejoin);
    visitor("flatShading", &flatShading);
    visitor("fog", &fog);
}
Material& MeshLambertMaterial::copy(const Material& source) {
    const auto& s = dynamic_cast<const MeshLambertMaterial&>(source);
    Material::copy(source);
    color.copy( s.color );
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
    specularMap = s.specularMap;
    alphaMap = s.alphaMap;
    envMap = s.envMap;
    envMapRotation.copy( s.envMapRotation );
    combine = s.combine;
    reflectivity = s.reflectivity;
    envMapIntensity = s.envMapIntensity;
    refractionRatio = s.refractionRatio;
    wireframe = s.wireframe;
    wireframeLinewidth = s.wireframeLinewidth;
    wireframeLinecap = s.wireframeLinecap;
    wireframeLinejoin = s.wireframeLinejoin;
    flatShading = s.flatShading;
    fog = s.fog;
    return *this;
}
std::shared_ptr<Material> MeshLambertMaterial::clone() const {
    auto result = std::make_shared<MeshLambertMaterial>();
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
namespace MeshLambertMaterialManaged {
using namespace MaterialClassRuntime;
#include "MeshLambertMaterialManagedSource.inc"
#include "MeshLambertMaterialManagedStatements.inc"
JSWeakValue& lexical(){static thread_local JSWeakValue value;return value;}
}
JSValue nativeMeshLambertMaterialConstructor(){
using namespace MeshLambertMaterialManaged;static thread_local JSWeakValue weak;if(const auto cached=weak.lock();cached&&cached->isConstructible())return *cached;static thread_local bool active=false;JSModuleInitializationScope scope(active,"MeshLambertMaterial");
auto imports=JSValue::object(nullptr);own(imports,"MultiplyOperation",JSValue(MultiplyOperation));
own(imports,"TangentSpaceNormalMap",JSValue(TangentSpaceNormalMap));
own(imports,"Material",nativeMaterialConstructor());
own(imports,"Vector2",nativeVector2Constructor());
own(imports,"Color",nativeColorConstructor());
own(imports,"Euler",nativeEulerConstructor());
const auto base=imports.get("Material");auto holder=JSValue::object(nullptr);own(holder,"imports",imports);
auto ctor=JSValue::builtinConstructor(+[](const JSValue& target,const Args& args,const Args& captures)->JSValue{const auto holder=captures[0];const auto parent=holder.get("constructor").getPrototype();auto self=parent.construct({},target);return MeshLambertMaterialManaged::constructor(self,args,holder.get("imports"),JSValue{});},{holder},"MeshLambertMaterial",1);own(holder,"constructor",ctor);ctor.setPrototype(base);auto prototype=ctor.get("prototype");prototype.setPrototype(base.get("prototype"));ctor.setFunctionSourceText(MeshLambertMaterialSource::Class);
{auto method=JSValue::function(+[](const JSValue& self,const Args& args,const Args& captures)->JSValue{return MeshLambertMaterialManaged::copy(self,args,captures[0],captures[1]);},{imports,prototype},"copy",1);method.setFunctionSourceText(MeshLambertMaterialSource::copy);auto descriptor=prototype.getOwnPropertyDescriptor("copy").value_or(JSPropertyDescriptor{});descriptor.configurable=true;descriptor.value=method;descriptor.writable=true;prototype.defineProperty("copy",descriptor);}
ctor.retainForHeapLifetime();weak=JSWeakValue(ctor);return ctor;}
JSValue nativeMeshLambertMaterialModule(){using namespace MeshLambertMaterialManaged;static thread_local JSWeakValue weak;if(const auto cached=weak.lock();cached&&cached->isObject())return *cached;auto bindings=JSValue::object(nullptr);own(bindings,"MeshLambertMaterial",nativeMeshLambertMaterialConstructor());auto facade=JSValue::object();own(facade,"MeshLambertMaterial",bindings.get("MeshLambertMaterial"));bindings.retainForHeapLifetime();lexical()=JSWeakValue(bindings);facade.retainForHeapLifetime();weak=JSWeakValue(facade);return facade;}
JSValue nativeMeshLambertMaterialBinding(const std::string& name){const auto keeper=nativeMeshLambertMaterialModule();return MeshLambertMaterialManaged::lexical().lock()->get(name);}
