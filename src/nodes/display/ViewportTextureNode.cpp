#include "nodes/display/ViewportTextureNode.h"
#include "nodes/display/ViewportNodeSources.inc"
#include <string_view>

#include <string>

bool ViewportTexture::filterAndMipmapsAgree() {
    // The filter reads mipmaps and the setting forbids making them, so they do not agree.
    const bool filterReadsMipmaps = std::string(kMinFilter) == "LinearMipmapLinearFilter";
    return !(filterReadsMipmaps && !generatesMipmaps);
}

bool ViewportTexture::makesDefaultTexture(bool hasGivenTexture) {
    // A texture is made only when none was given.
    return !hasGivenTexture;
}

// Native managed port of pinned src/nodes/display/ViewportTextureNode.js.
#include "nodes/accessors/TextureNode.h"
#include "nodes/display/ScreenNode.h"
#include "nodes/tsl/TSLBase.h"
#include "nodes/utils/UtilityNodeRuntime.h"
#include "core/JSWeakMap.h"
#include "math/Vector2.h"
#include "textures/FramebufferTexture.h"
#include "constants.h"
#include <algorithm>

namespace {
using namespace UtilityNodeRuntime;
JSWeakValue& viewportTextureLexical(){static thread_local JSWeakValue weak;return weak;}
JSValue viewportTextureMethod(const JSValue& receiver,const Args& args,const Args& captures){
    auto self=receiver;const auto name=captures[0],table=captures[1];
    if(name.strictEquals("getTextureForReference")){
        const auto reference=arg(args,0,nullptr);JSValue framebuffer,cache;
        if(self.get("referenceNode").toBoolean()){
            framebuffer=self.get("referenceNode").get("defaultFramebuffer");
            cache=self.get("referenceNode").get("_cacheTextures");
        }else {framebuffer=self.get("defaultFramebuffer");cache=self.get("_cacheTextures");}
        if(reference.isNull())return framebuffer;
        if(invoke(cache,"has")({reference}).strictEquals(false)){
            const auto texture=invoke(framebuffer,"clone")();invoke(cache,"set")({reference,texture});
        }
        return invoke(cache,"get")({reference});
    }
    if(name.strictEquals("updateReference")){
        const auto renderer=arg(args,0).get("renderer"),renderTarget=invoke(renderer,"getRenderTarget")(),canvasTarget=invoke(renderer,"getCanvasTarget")();
        const auto reference=renderTarget.toBoolean()?renderTarget:canvasTarget;
        const auto value=invoke(self,"getTextureForReference")({reference});self.set("value",value);return self.get("value");
    }
    if(name.strictEquals("updateBefore")){
        const auto renderer=arg(args,0).get("renderer"),renderTarget=invoke(renderer,"getRenderTarget")(),canvasTarget=invoke(renderer,"getCanvasTarget")();
        const auto reference=renderTarget.toBoolean()?renderTarget:canvasTarget,size=table.get("_size");
        if(reference.isNull())invoke(renderer,"getDrawingBufferSize")({size});
        else if(reference.get("getDrawingBufferSize").toBoolean())invoke(reference,"getDrawingBufferSize")({size});
        else {const auto set=invoke(size,"set");const auto width=reference.get("width"),height=reference.get("height");set({width,height});}
        auto texture=invoke(self,"getTextureForReference")({reference});
        const auto width=texture.get("image").get("width"),sizeWidth=size.get("width");
        bool resize=!width.strictEquals(sizeWidth);
        if(!resize){const auto height=texture.get("image").get("height"),sizeHeight=size.get("height");resize=!height.strictEquals(sizeHeight);}
        if(resize){
            auto image=texture.get("image");const auto x=size.get("width");image.set("width",x);
            image=texture.get("image");const auto y=size.get("height");image.set("height",y);texture.set("needsUpdate",true);
        }
        const auto previous=texture.get("generateMipmaps"),generate=self.get("generateMipmaps");texture.set("generateMipmaps",generate);
        invoke(renderer,"copyFramebufferToTexture")({texture});texture.set("generateMipmaps",previous);return {};
    }
    if(name.strictEquals("clone")){
        const auto constructor=self.get("constructor"),uv=self.get("uvNode"),level=self.get("levelNode"),value=self.get("value");
        auto node=constructor.construct({uv,level,value});const auto mipmaps=self.get("generateMipmaps");node.set("generateMipmaps",mipmaps);return node;
    }
    throw std::logic_error("Unknown ViewportTextureNode method");
}
}
JSValue nativeViewportTextureNodeModule(){
    using namespace UtilityNodeRuntime;
    static thread_local JSWeakValue weak;if(const auto value=weak.lock();value&&value->isObject())return *value;
    static thread_local bool initializing=false;JSModuleInitializationScope scope(initializing,"ViewportTextureNode module");
    const auto texture=nativeTextureNodeModule(),types=nativeNodeConstantsBinding("NodeUpdateType"),tsl=nativeTSLBase(),screen=nativeScreenNodeModule();
    const auto vector=nativeVector2Constructor(),framebuffer=nativeFramebufferTextureModule(),constants=nativeConstantsModule();
    auto table=JSValue::object(nullptr),bindings=JSValue::object(nullptr),holder=JSValue::object(nullptr);
    own(table,"screenUV",nativeScreenNodeBinding("screenUV"));own(table,"FramebufferTexture",nativeFramebufferTextureBinding("FramebufferTexture"));
    own(table,"LinearMipmapLinearFilter",nativeConstantsBinding("LinearMipmapLinearFilter"));own(table,"NodeUpdateType",types);own(table,"_size",vector.construct());
    auto ctor=JSValue::builtinConstructor(+[](const JSValue& target,const Args& args,const Args& captures)->JSValue{
        const auto table=captures[0],holder=captures[1];const auto uv=arg(args,0,table.get("screenUV")),level=arg(args,1,nullptr);auto framebuffer=arg(args,2,nullptr),defaultFramebuffer=JSValue(nullptr);
        if(framebuffer.isNull()){defaultFramebuffer=table.get("FramebufferTexture").construct();defaultFramebuffer.set("minFilter",table.get("LinearMipmapLinearFilter"));framebuffer=defaultFramebuffer;}
        else defaultFramebuffer=framebuffer;
        auto self=holder.get("constructor").getPrototype().construct({framebuffer,uv,level},target);
        self.set("generateMipmaps",false);self.set("defaultFramebuffer",defaultFramebuffer);self.set("isOutputTextureNode",true);
        self.set("updateBeforeType",table.get("NodeUpdateType").get("RENDER"));self.set("_cacheTextures",createNativeWeakMap());return self;
    },{table,holder},"ViewportTextureNode",0);
    ctor.setFunctionSourceText(ViewportTextureNodeSource::classSource);holder.set("constructor",ctor);ctor.setPrototype(nativeTextureNodeBinding("default"));auto prototype=ctor.get("prototype");prototype.setPrototype(ctor.getPrototype().get("prototype"));
    JSPropertyDescriptor type;type.getter=JSValue::function(+[](const JSValue&,const Args&,const Args&)->JSValue{return "ViewportTextureNode";},{},"get type",0);type.getter->setFunctionSourceText(ViewportTextureNodeSource::typeSource);type.configurable=true;ctor.defineProperty("type",type);
    for(const auto& [name,length]:std::initializer_list<std::pair<const char*,unsigned>>{{"getTextureForReference",0},{"updateReference",1},{"updateBefore",1},{"clone",0}}){JSPropertyDescriptor d;d.value=JSValue::function(viewportTextureMethod,{name,table},name,length);d.value->setFunctionSourceText(std::string_view(name)=="getTextureForReference"?ViewportTextureNodeSource::getTextureForReferenceSource:std::string_view(name)=="updateReference"?ViewportTextureNodeSource::updateReferenceSource:std::string_view(name)=="updateBefore"?ViewportTextureNodeSource::updateBeforeSource:ViewportTextureNodeSource::cloneSource);d.writable=d.configurable=true;prototype.defineProperty(name,d);}
    own(bindings,"default",ctor);const auto nodeProxy=nativeTSLBaseBinding("nodeProxy");
    own(bindings,"viewportTexture",invoke(nodeProxy.call({}, {ctor}),"setParameterLength")({0,3}));
    auto properties=JSValue::object();own(properties,"generateMipmaps",true);
    own(bindings,"viewportMipTexture",invoke(nodeProxy.call({}, {ctor,nullptr,nullptr,properties}),"setParameterLength")({0,3}));
    own(table,"_singletonOpaqueViewportTextureNode",bindings.get("viewportMipTexture").call());
    own(bindings,"viewportOpaqueMipTexture",JSValue::function(+[](const JSValue&,const Args& args,const Args& captures)->JSValue{
        const auto table=captures[0];const auto sample=invoke(table.get("_singletonOpaqueViewportTextureNode"),"sample");return sample({arg(args,0,table.get("screenUV")),arg(args,1,nullptr)});
    },{table},"viewportOpaqueMipTexture",0));
    auto opaque=bindings.get("viewportOpaqueMipTexture");opaque.setFunctionSourceText(ViewportTextureNodeSource::viewportOpaqueMipTextureSource);
    auto module=JSValue::object();auto names=bindings.ownPropertyNames();std::sort(names.begin(),names.end());for(const auto& name:names)own(module,name.c_str(),bindings.get(name));
    bindings.retainForHeapLifetime();viewportTextureLexical()=JSWeakValue(bindings);module.retainForHeapLifetime();weak=JSWeakValue(module);return module;
}
JSValue nativeViewportTextureNodeBinding(const std::string& name){const auto keeper=nativeViewportTextureNodeModule();return viewportTextureLexical().lock()->get(name);}
JSValue nativeViewportTextureNodeConstructor(){return nativeViewportTextureNodeBinding("default");}
ViewportTextureNode::ViewportTextureNode(const JSValue& uv,const JSValue& level,const JSValue& framebuffer):object_(constructor().construct({uv,level,framebuffer})){}
