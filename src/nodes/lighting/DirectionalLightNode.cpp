#include "nodes/lighting/DirectionalLightNode.h"

#include "nodes/accessors/Lights.h"
#include "nodes/lighting/AnalyticLightNode.h"
#include "nodes/lighting/LightingEventNodeSources.inc"
#include "nodes/lighting/LightingNodeRuntime.h"

namespace {
using namespace LightingNodeRuntime;

JSWeakValue& lexical() {
    static thread_local JSWeakValue weak;
    return weak;
}

// setupDirect() {
//
//     const lightColor = this.colorNode;
//     const lightDirection = lightTargetDirection( this.light );
//
//     return { lightDirection, lightColor };
//
// }
JSValue setupDirect(const JSValue& self, const Args&, const Args&) {
    // The source reads the colour first and the direction second; keep that order because the
    // direction helper allocates nodes and node identity is observable.
    const auto lightColor = self.get("colorNode");
    const auto lightDirection =
        nativeLightsBinding("lightTargetDirection").call({}, {self.get("light")});
    // The returned object literal lists lightDirection before lightColor.
    auto result = JSValue::object(nullptr);
    result.set("lightDirection", lightDirection);
    result.set("lightColor", lightColor);
    return result;
}
}  // namespace

JSValue nativeDirectionalLightNodeModule() {
    using namespace LightingNodeRuntime;
    static thread_local JSWeakValue weak;
    if (const auto cached = weak.lock(); cached && cached->isObject()) return *cached;

    static thread_local bool initializing = false;
    JSModuleInitializationScope scope(initializing, "DirectionalLightNode module");
    nativeLightsModule();
    const auto base = nativeAnalyticLightNodeBinding("default");
    auto bindings = JSValue::object(nullptr);
    const auto ctor = makeClass(
        "DirectionalLightNode", 0, base,
        +[](const JSValue& target, const Args& args, const Args& captures) -> JSValue {
            auto self = superConstruct(target, captures);
            self.set("light", arg(args, 0, nullptr));
            return self;
        },
        {}, DirectionalLightNodeSource::classSource, DirectionalLightNodeSource::typeSource);
    method(ctor.get("prototype"), "setupDirect", 0, setupDirect, {},
           DirectionalLightNodeSource::setupDirectSource);
    own(bindings, "default", ctor);
    return publish(bindings, lexical(), weak);
}

JSValue nativeDirectionalLightNodeBinding(const std::string& name) {
    const auto keeper = nativeDirectionalLightNodeModule();
    (void) keeper;
    return lexical().lock()->get(name);
}

JSValue nativeDirectionalLightNodeConstructor() {
    return nativeDirectionalLightNodeBinding("default");
}

DirectionalLightNode::DirectionalLightNode(const JSValue& light)
    : object_(constructor().construct({light})) {}
