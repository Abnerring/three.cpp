#include "renderers/common/Constants.h"
#include "core/JSModuleObject.h"

JSValue nativeRendererCommonConstants() {
    static thread_local JSWeakValue weak;
    if (const auto value = weak.lock(); value && value->isObject()) return *value;
    auto module = JSValue::object(), attributes = JSValue::object();
    attributes.set("VERTEX",1); attributes.set("INDEX",2); attributes.set("STORAGE",3); attributes.set("INDIRECT",4);
    module.set("AttributeType",attributes); module.set("GPU_CHUNK_BYTES",GPU_CHUNK_BYTES);
    module.retainForHeapLifetime(); weak = JSWeakValue(module); return module;
}
