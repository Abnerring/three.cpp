#include "materials/RawShaderMaterial.h"

RawShaderMaterial::RawShaderMaterial(const JsonValue& parameters) : ShaderMaterial(parameters) {
    type = "RawShaderMaterial";
}
void RawShaderMaterial::visitProperties(const PropertyVisitor& visitor) {
    ShaderMaterial::visitProperties(visitor);
    visitor("isRawShaderMaterial", &isRawShaderMaterial);
}
std::shared_ptr<Material> RawShaderMaterial::clone() const {
    auto result = std::make_shared<RawShaderMaterial>();
    result->copy(*this);
    return result;
}

// Managed source-class authority. Original typed implementation above remains byte-exact.
#include "core/JSModuleObject.h"
#include "materials/MaterialClassRuntime.h"
#include "materials/ShaderMaterial.h"
namespace RawShaderMaterialManaged {
using namespace MaterialClassRuntime;
#include "RawShaderMaterialManagedSource.inc"
#include "RawShaderMaterialManagedStatements.inc"
JSWeakValue& lexical(){static thread_local JSWeakValue value;return value;}
}
JSValue nativeRawShaderMaterialConstructor(){
using namespace RawShaderMaterialManaged;static thread_local JSWeakValue weak;if(const auto cached=weak.lock();cached&&cached->isConstructible())return *cached;static thread_local bool active=false;JSModuleInitializationScope scope(active,"RawShaderMaterial");
auto imports=JSValue::object(nullptr);own(imports,"ShaderMaterial",nativeShaderMaterialConstructor());
const auto base=imports.get("ShaderMaterial");auto holder=JSValue::object(nullptr);own(holder,"imports",imports);
auto ctor=JSValue::builtinConstructor(+[](const JSValue& target,const Args& args,const Args& captures)->JSValue{const auto holder=captures[0];const auto parent=holder.get("constructor").getPrototype();auto self=parent.construct({arg(args,0)},target);return RawShaderMaterialManaged::constructor(self,args,holder.get("imports"),JSValue{});},{holder},"RawShaderMaterial",1);own(holder,"constructor",ctor);ctor.setPrototype(base);auto prototype=ctor.get("prototype");prototype.setPrototype(base.get("prototype"));ctor.setFunctionSourceText(RawShaderMaterialSource::Class);
ctor.retainForHeapLifetime();weak=JSWeakValue(ctor);return ctor;}
JSValue nativeRawShaderMaterialModule(){using namespace RawShaderMaterialManaged;static thread_local JSWeakValue weak;if(const auto cached=weak.lock();cached&&cached->isObject())return *cached;auto bindings=JSValue::object(nullptr);own(bindings,"RawShaderMaterial",nativeRawShaderMaterialConstructor());auto facade=JSValue::object();own(facade,"RawShaderMaterial",bindings.get("RawShaderMaterial"));bindings.retainForHeapLifetime();lexical()=JSWeakValue(bindings);facade.retainForHeapLifetime();weak=JSWeakValue(facade);return facade;}
JSValue nativeRawShaderMaterialBinding(const std::string& name){const auto keeper=nativeRawShaderMaterialModule();return RawShaderMaterialManaged::lexical().lock()->get(name);}
