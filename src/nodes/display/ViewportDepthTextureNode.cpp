#include "nodes/display/ViewportDepthTextureNode.h"
#include "nodes/display/ViewportNodeSources.inc"
#include <string_view>

ViewportDepthTextureNode::Source ViewportDepthTextureNode::sourceFor(bool textureWasGiven) {
    // A given texture is used as it stands; only an absent one falls back to the shared one.
    return textureWasGiven ? Source::Given : Source::Shared;
}

bool ViewportDepthTextureNode::sharesOneDepthTexture() {
    return true;
}

bool ViewportDepthTextureNode::overridesReferenceUpdate() {
    // The ordinary behaviour is kept, unlike the shared sibling which overrides it.
    return false;
}

#include "nodes/display/ViewportDepthTextureNode.h"

namespace {
// The one shared texture, made the first time a node asks for one.
std::uint64_t gSharedTextureId = 0;
std::uint32_t gNextId = 1;
std::uint32_t gDefaultTexturesMade = 0;
std::uint32_t gNodesServed = 0;
}

std::uint32_t ViewportDepthTexture::defaultTextureCount() {
    return gDefaultTexturesMade;
}

std::uint32_t ViewportDepthTexture::nodesServed() {
    return gNodesServed;
}

std::uint64_t ViewportDepthTexture::textureId(bool hasOwnTexture, std::uint64_t ownTextureId) {
    // A texture that was given is used as it stands.
    if (hasOwnTexture) {
        gNodesServed += 1;
        return ownTextureId;
    }
    // Otherwise the shared one, made if this is the first ask.
    gNodesServed += 1;
    if (gSharedTextureId == 0) {
        gSharedTextureId = gNextId;
        gNextId += 1;
        gDefaultTexturesMade += 1;
    }
    return gSharedTextureId;
}

void ViewportDepthTexture::reset() {
    gSharedTextureId = 0;
    gNextId = 1;
    gDefaultTexturesMade = 0;
    gNodesServed = 0;
}

#include "nodes/display/ViewportTextureNode.h"
#include "nodes/display/ScreenNode.h"
#include "nodes/tsl/TSLBase.h"
#include "nodes/utils/UtilityNodeRuntime.h"
#include "textures/DepthTexture.h"
namespace {
using namespace UtilityNodeRuntime;
JSWeakValue& depthTextureLexical(){static thread_local JSWeakValue weak;return weak;}
}
JSValue nativeViewportDepthTextureNodeModule(){
    using namespace UtilityNodeRuntime;static thread_local JSWeakValue weak;if(const auto value=weak.lock();value&&value->isObject())return *value;
    static thread_local bool initializing=false;JSModuleInitializationScope scope(initializing,"ViewportDepthTextureNode module");
    const auto viewport=nativeViewportTextureNodeModule(),tsl=nativeTSLBase(),screen=nativeScreenNodeModule(),depth=nativeDepthTextureModule();
    auto state=JSValue::object(nullptr),holder=JSValue::object(nullptr),bindings=JSValue::object(nullptr);own(state,"_sharedDepthbuffer",nullptr);
    own(state,"DepthTexture",nativeDepthTextureBinding("DepthTexture"));own(state,"screenUV",nativeScreenNodeBinding("screenUV"));
    auto ctor=JSValue::builtinConstructor(+[](const JSValue& target,const Args& args,const Args& captures)->JSValue{
        auto state=captures[0];const auto holder=captures[1],uv=arg(args,0,state.get("screenUV")),level=arg(args,1,nullptr);auto depth=arg(args,2,nullptr);
        if(depth.isNull()){if(state.get("_sharedDepthbuffer").isNull())state.set("_sharedDepthbuffer",state.get("DepthTexture").construct());depth=state.get("_sharedDepthbuffer");}
        return holder.get("constructor").getPrototype().construct({uv,level,depth},target);
    },{state,holder},"ViewportDepthTextureNode",0);
    ctor.setFunctionSourceText(ViewportDepthTextureNodeSource::classSource);holder.set("constructor",ctor);ctor.setPrototype(nativeViewportTextureNodeBinding("default"));auto prototype=ctor.get("prototype");prototype.setPrototype(ctor.getPrototype().get("prototype"));
    JSPropertyDescriptor type;type.getter=JSValue::function(+[](const JSValue&,const Args&,const Args&)->JSValue{return "ViewportDepthTextureNode";},{},"get type",0);type.getter->setFunctionSourceText(ViewportDepthTextureNodeSource::typeSource);type.configurable=true;ctor.defineProperty("type",type);
    own(bindings,"default",ctor);own(bindings,"viewportDepthTexture",invoke(nativeTSLBaseBinding("nodeProxy").call({}, {ctor}),"setParameterLength")({0,3}));
    auto module=JSValue::object();for(const auto* name:{"default","viewportDepthTexture"})own(module,name,bindings.get(name));bindings.retainForHeapLifetime();depthTextureLexical()=JSWeakValue(bindings);module.retainForHeapLifetime();weak=JSWeakValue(module);return module;
}
JSValue nativeViewportDepthTextureNodeBinding(const std::string& name){const auto keeper=nativeViewportDepthTextureNodeModule();return depthTextureLexical().lock()->get(name);}
JSValue nativeViewportDepthTextureNodeConstructor(){return nativeViewportDepthTextureNodeBinding("default");}
ViewportDepthTextureNode::ViewportDepthTextureNode(const JSValue& uv,const JSValue& level,const JSValue& depth):object_(constructor().construct({uv,level,depth})){}
