#include "nodes/utils/SampleNode.h"

#include "nodes/NodesExtraSources.inc"
#include "nodes/accessors/UV.h"
#include "nodes/core/Node.h"
#include "nodes/lighting/LightingNodeRuntime.h"
#include "nodes/tsl/TSLCore.h"

namespace {
using namespace LightingNodeRuntime;

JSWeakValue& lexical() { static thread_local JSWeakValue weak; return weak; }

// isCacheable( /*builder*/ ) { return false; }
JSValue isCacheable(const JSValue&, const Args&, const Args&) { return JSValue(false); }

// sample( uv ) { return this.callback( uv ); }
JSValue sample(const JSValue& self, const Args& args, const Args&) {
    return invoke(self.get("callback"), "call")({self, arg(args, 0, nullptr)});
}

// setup() { return this.sample( uv() ); }
JSValue setup(const JSValue& self, const Args&, const Args&) {
    const auto uvNode = nativeUVBinding("uv");
    return invoke(self, "sample")({invoke(uvNode, "call")({})});
}

// export const sample = ( callback, uv = null ) => new SampleNode( callback, nodeObject( uv ) );
JSValue sampleFactory(const JSValue&, const Args& args, const Args& captures) {
    auto uvValue = arg(args, 1, JSValue{});
    if (uvValue.isUndefined()) uvValue = JSValue(nullptr);
    const auto nodeObject = captures[0];
    const auto converted = invoke(nodeObject, "call")({uvValue});
    return captures[1].construct({arg(args, 0, nullptr), converted});
}
}  // namespace

JSValue nativeSampleNodeModule() {
    using namespace LightingNodeRuntime;
    static thread_local JSWeakValue weak;
    if (const auto value = weak.lock(); value && value->isObject()) return *value;
    static thread_local bool initializing = false;
    JSModuleInitializationScope scope(initializing, "SampleNode module");
    nativeUVModule();
    nativeTSLCore();
    const auto base = nativeNodeConstructor();
    auto bindings = JSValue::object(nullptr);
    const auto ctor = makeClass(
        "SampleNode", 1, base,
        +[](const JSValue& target, const Args& args, const Args& captures) -> JSValue {
            auto self = superConstruct(target, captures, {});
            self.set("callback", arg(args, 0, nullptr));
            self.set("uvNode", arg(args, 1, JSValue(nullptr)));
            self.set("isSampleNode", true);
            return self;
        },
        {}, SampleNodeSource::classSource, SampleNodeSource::typeSource);
    auto prototype = ctor.get("prototype");
    method(prototype, "isCacheable", 1, isCacheable, {}, SampleNodeSource::isCacheableSource);
    method(prototype, "setup", 0, setup, {}, SampleNodeSource::setupSource);
    method(prototype, "sample", 1, sample, {}, SampleNodeSource::sampleSource);
    auto nodeObject = nativeTSLCoreBinding("nodeObject");
    // export const sample = ( callback, uv = null ) => ... ; only the second parameter is defaulted.
    auto factory = JSValue::function(sampleFactory, {nodeObject, ctor}, "sample", 1);
    own(bindings, "sample", factory);
    own(bindings, "default", ctor);
    return publish(bindings, lexical(), weak);
}

JSValue nativeSampleNodeBinding(const std::string& name) {
    const auto keeper = nativeSampleNodeModule();
    (void) keeper;
    return lexical().lock()->get(name);
}

JSValue nativeSampleNodeConstructor() { return nativeSampleNodeBinding("default"); }
JSValue nativeSampleFunction() { return nativeSampleNodeBinding("sample"); }
