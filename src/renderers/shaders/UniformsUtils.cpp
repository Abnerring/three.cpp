#include "renderers/shaders/UniformsUtils.h"
#include "math/ColorManagement.h"
#include "utils.h"
#include "core/JSModuleObject.h"
#include <unordered_set>

namespace {
JSValue enumerableObject(const JSValue& value) {
    if (value.isObject()) return value;
    if (value.isNull() || value.isUndefined()) return JSValue::object(nullptr);
    auto result = JSValue::object();
    if (value.type() == JSValue::Type::String) {
        const auto length = value.get("length").toNumber();
        for (double index = 0; index < length; ++index) {
            const auto key = JSValue(index).toString();
            JSPropertyDescriptor descriptor; descriptor.value = value.get(key);
            descriptor.enumerable = true; descriptor.writable = false; descriptor.configurable = false;
            result.defineProperty(key,descriptor);
        }
        JSPropertyDescriptor descriptor; descriptor.value = length;
        descriptor.enumerable = false; descriptor.writable = false; descriptor.configurable = false;
        result.defineProperty("length",descriptor);
    }
    return result;
}
std::vector<std::string> enumerableKeys(JSValue object) {
    std::vector<std::string> keys;
    std::unordered_set<std::string> seen;
    if (!object.isObject()) return keys;
    for (auto current = object; current.isObject(); current = current.getPrototype()) {
        for (const auto& key : current.ownPropertyNames()) {
            if (!seen.insert(key).second) continue;
            const auto descriptor = current.getOwnPropertyDescriptor(key);
            if (descriptor && descriptor->enumerable.value_or(false)) keys.push_back(key);
        }
    }
    return keys;
}
bool isThreeObject(const JSValue& value) {
    if (!value.toBoolean()) return false;
    for (const auto* flag : {"isColor", "isMatrix3", "isMatrix4", "isVector2", "isVector3", "isVector4", "isTexture", "isQuaternion"})
        if (value.get(flag).toBoolean()) return true;
    return false;
}
JSValue cloneValue(const JSValue& value) {
    return value.get("clone").call(value);
}
}

JSValue cloneUniforms(const JSValue& source) {
    auto result = JSValue::object();
    const auto object = enumerableObject(source);
    for (const auto& name : enumerableKeys(object)) {
        if (!object.has(name)) continue;
        result.set(name, JSValue::object());
        const auto properties = enumerableObject(object.get(name,source));
        for (const auto& key : enumerableKeys(properties)) {
            if (!properties.has(key)) continue;
            const auto entry = object.get(name,source);
            const auto property = entry.isNull() || entry.isUndefined() ? entry.get(key) : enumerableObject(entry).get(key,entry);
            JSValue copy;
            // Assignment evaluates dst[u] before the clone/slice RHS. The
            // separate array-of-math cloning loop finishes before its assignment.
            JSValue target;
            if (isThreeObject(property)) {
                if (property.get("isRenderTargetTexture").toBoolean()) {
                    ThreeUtils::warn({"UniformsUtils: Textures of render targets cannot be cloned via cloneUniforms() or mergeUniforms()."});
                    target = result.get(name);
                    copy = nullptr;
                } else {
                    target = result.get(name);
                    copy = cloneValue(property);
                }
            } else if (property.type() == JSValue::Type::Array) {
                if (isThreeObject(property.get("0"))) {
                    copy = JSValue::array();
                    const auto length = property.get("length").toNumber();
                    for (double i = 0; i < length; ++i) copy.set(JSValue(i).toString(), cloneValue(property.get(JSValue(i).toString())));
                    target = result.get(name);
                } else {
                    target = result.get(name);
                    copy = property.get("slice").call(property);
                }
            } else {
                target = result.get(name);
                copy = property;
            }
            target.set(key, copy);
        }
    }
    return result;
}
JSValue cloneUniformsGroups(const JSValue& source) {
    auto result = JSValue::array();
    for (double i = 0; i < source.get("length").toNumber(); ++i)
        result.get("push").call(result, {cloneValue(source.get(JSValue(i).toString()))});
    return result;
}

JSValue mergeUniforms(const JSValue& uniforms) {
    auto merged = JSValue::object();
    for (double i = 0; i < uniforms.get("length").toNumber(); ++i) {
        const auto temporary = cloneUniforms(uniforms.get(JSValue(i).toString()));
        for (const auto& key : enumerableKeys(temporary)) {
            if (temporary.has(key)) merged.set(key,temporary.get(key));
        }
    }
    return merged;
}
JSValue getUnlitUniformColorSpace(const JSValue& renderer) {
    const auto target = renderer.get("getRenderTarget").call(renderer);
    if (target.isNull()) return renderer.get("outputColorSpace");
    if (target.get("isXRRenderTarget").strictEquals(true)) return target.get("texture").get("colorSpace");
    return nativeColorManagement().get("workingColorSpace");
}


namespace {
using Args = std::vector<JSValue>;
const JSValue& argument(const Args& args, std::size_t index) {
    static const JSValue undefined;
    return index < args.size() ? args[index] : undefined;
}
JSValue exportedFunction(const JSValue&, const Args& args, const Args& captures) {
    const auto& name = captures[1].string();
    if (name == "cloneUniforms")
        return cloneUniforms(argument(args, 0));
    if (name == "mergeUniforms")
        return mergeUniforms(argument(args, 0));
    if (name == "cloneUniformsGroups")
        return cloneUniformsGroups(argument(args, 0));
    return getUnlitUniformColorSpace(argument(args, 0));
}
JSPropertyDescriptor literal(JSValue value) {
    JSPropertyDescriptor descriptor;
    descriptor.value = std::move(value);
    descriptor.writable = descriptor.enumerable = descriptor.configurable = true;
    return descriptor;
}
JSValue exports() {
    static thread_local JSWeakValue cached;
    if (const auto value = cached.lock(); value && value->isObject())
        return *value;
    auto environment = JSValue::object(nullptr);
    for (const auto* name :
         {"cloneUniforms", "mergeUniforms", "cloneUniformsGroups", "getUnlitUniformColorSpace"}) {
        const auto fn = JSValue::constructor(exportedFunction, {environment, name}, name, 1,
                                             JSValue::ConstructorKind::Callable);
        environment.defineProperty(name, literal(fn));
    }
    auto legacy = JSValue::object();
    // The source export is a literal: inherited setters do not receive its
    // initial clone/merge fields. Later ordinary assignments are observable.
    legacy.defineProperty("clone", literal(environment.get("cloneUniforms")));
    legacy.defineProperty("merge", literal(environment.get("mergeUniforms")));
    environment.defineProperty("UniformsUtils", literal(legacy));
    environment.retainForHeapLifetime();
    cached = JSWeakValue(environment);
    return environment;
}
} // namespace
JSValue nativeUniformsUtils() {
    return exports().get("UniformsUtils");
}
JSValue nativeCloneUniforms() {
    return exports().get("cloneUniforms");
}
JSValue nativeMergeUniforms() {
    return exports().get("mergeUniforms");
}
JSValue nativeCloneUniformsGroups() {
    return exports().get("cloneUniformsGroups");
}
JSValue nativeGetUnlitUniformColorSpace() {
    return exports().get("getUnlitUniformColorSpace");
}
