#include "nodes/gpgpu/BarrierNode.h"

#include "nodes/tsl/TSLCore.h"
#include "nodes/utils/UtilityNodeRuntime.h"
#include "utils.h"

namespace {
using namespace UtilityNodeRuntime;

JSWeakValue& lexical() {
    static thread_local JSWeakValue weak;
    return weak;
}

JSValue method(const JSValue& receiver, const Args& args, const Args& captures) {
    const auto self = receiver;
    const auto name = captures[0].string();
    const auto builder = arg(args, 0);

    if (name == "isCacheable") return false;

    if (name == "setup") {
        builder.set("allowEarlyReturns", false);
        builder.set("allowGlobalVariables", false);
        return {};
    }

    if (name == "generate") {
        const auto scope = self.get("scope").toString();
        const auto barrierMethod = scope + "Barrier";
        const auto shaderStage = builder.get("shaderStage").toString();
        if (shaderStage != "compute") {
            ThreeUtils::error({"TSL: \"" + barrierMethod + "\" is not supported in the " +
                               shaderStage + " stage and can only be executed in compute."});
        }

        const auto renderer = builder.get("renderer");
        if (renderer.get("backend").get("isWebGLBackend").strictEquals(true)) {
            invoke(builder, "addFlowCode")({"\t// " + barrierMethod + "() \n"});
        } else {
            invoke(builder, "addLineFlowCode")({barrierMethod + "()", self});
        }
        return {};
    }

    throw std::logic_error("Unknown BarrierNode method: " + name);
}

JSValue makeBarrierFunction(const char* scope, const JSValue& barrier) {
    const std::string source = std::string("() => barrier( '") + scope +
                               "' ).toStack()";
    auto function = JSValue::function(
        +[](const JSValue&, const Args&, const Args& captures) -> JSValue {
            return invoke(captures[0].call({}, {captures[1]}), "toStack")();
        },
        {barrier, scope}, std::string(scope) + "Barrier", 0);
    function.setFunctionSourceText(source);
    return function;
}

} // namespace

JSValue nativeBarrierNodeModule() {
    using namespace UtilityNodeRuntime;
    static thread_local JSWeakValue weak;
    if (const auto value = weak.lock(); value && value->isObject()) return *value;
    static thread_local bool initializing = false;
    JSModuleInitializationScope scope(initializing, "BarrierNode module");
    nativeTSLCore();

    const auto constructor = makeConstructor(
        "BarrierNode", 1,
        +[](const JSValue& target, const Args& args, const Args& captures) -> JSValue {
            auto self = superConstruct(target, captures);
            self.set("scope", arg(args, 0));
            self.set("isBarrierNode", true);
            return self;
        },
        {{"isCacheable", 0}, {"setup", 1}, {"generate", 1}}, method);

    const auto proxy = nativeTSLCoreBinding("nodeProxy").call({}, {constructor});
    const auto barrier = proxy;
    auto bindings = JSValue::object(nullptr);
    own(bindings, "default", constructor);
    own(bindings, "barrier", barrier);
    own(bindings, "workgroupBarrier", makeBarrierFunction("workgroup", barrier));
    own(bindings, "storageBarrier", makeBarrierFunction("storage", barrier));
    own(bindings, "textureBarrier", makeBarrierFunction("texture", barrier));

    auto module = JSValue::object(nullptr);
    for (const auto& name : {"default", "barrier", "workgroupBarrier", "storageBarrier", "textureBarrier"})
        own(module, name, bindings.get(name));
    bindings.retainForHeapLifetime();
    lexical() = JSWeakValue(bindings);
    module.retainForHeapLifetime();
    weak = JSWeakValue(module);
    return module;
}

JSValue nativeBarrierNodeBinding(const std::string& name) {
    const auto keeper = nativeBarrierNodeModule();
    (void)keeper;
    return lexical().lock()->get(name);
}

JSValue nativeBarrierNodeConstructor() {
    return nativeBarrierNodeBinding("default");
}

