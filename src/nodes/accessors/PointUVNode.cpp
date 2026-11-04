#include "nodes/accessors/PointUVNode.h"

#include "nodes/core/PointUVNode.h"
#include "nodes/tsl/TSLBase.h"
#include "nodes/utils/UtilityNodeRuntime.h"
#include "PointUVNodeSource.inc"

namespace {
using namespace UtilityNodeRuntime;

JSWeakValue& lexical() {
    static thread_local JSWeakValue weak;
    return weak;
}

JSValue method(const JSValue&, const Args& args, const Args& captures) {
    if (captures[0].strictEquals("isCacheable")) return false;
    if (captures[0].strictEquals("generate")) {
        // The builder argument is intentionally ignored, matching the pinned JS method.
        (void)arg(args, 0);
        return PointUVNode::generate();
    }
    throw std::logic_error("Unknown PointUVNode method");
}
} // namespace

JSValue nativePointUVNodeModule() {
    using namespace UtilityNodeRuntime;
    static thread_local JSWeakValue weak;
    if (const auto value = weak.lock(); value && value->isObject()) return *value;

    static thread_local bool initializing = false;
    JSModuleInitializationScope scope(initializing, "PointUVNode module");
    const auto base = nativeNodeConstructor();
    const auto tsl = nativeTSLBase();
    auto ctor = makeConstructor(
        "PointUVNode", 0,
        +[](const JSValue& target, const Args&, const Args& captures) -> JSValue {
            auto self = superConstruct(target, captures, {"vec2"});
            self.set("isPointUVNode", true);
            return self;
        },
        {{"isCacheable", 0}, {"generate", 0}}, method);

    ctor.setFunctionSourceText(PointUVNodeSource::classSource);
    auto prototype = ctor.get("prototype");
    prototype.get("isCacheable").setFunctionSourceText(PointUVNodeSource::isCacheableSource);
    prototype.get("generate").setFunctionSourceText(PointUVNodeSource::generateSource);
    auto typeGetter = ctor.getOwnPropertyDescriptor("type")->getter.value();
    typeGetter.setFunctionSourceText(PointUVNodeSource::typeSource);

    auto bindings = JSValue::object(nullptr);
    own(bindings, "default", ctor);
    const auto pointUV = nativeTSLBaseBinding("nodeImmutable").call({}, {ctor});
    own(bindings, "pointUV", pointUV);

    bindings.retainForHeapLifetime();
    lexical() = JSWeakValue(bindings);
    auto module = JSValue::object(nullptr);
    own(module, "default", ctor);
    own(module, "pointUV", pointUV);
    module.retainForHeapLifetime();
    weak = JSWeakValue(module);
    (void)base;
    (void)tsl;
    return module;
}

JSValue nativePointUVNodeBinding(const std::string& name) {
    const auto keeper = nativePointUVNodeModule();
    (void)keeper;
    return lexical().lock()->get(name);
}

JSValue nativePointUVNodeConstructor() {
    return nativePointUVNodeBinding("default");
}
