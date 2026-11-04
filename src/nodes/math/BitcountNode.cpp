#include "nodes/math/BitcountNode.h"

#include "nodes/math/MathNode.h"
#include "nodes/tsl/TSLCore.h"
#include "nodes/utils/UtilityNodeRuntime.h"

namespace {
using namespace UtilityNodeRuntime;

JSWeakValue& lexical() {
    static thread_local JSWeakValue weak;
    return weak;
}

} // namespace

JSValue nativeBitcountNodeConstructor() {
    static thread_local JSWeakValue weak;
    if (const auto cached = weak.lock(); cached && cached->isConstructible()) return *cached;

    const auto base = nativeMathNodeConstructor();
    auto holder = JSValue::object(nullptr);
    auto constructor = JSValue::builtinConstructor(
        +[](const JSValue& newTarget, const Args& args, const Args& captures) -> JSValue {
            auto self = captures[0].get("constructor").getPrototype().construct(
                {arg(args, 0), arg(args, 1)}, newTarget);
            self.set("method", arg(args, 0));
            self.set("aNode", arg(args, 1));
            self.set("bNode", nullptr);
            self.set("cNode", nullptr);
            self.set("isBitcountNode", true);
            return self;
        },
        {holder}, "BitcountNode", 2);
    holder.set("constructor", constructor);
    constructor.setPrototype(base);
    auto prototype = constructor.get("prototype");
    prototype.setPrototype(base.get("prototype"));
    JSPropertyDescriptor type;
    type.getter = JSValue::function(
        +[](const JSValue&, const Args&, const Args&) -> JSValue { return "BitcountNode"; },
        {}, "get type", 0);
    type.configurable = true;
    constructor.defineProperty("type", type);
    constructor.set("COUNT_TRAILING_ZEROS", "countTrailingZeros");
    constructor.set("COUNT_LEADING_ZEROS", "countLeadingZeros");
    constructor.set("COUNT_ONE_BITS", "countOneBits");
    constructor.retainForHeapLifetime();
    weak = JSWeakValue(constructor);
    return constructor;
}

JSValue nativeBitcountNodeModule() {
    using namespace UtilityNodeRuntime;
    static thread_local JSWeakValue weak;
    if (const auto cached = weak.lock(); cached && cached->isObject()) return *cached;

    static thread_local bool initializing = false;
    JSModuleInitializationScope scope(initializing, "BitcountNode module");
    const auto constructor = nativeBitcountNodeConstructor();
    auto bindings = JSValue::object(nullptr);
    own(bindings, "default", constructor);
    for (const auto& [name, method] : {
             std::pair{"countTrailingZeros", "COUNT_TRAILING_ZEROS"},
             std::pair{"countLeadingZeros", "COUNT_LEADING_ZEROS"},
             std::pair{"countOneBits", "COUNT_ONE_BITS"}}) {
        auto proxy = nativeTSLCoreBinding("nodeProxyIntent").call(
            {}, {constructor, constructor.get(method)});
        proxy = invoke(proxy, "setParameterLength")({1});
        own(bindings, name, proxy);
    }

    auto module = JSValue::object(nullptr);
    for (const auto& name : bindings.ownPropertyNames()) own(module, name.c_str(), bindings.get(name));
    bindings.retainForHeapLifetime();
    lexical() = JSWeakValue(bindings);
    module.retainForHeapLifetime();
    weak = JSWeakValue(module);
    return module;
}

JSValue nativeBitcountNodeBinding(const std::string& name) {
    const auto keeper = nativeBitcountNodeModule();
    (void)keeper;
    return lexical().lock()->get(name);
}
