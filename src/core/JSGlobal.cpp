#include "core/JSGlobal.h"

JSValue nativeGlobalObject(){
    static thread_local JSWeakValue weak;
    if(const auto object=weak.lock();object&&object->isObject())return *object;
    auto object=JSValue::object(nullptr);object.retainForHeapLifetime();weak=JSWeakValue(object);return object;
}
JSValue nativeGlobalBinding(const std::string& name){
    const auto global=nativeGlobalObject();
    if(!global.has(name))throw JSReferenceError(name+" is not defined");
    return global.get(name);
}
