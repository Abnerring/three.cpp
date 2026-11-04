#include "nodes/accessors/Arrays.h"

#include "nodes/accessors/StorageBufferNode.h"
#include "nodes/core/NodeUtils.h"
#include "nodes/utils/UtilityNodeRuntime.h"
#include "renderers/common/StorageBufferAttribute.h"
#include "renderers/common/StorageInstancedBufferAttribute.h"

#include <string>

namespace {
using namespace UtilityNodeRuntime;

JSWeakValue& lexical() {
    static thread_local JSWeakValue weak;
    return weak;
}

JSValue makeArray(const JSValue&, const Args& args, const Args& captures) {
    const auto count = arg(args, 0);
    const auto type = arg(args, 1, "float");
    JSValue itemSize;
    JSValue typedArray;
    if (type.get("isStructTypeNode").strictEquals(true)) {
        itemSize = invoke(type, "getLength")();
        typedArray = nativeNodeUtilsBinding("getTypedArrayFromType").call({}, {"float"});
    } else {
        itemSize = nativeNodeUtilsBinding("getLengthFromType").call({}, {type});
        typedArray = nativeNodeUtilsBinding("getTypedArrayFromType").call({}, {type});
    }

    const auto attribute = captures[0].construct({count, itemSize, typedArray});
    const auto storage = nativeStorageBufferNodeBinding("storage");
    return storage.call({}, {attribute, type, attribute.get("count")});
}

JSValue makeInstancedArray(const JSValue&, const Args& args, const Args& captures) {
    const auto count = arg(args, 0);
    const auto type = arg(args, 1, "float");
    JSValue itemSize;
    JSValue typedArray;
    if (type.get("isStructTypeNode").strictEquals(true)) {
        itemSize = invoke(type, "getLength")();
        typedArray = nativeNodeUtilsBinding("getTypedArrayFromType").call({}, {"float"});
    } else {
        itemSize = nativeNodeUtilsBinding("getLengthFromType").call({}, {type});
        typedArray = nativeNodeUtilsBinding("getTypedArrayFromType").call({}, {type});
    }

    const auto attribute = captures[0].construct({count, itemSize, typedArray});
    const auto storage = nativeStorageBufferNodeBinding("storage");
    return storage.call({}, {attribute, type, attribute.get("count")});
}
} // namespace

JSValue nativeArraysModule() {
    static thread_local JSWeakValue weak;
    if (const auto cached = weak.lock(); cached && cached->isObject()) return *cached;
    static thread_local bool initializing = false;
    JSModuleInitializationScope scope(initializing, "Arrays module");

    nativeNodeUtils();
    nativeStorageBufferNodeModule();
    nativeStorageBufferAttributeModule();
    nativeStorageInstancedBufferAttributeModule();

    auto bindings = JSValue::object(nullptr);
    const auto attribute = nativeStorageBufferAttributeConstructor();
    const auto instanced = nativeStorageInstancedBufferAttributeConstructor();
    own(bindings, "attributeArray", JSValue::function(makeArray, {attribute}, "attributeArray", 1));
    own(bindings, "instancedArray", JSValue::function(makeInstancedArray, {instanced}, "instancedArray", 1));

    auto module = JSValue::object(nullptr);
    for (const auto& name : bindings.ownPropertyNames()) own(module, name.c_str(), bindings.get(name));
    bindings.retainForHeapLifetime();
    lexical() = JSWeakValue(bindings);
    module.retainForHeapLifetime();
    weak = JSWeakValue(module);
    return module;
}

JSValue nativeArraysBinding(const std::string& name) {
    const auto keeper = nativeArraysModule();
    (void)keeper;
    return lexical().lock()->get(name);
}

