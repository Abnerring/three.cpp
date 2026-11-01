#include "materials/MeshToonMaterial.h"

MeshToonMaterial::MeshToonMaterial(const JsonValue& parameters) {
    type = "MeshToonMaterial";
    setValues(parameters);
}
void MeshToonMaterial::visitProperties(const PropertyVisitor& visitor) {
    Material::visitProperties(visitor);
    visitor("isMeshToonMaterial", &isMeshToonMaterial);
    visitor("defines", &defines);
    visitor("color", &color);
    visitor("map", &map);
    visitor("gradientMap", &gradientMap);
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
    visitor("alphaMap", &alphaMap);
    visitor("wireframe", &wireframe);
    visitor("wireframeLinewidth", &wireframeLinewidth);
    visitor("wireframeLinecap", &wireframeLinecap);
    visitor("wireframeLinejoin", &wireframeLinejoin);
    visitor("fog", &fog);
}
Material& MeshToonMaterial::copy(const Material& source) {
    const auto& s = dynamic_cast<const MeshToonMaterial&>(source);
    Material::copy(source);
    color.copy( s.color );
    map = s.map;
    gradientMap = s.gradientMap;
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
    alphaMap = s.alphaMap;
    wireframe = s.wireframe;
    wireframeLinewidth = s.wireframeLinewidth;
    wireframeLinecap = s.wireframeLinecap;
    wireframeLinejoin = s.wireframeLinejoin;
    fog = s.fog;
    return *this;
}
std::shared_ptr<Material> MeshToonMaterial::clone() const {
    auto result = std::make_shared<MeshToonMaterial>();
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
namespace MeshToonMaterialManaged {
using namespace MaterialClassRuntime;
#include "MeshToonMaterialManagedSource.inc"
#include "MeshToonMaterialManagedStatements.inc"
JSWeakValue& lexical(){static thread_local JSWeakValue value;return value;}
}
JSValue nativeMeshToonMaterialConstructor(){
using namespace MeshToonMaterialManaged;static thread_local JSWeakValue weak;if(const auto cached=weak.lock();cached&&cached->isConstructible())return *cached;static thread_local bool active=false;JSModuleInitializationScope scope(active,"MeshToonMaterial");
auto imports=JSValue::object(nullptr);own(imports,"TangentSpaceNormalMap",JSValue(TangentSpaceNormalMap));
own(imports,"Material",nativeMaterialConstructor());
own(imports,"Vector2",nativeVector2Constructor());
own(imports,"Color",nativeColorConstructor());
const auto base=imports.get("Material");auto holder=JSValue::object(nullptr);own(holder,"imports",imports);
auto ctor=JSValue::builtinConstructor(+[](const JSValue& target,const Args& args,const Args& captures)->JSValue{const auto holder=captures[0];const auto parent=holder.get("constructor").getPrototype();auto self=parent.construct({},target);return MeshToonMaterialManaged::constructor(self,args,holder.get("imports"),JSValue{});},{holder},"MeshToonMaterial",1);own(holder,"constructor",ctor);ctor.setPrototype(base);auto prototype=ctor.get("prototype");prototype.setPrototype(base.get("prototype"));ctor.setFunctionSourceText(MeshToonMaterialSource::Class);
{auto method=JSValue::function(+[](const JSValue& self,const Args& args,const Args& captures)->JSValue{return MeshToonMaterialManaged::copy(self,args,captures[0],captures[1]);},{imports,prototype},"copy",1);method.setFunctionSourceText(MeshToonMaterialSource::copy);auto descriptor=prototype.getOwnPropertyDescriptor("copy").value_or(JSPropertyDescriptor{});descriptor.configurable=true;descriptor.value=method;descriptor.writable=true;prototype.defineProperty("copy",descriptor);}
ctor.retainForHeapLifetime();weak=JSWeakValue(ctor);return ctor;}
JSValue nativeMeshToonMaterialModule(){using namespace MeshToonMaterialManaged;static thread_local JSWeakValue weak;if(const auto cached=weak.lock();cached&&cached->isObject())return *cached;auto bindings=JSValue::object(nullptr);own(bindings,"MeshToonMaterial",nativeMeshToonMaterialConstructor());auto facade=JSValue::object();own(facade,"MeshToonMaterial",bindings.get("MeshToonMaterial"));bindings.retainForHeapLifetime();lexical()=JSWeakValue(bindings);facade.retainForHeapLifetime();weak=JSWeakValue(facade);return facade;}
JSValue nativeMeshToonMaterialBinding(const std::string& name){const auto keeper=nativeMeshToonMaterialModule();return MeshToonMaterialManaged::lexical().lock()->get(name);}
