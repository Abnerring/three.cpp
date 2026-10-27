#include "constants.h"
#include "core/JSModuleObject.h"

namespace {
void own(JSValue object,const char* key,const JSValue& value){
    JSPropertyDescriptor d;d.value=value;d.writable=d.enumerable=d.configurable=true;object.defineProperty(key,d);
}
JSWeakValue& lexical(){static thread_local JSWeakValue weak;return weak;}
}
JSValue nativeConstantsModule(){
    static thread_local JSWeakValue weak;
    if(const auto value=weak.lock();value&&value->isObject())return *value;
    auto bindings=JSValue::object(nullptr);
#include "ConstantsRuntimeData.inc"
    auto module=JSValue::object();
    for(const auto& name:bindings.ownPropertyNames())own(module,name.c_str(),bindings.get(name));
    bindings.retainForHeapLifetime();lexical()=JSWeakValue(bindings);
    module.retainForHeapLifetime();weak=JSWeakValue(module);return module;
}
JSValue nativeConstantsBinding(const std::string& name){
    const auto keeper=nativeConstantsModule();return lexical().lock()->get(name);
}
