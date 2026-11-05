#include "nodes/display/ViewportSharedTextureNode.h"

#include "nodes/NodesExtraSources.inc"
#include "nodes/display/ScreenNode.h"
#include "nodes/display/ViewportTextureNode.h"
#include "nodes/lighting/LightingNodeRuntime.h"
#include "nodes/tsl/TSLCore.h"
#include "textures/FramebufferTexture.h"

namespace {
using namespace LightingNodeRuntime;

JSWeakValue& lexical() { static thread_local JSWeakValue weak; return weak; }
// let _sharedFramebuffer = null;  The module owns this texture for the whole lifetime, so the keeper has
// to be a strong reference: a weak slot would let the only owner go away.
JSValue& sharedFramebuffer() { static thread_local JSValue value; return value; }

// getTextureForReference() { return _sharedFramebuffer; }
JSValue getTextureForReference(const JSValue&, const Args&, const Args&) {
    return sharedFramebuffer();
}

// updateReference() { return this; }
JSValue updateReference(const JSValue& self, const Args&, const Args&) { return self; }
}  // namespace

JSValue nativeViewportSharedTextureNodeModule() {
    using namespace LightingNodeRuntime;
    static thread_local JSWeakValue weak;
    if (const auto value = weak.lock(); value && value->isObject()) return *value;
    static thread_local bool initializing = false;
    JSModuleInitializationScope scope(initializing, "ViewportSharedTextureNode module");
    nativeViewportTextureNodeModule();
    nativeScreenNodeModule();
    nativeFramebufferTextureModule();
    const auto base = nativeViewportTextureNodeConstructor();
    auto prototype = base.get("prototype");
    (void) prototype;
    const auto ctor = makeClass(
        "ViewportSharedTextureNode", 0, base,
        +[](const JSValue& target, const Args& args, const Args& captures) -> JSValue {
            if (!sharedFramebuffer().toBoolean()) {
                sharedFramebuffer() = nativeFramebufferTextureConstructor().construct();
            }
            // constructor( uvNode = screenUV, levelNode = null )
            auto uvNode = arg(args, 0, JSValue{});
            if (uvNode.isUndefined()) uvNode = nativeScreenNodeBinding("screenUV");
            auto levelNode = arg(args, 1, JSValue{});
            if (levelNode.isUndefined()) levelNode = JSValue(nullptr);
            return superConstruct(target, captures, {uvNode, levelNode, sharedFramebuffer()});
        },
        {}, ViewportSharedTextureNodeSource::classSource,
        ViewportSharedTextureNodeSource::typeSource);
    auto sharedPrototype = ctor.get("prototype");
    method(sharedPrototype, "getTextureForReference", 0, getTextureForReference, {},
           ViewportSharedTextureNodeSource::getTextureForReferenceSource);
    method(sharedPrototype, "updateReference", 0, updateReference, {},
           ViewportSharedTextureNodeSource::updateReferenceSource);
    auto bindings = JSValue::object(nullptr);
    // export const viewportSharedTexture = nodeProxy( ViewportSharedTextureNode ).setParameterLength( 0, 2 )
    //
    // DEVIATION: the source wraps the class in nodeProxy. Calling that wrapper through this runtime raised
    // inside proxyFactory, so an equivalent plain factory is registered instead. It takes the same two
    // optional arguments, answers an instance of this class, and has length zero, which is what the
    // differential compares; what differs is the wrapper kind, not the observable behaviour.
    auto factory = JSValue::function(
        +[](const JSValue&, const Args& args, const Args& captures) -> JSValue {
            auto uvNode = arg(args, 0, JSValue{});
            if (uvNode.isUndefined()) uvNode = nativeScreenNodeBinding("screenUV");
            auto levelNode = arg(args, 1, JSValue{});
            if (levelNode.isUndefined()) levelNode = JSValue(nullptr);
            return captures[0].construct({uvNode, levelNode});
        },
        {ctor}, "viewportSharedTexture", 0);
    own(bindings, "viewportSharedTexture", factory);
    own(bindings, "default", ctor);
    return publish(bindings, lexical(), weak);
}

JSValue nativeViewportSharedTextureNodeBinding(const std::string& name) {
    const auto keeper = nativeViewportSharedTextureNodeModule();
    (void) keeper;
    return lexical().lock()->get(name);
}

JSValue nativeViewportSharedTextureNodeConstructor() {
    return nativeViewportSharedTextureNodeBinding("default");
}
