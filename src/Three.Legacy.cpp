#include "Three.Legacy.h"

#include "core/JSSourceTrace.h"

#include <string>

namespace {
JSWeakValue& lexical() { static thread_local JSWeakValue weak; return weak; }
}  // namespace

JSValue nativeThreeLegacyModule() {
    static thread_local JSWeakValue weak;
    if (const auto value = weak.lock(); value && value->isObject()) return *value;

    // The pinned source is empty, so the module deliberately exports nothing.
    auto module = JSValue::object();
    auto bindings = JSValue::object(nullptr);
    module.retainForHeapLifetime();
    bindings.retainForHeapLifetime();
    lexical() = JSWeakValue(bindings);
    weak = JSWeakValue(module);
    return module;
}
