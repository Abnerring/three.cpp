#include "nodes/lighting/BasicEnvironmentNode.h"

#include "nodes/accessors/MaterialProperties.h"
#include "nodes/lighting/LightingEventNodeSources.inc"
#include "nodes/lighting/LightingNode.h"
#include "nodes/lighting/LightingNodeRuntime.h"
#include "nodes/utils/CubeMapNode.h"

namespace {
using namespace LightingNodeRuntime;

JSWeakValue& lexical() {
    static thread_local JSWeakValue weak;
    return weak;
}

JSValue getUV(const JSValue&, const Args& args, const Args& captures) {
    const auto textureNode = arg(args, 0);
    auto uvNode = textureNode.get("uvNode");
    if (!uvNode.toBoolean()) uvNode = invoke(textureNode, "getDefaultUV")();
    return invoke(captures[0], "mul")({uvNode});
}

JSValue setup(const JSValue& self, const Args& args, const Args&) {
    const auto builder = arg(args, 0);
    const auto cubeMap = nativeCubeMapNodeBinding("cubeMapNode").call({}, {self.get("envNode")});
    auto options = JSValue::object(nullptr);
    auto callback = JSValue::function(getUV, {nativeMaterialPropertiesBinding("materialEnvRotation")}, "", 1);
    callback.setFunctionSourceText(
        "( textureNode ) => materialEnvRotation.mul( textureNode.uvNode || textureNode.getDefaultUV() )");
    own(options, "getUV", callback);
    own(options, "forceUVContext", true);
    builder.get("context").set("environment", invoke(cubeMap, "context")({options}));
    return {};
}
}

JSValue nativeBasicEnvironmentNodeModule() {
    using namespace LightingNodeRuntime;
    static thread_local JSWeakValue weak;
    if (const auto cached = weak.lock(); cached && cached->isObject()) return *cached;

    static thread_local bool initializing = false;
    JSModuleInitializationScope scope(initializing, "BasicEnvironmentNode module");
    nativeCubeMapNodeModule();
    nativeMaterialPropertiesModule();
    const auto base = nativeLightingNodeBinding("default");
    auto bindings = JSValue::object(nullptr);
    const auto ctor = makeClass(
        "BasicEnvironmentNode", 0, base,
        +[](const JSValue& target, const Args& args, const Args& captures) -> JSValue {
            auto self = superConstruct(target, captures);
            self.set("envNode", arg(args, 0, nullptr));
            return self;
        }, {}, BasicEnvironmentNodeSource::classSource, BasicEnvironmentNodeSource::typeSource);
    method(ctor.get("prototype"), "setup", 1, setup, {}, BasicEnvironmentNodeSource::setupSource);
    own(bindings, "default", ctor);
    return publish(bindings, lexical(), weak);
}

JSValue nativeBasicEnvironmentNodeBinding(const std::string& name) {
    const auto keeper = nativeBasicEnvironmentNodeModule();
    (void)keeper;
    return lexical().lock()->get(name);
}

JSValue nativeBasicEnvironmentNodeConstructor() {
    return nativeBasicEnvironmentNodeBinding("default");
}

BasicEnvironmentNode::BasicEnvironmentNode(const JSValue& envNode)
    : object_(constructor().construct({envNode})) {}
