#include "core/JSFunction.h"
namespace {
using Args = std::vector<JSValue>;
JSValue empty(const JSValue&, const Args&, const Args&) { return {}; }
JSValue functionConstructor(const JSValue&, const JSValue& newTarget,
                            const Args& args, const Args&) {
    if (!args.empty()) throw JSFunctionDynamicSourceUnsupported();
    const auto target = newTarget.isUndefined() ? nativeFunctionConstructor() : newTarget;
    const auto prototype = target.getPrototypeFromConstructor(nativeFunctionConstructor().get("prototype"));
    auto result = JSValue::constructor(empty, {}, "anonymous", 0, JSValue::ConstructorKind::Callable);
    result.setPrototype(prototype);
    result.setInternalSlot("Function.languageMode", "sloppy-empty");
    result.setFunctionSourceText("function anonymous(\n) {\n\n}");
    return result;
}
}
JSValue nativeFunctionConstructor() {
    static thread_local JSWeakValue weak;
    if (const auto value = weak.lock(); value && value->isConstructible()) return *value;
    // Every native function already inherits the heap's actual Function.prototype.
    auto prototype = JSValue::function(empty).getPrototype();
    auto constructor = JSValue::callableBuiltinConstructor(functionConstructor, {}, "Function", 1, prototype);
    constructor.retainForHeapLifetime(); weak = JSWeakValue(constructor); return constructor;
}
