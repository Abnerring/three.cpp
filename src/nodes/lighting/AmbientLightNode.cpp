#include "nodes/lighting/AmbientLightNode.h"

#include "nodes/lighting/AnalyticLightNode.h"
#include "nodes/lighting/LightingEventNodeSources.inc"
#include "nodes/lighting/LightingNodeRuntime.h"

namespace {
using namespace LightingNodeRuntime;

JSWeakValue& lexical() {
    static thread_local JSWeakValue weak;
    return weak;
}

// setup( { context } ) { context.irradiance.addAssign( this.colorNode ); }
JSValue setup(const JSValue& self, const Args& args, const Args&) {
    const auto context = arg(args, 0).get("context");
    invoke(context.get("irradiance"), "addAssign")({self.get("colorNode")});
    return {};
}
}  // namespace

JSValue nativeAmbientLightNodeModule() {
    using namespace LightingNodeRuntime;
    static thread_local JSWeakValue weak;
    if (const auto cached = weak.lock(); cached && cached->isObject()) return *cached;

    static thread_local bool initializing = false;
    JSModuleInitializationScope scope(initializing, "AmbientLightNode module");
    const auto base = nativeAnalyticLightNodeBinding("default");
    auto bindings = JSValue::object(nullptr);
    const auto ctor = makeClass(
        "AmbientLightNode", 0, base,
        +[](const JSValue& target, const Args& args, const Args& captures) -> JSValue {
            auto self = superConstruct(target, captures);
            self.set("light", arg(args, 0, nullptr));
            return self;
        },
        {}, AmbientLightNodeSource::classSource, AmbientLightNodeSource::typeSource);
    method(ctor.get("prototype"), "setup", 1, setup, {}, AmbientLightNodeSource::setupSource);
    own(bindings, "default", ctor);
    return publish(bindings, lexical(), weak);
}

JSValue nativeAmbientLightNodeBinding(const std::string& name) {
    const auto keeper = nativeAmbientLightNodeModule();
    (void) keeper;
    return lexical().lock()->get(name);
}

JSValue nativeAmbientLightNodeConstructor() { return nativeAmbientLightNodeBinding("default"); }

AmbientLightNode::AmbientLightNode(const JSValue& light) : object_(constructor().construct({light})) {}
