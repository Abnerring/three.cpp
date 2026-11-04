#include "nodes/lighting/BasicLightMapNode.h"

#include "nodes/lighting/LightingNode.h"
#include "nodes/lighting/LightingNodeRuntime.h"
#include "nodes/tsl/TSLBase.h"
#include "nodes/lighting/LightingEventNodeSources.inc"

namespace {

using namespace LightingNodeRuntime;

JSWeakValue& lexical() {
    static thread_local JSWeakValue weak;
    return weak;
}

JSValue setup(const JSValue& self, const Args& args, const Args&) {
    const auto builder = arg(args, 0);
    const auto reciprocalPi = nativeTSLBaseBinding("float").call({}, {1.0 / 3.141592653589793238462643383279502884});
    const auto value = invoke(self.get("lightMapNode"), "mul")({reciprocalPi});
    builder.get("context").set("irradianceLightMap", value);
    return {};
}

} // namespace

JSValue nativeBasicLightMapNodeModule() {
    static thread_local JSWeakValue weak;
    if (const auto cached = weak.lock(); cached && cached->isObject()) return *cached;

    static thread_local bool initializing = false;
    JSModuleInitializationScope scope(initializing, "BasicLightMapNode module");
    nativeLightingNodeModule();
    nativeTSLBase();

    const auto base = nativeLightingNodeBinding("default");
    auto bindings = JSValue::object(nullptr);
    const auto ctor = makeClass(
        "BasicLightMapNode", 0, base,
        +[](const JSValue& target, const Args& args, const Args& captures) -> JSValue {
            auto self = superConstruct(target, captures);
            self.set("lightMapNode", arg(args, 0, nullptr));
            return self;
        }, {}, BasicLightMapNodeSource::classSource, BasicLightMapNodeSource::typeSource);
    method(ctor.get("prototype"), "setup", 1, setup, {}, BasicLightMapNodeSource::setupSource);

    own(bindings, "default", ctor);
    return publish(bindings, lexical(), weak);
}

JSValue nativeBasicLightMapNodeBinding(const std::string& name) {
    const auto keeper = nativeBasicLightMapNodeModule();
    (void)keeper;
    return lexical().lock()->get(name);
}

JSValue nativeBasicLightMapNodeConstructor() {
    return nativeBasicLightMapNodeBinding("default");
}

BasicLightMapNode::BasicLightMapNode(const JSValue& value)
    : object_(constructor().construct({value})) {}
