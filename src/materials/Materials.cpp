#include "materials/Materials.h"
#include "core/JSModuleObject.h"
namespace {
JSWeakValue& lexical(){static thread_local JSWeakValue value;return value;}
void own(JSValue object,const std::string& key,const JSValue& value){JSPropertyDescriptor d;d.value=value;d.writable=d.enumerable=d.configurable=true;object.defineProperty(key,d);}
}
JSValue nativeMaterialsModule(){
 static thread_local JSWeakValue weak;if(const auto cached=weak.lock();cached&&cached->isObject())return *cached;
 static thread_local bool active=false;JSModuleInitializationScope scope(active,"Materials");
 auto bindings=JSValue::object(nullptr);
 own(bindings,"ShadowMaterial",nativeShadowMaterialConstructor());
 own(bindings,"SpriteMaterial",nativeSpriteMaterialConstructor());
 own(bindings,"RawShaderMaterial",nativeRawShaderMaterialConstructor());
 own(bindings,"ShaderMaterial",nativeShaderMaterialConstructor());
 own(bindings,"PointsMaterial",nativePointsMaterialConstructor());
 own(bindings,"MeshPhysicalMaterial",nativeMeshPhysicalMaterialConstructor());
 own(bindings,"MeshStandardMaterial",nativeMeshStandardMaterialConstructor());
 own(bindings,"MeshPhongMaterial",nativeMeshPhongMaterialConstructor());
 own(bindings,"MeshToonMaterial",nativeMeshToonMaterialConstructor());
 own(bindings,"MeshNormalMaterial",nativeMeshNormalMaterialConstructor());
 own(bindings,"MeshLambertMaterial",nativeMeshLambertMaterialConstructor());
 own(bindings,"MeshDepthMaterial",nativeMeshDepthMaterialConstructor());
 own(bindings,"MeshDistanceMaterial",nativeMeshDistanceMaterialConstructor());
 own(bindings,"MeshBasicMaterial",nativeMeshBasicMaterialConstructor());
 own(bindings,"MeshMatcapMaterial",nativeMeshMatcapMaterialConstructor());
 own(bindings,"LineDashedMaterial",nativeLineDashedMaterialConstructor());
 own(bindings,"LineBasicMaterial",nativeLineBasicMaterialConstructor());
 own(bindings,"Material",nativeMaterialConstructor());
 auto facade=JSValue::object();
 own(facade,"LineBasicMaterial",bindings.get("LineBasicMaterial"));
 own(facade,"LineDashedMaterial",bindings.get("LineDashedMaterial"));
 own(facade,"Material",bindings.get("Material"));
 own(facade,"MeshBasicMaterial",bindings.get("MeshBasicMaterial"));
 own(facade,"MeshDepthMaterial",bindings.get("MeshDepthMaterial"));
 own(facade,"MeshDistanceMaterial",bindings.get("MeshDistanceMaterial"));
 own(facade,"MeshLambertMaterial",bindings.get("MeshLambertMaterial"));
 own(facade,"MeshMatcapMaterial",bindings.get("MeshMatcapMaterial"));
 own(facade,"MeshNormalMaterial",bindings.get("MeshNormalMaterial"));
 own(facade,"MeshPhongMaterial",bindings.get("MeshPhongMaterial"));
 own(facade,"MeshPhysicalMaterial",bindings.get("MeshPhysicalMaterial"));
 own(facade,"MeshStandardMaterial",bindings.get("MeshStandardMaterial"));
 own(facade,"MeshToonMaterial",bindings.get("MeshToonMaterial"));
 own(facade,"PointsMaterial",bindings.get("PointsMaterial"));
 own(facade,"RawShaderMaterial",bindings.get("RawShaderMaterial"));
 own(facade,"ShaderMaterial",bindings.get("ShaderMaterial"));
 own(facade,"ShadowMaterial",bindings.get("ShadowMaterial"));
 own(facade,"SpriteMaterial",bindings.get("SpriteMaterial"));
 bindings.retainForHeapLifetime();lexical()=JSWeakValue(bindings);facade.retainForHeapLifetime();weak=JSWeakValue(facade);return facade;
}
JSValue nativeMaterialsBinding(const std::string& name){const auto keeper=nativeMaterialsModule();return lexical().lock()->get(name);}
