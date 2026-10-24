#include "nodes/core/NodeVarying.h"
#include "nodes/core/NodeVar.h"
#include "core/JSModuleObject.h"

JSValue nativeNodeVaryingConstructor() {
    static thread_local JSWeakValue weak;
    if (auto cached = weak.lock(); cached && cached->isConstructible()) return *cached;
    static thread_local bool initializing = false;
    JSModuleInitializationScope scope(initializing, "NodeVarying constructor");
    auto constructor = JSValue::builtinConstructor([](const JSValue& newTarget,
        const std::vector<JSValue>& args, const std::vector<JSValue>& captures) {
        auto self = captures[0].construct({args.size() > 0 ? args[0] : JSValue{},
                                         args.size() > 1 ? args[1] : JSValue{}}, newTarget);
        self.set("needsInterpolation", false);
        self.set("isNodeVarying", true);
        self.set("interpolationType", args.size() > 2 && !args[2].isUndefined() ? args[2] : JSValue(nullptr));
        self.set("interpolationSampling", args.size() > 3 && !args[3].isUndefined() ? args[3] : JSValue(nullptr));
        return self;
    }, {nativeNodeVarConstructor()}, "NodeVarying", 2);
    constructor.setPrototype(nativeNodeVarConstructor());
    constructor.get("prototype").setPrototype(nativeNodeVarConstructor().get("prototype"));
    constructor.retainForHeapLifetime(); weak = JSWeakValue(constructor); return constructor;
}
