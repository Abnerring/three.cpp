#include "nodes/core/constants.h"
#include "core/JSModuleObject.h"

namespace {
void define(JSValue object, const JSValue& key, const JSValue& value) {
    JSPropertyDescriptor descriptor;
    descriptor.value = value;
    descriptor.writable = descriptor.enumerable = descriptor.configurable = true;
    object.defineProperty(key, descriptor);
}
JSValue dictionary(std::initializer_list<std::pair<const char*, const char*>> fields) {
    auto object = JSValue::object();
    for (const auto& [key, value] : fields) define(object, key, value);
    return object;
}
JSValue array(std::initializer_list<const char*> values) {
    auto result = JSValue::array();
    for (const auto* value : values) define(result, std::to_string(result.length()), value);
    return result;
}
}
JSValue nativeNodeConstants() {
    static thread_local JSWeakValue weak;
    if (const auto module = weak.lock(); module && module->isObject()) return *module;
    auto module = JSValue::object();
    define(module, "NodeShaderStage", dictionary({{"VERTEX", "vertex"}, {"FRAGMENT", "fragment"}}));
    define(module, "NodeUpdateType", dictionary({{"NONE", "none"}, {"FRAME", "frame"}, {"RENDER", "render"}, {"OBJECT", "object"}}));
    define(module, "NodeType", dictionary({{"BOOLEAN", "bool"}, {"INTEGER", "int"}, {"FLOAT", "float"}, {"VECTOR2", "vec2"},
        {"VECTOR3", "vec3"}, {"VECTOR4", "vec4"}, {"MATRIX2", "mat2"}, {"MATRIX3", "mat3"}, {"MATRIX4", "mat4"}}));
    define(module, "NodeAccess", dictionary({{"READ_ONLY", "readOnly"}, {"WRITE_ONLY", "writeOnly"}, {"READ_WRITE", "readWrite"}}));
    define(module, "defaultShaderStages", array({"fragment", "vertex"}));
    define(module, "defaultBuildStages", array({"setup", "analyze", "generate"}));
    define(module, "shaderStages", array({"fragment", "vertex", "compute"}));
    define(module, "vectorComponents", array({"x", "y", "z", "w"}));
    module.retainForHeapLifetime(); weak = JSWeakValue(module); return module;
}
