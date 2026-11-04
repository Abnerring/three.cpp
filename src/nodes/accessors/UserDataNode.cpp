#include "nodes/accessors/UserDataNode.h"
#include "nodes/accessors/ReferenceNode.h"
#include "nodes/utils/UtilityNodeRuntime.h"

namespace {
using namespace UtilityNodeRuntime;
#include "UserDataNodeSource.inc"

JSWeakValue& lexical() {
    static thread_local JSWeakValue weak;
    return weak;
}
}

JSValue nativeUserDataNodeModule() {
    using namespace UtilityNodeRuntime;
    static thread_local JSWeakValue weak;
    if (const auto module = weak.lock(); module && module->isObject()) return *module;

    static thread_local bool initializing = false;
    JSModuleInitializationScope scope(initializing, "UserDataNode module");

    const auto base = nativeReferenceNodeBinding("default");
    auto holder = JSValue::object(nullptr);
    auto bindings = JSValue::object(nullptr);

    auto ctor = JSValue::builtinConstructor(
        +[](const JSValue& target, const Args& args, const Args& captures) -> JSValue {
            const auto userData = arg(args, 2, nullptr);
            auto self = captures[0].get("constructor").getPrototype().construct(
                {arg(args, 0), arg(args, 1), userData}, target);
            self.set("userData", userData);
            self.set("isUserDataNode", true);
            return self;
        },
        {holder}, "UserDataNode", 2);
    holder.set("constructor", ctor);
    ctor.setPrototype(base);
    auto prototype = ctor.get("prototype");
    prototype.setPrototype(base.get("prototype"));
    ctor.setFunctionSourceText(UserDataNodeSource::UserDataNode);

    JSPropertyDescriptor type;
    type.getter = JSValue::function(
        +[](const JSValue&, const Args&, const Args&) -> JSValue { return "UserDataNode"; }, {},
        "get type", 0);
    type.getter->setFunctionSourceText(UserDataNodeSource::get_type);
    type.configurable = true;
    ctor.defineProperty("type", type);

    JSPropertyDescriptor update;
    update.value = JSValue::function(
        +[](const JSValue& receiver, const Args& args, const Args&) -> JSValue {
            auto self = receiver;
            const auto userData = self.get("userData");
            const auto state = arg(args, 0);
            const auto object = state.get("object");
            const auto reference = !userData.isNull() ? userData : object.get("userData");
            self.set("reference", reference);
            return reference;
        },
        {}, "updateReference", 1);
    update.value->setFunctionSourceText(UserDataNodeSource::updateReference);
    update.writable = update.configurable = true;
    prototype.defineProperty("updateReference", update);

    own(bindings, "default", ctor);
    auto factory = JSValue::function(
        +[](const JSValue&, const Args& args, const Args& captures) -> JSValue {
            return captures[0].construct({arg(args, 0), arg(args, 1), arg(args, 2, nullptr)});
        },
        {ctor}, "userData", 3);
    factory.setFunctionSourceText(UserDataNodeSource::userData);
    own(bindings, "userData", factory);

    auto module = JSValue::object();
    own(module, "default", ctor);
    own(module, "userData", factory);
    bindings.retainForHeapLifetime();
    lexical() = JSWeakValue(bindings);
    module.retainForHeapLifetime();
    weak = JSWeakValue(module);
    return module;
}

JSValue nativeUserDataNodeBinding(const std::string& name) {
    const auto keeper = nativeUserDataNodeModule();
    return lexical().lock()->get(name);
}

JSValue nativeUserDataNodeConstructor() { return nativeUserDataNodeBinding("default"); }
