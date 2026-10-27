#include "textures/FramebufferTexture.h"
#include "core/JSModuleObject.h"
FramebufferTexture::FramebufferTexture(double width, double height) : Texture(std::make_shared<TextureImage>(width, height)) {
    isFramebufferTexture = true;
    magFilter = NearestFilter; minFilter = NearestFilter; generateMipmaps = false;
    needsUpdate = true;
}
Texture& FramebufferTexture::copy(const Texture& other) {
    Texture::copy(other);
    auto descriptor = other.image.get() ? std::make_shared<TextureImage>(*other.image.get()) : std::make_shared<TextureImage>();
    descriptor->isArray = false;
    source = std::make_shared<TextureSource>(std::move(descriptor));
    return *this;
}
std::shared_ptr<Texture> FramebufferTexture::clone() const {
    auto result = std::make_shared<FramebufferTexture>(); result->copy(*this); return result;
}

namespace {
using Args=std::vector<JSValue>;
#include "textures/FramebufferTextureSource.inc"
JSWeakValue& framebufferBindings(){static thread_local JSWeakValue value;return value;}
JSValue arg(const Args& args,unsigned i){return i<args.size()?args[i]:JSValue{};}
void own(JSValue object,const char* key,const JSValue& value){JSPropertyDescriptor d;d.value=value;d.writable=d.enumerable=d.configurable=true;object.defineProperty(key,d);}
}
JSValue nativeFramebufferTextureModule(){
    static thread_local JSWeakValue weak;if(const auto module=weak.lock();module&&module->isObject())return *module;
    static thread_local bool initializing=false;JSModuleInitializationScope scope(initializing,"FramebufferTexture");
    const auto base=nativeTextureConstructor(),source=nativeTextureSourceConstructor();auto state=JSValue::object(nullptr);
    auto ctor=JSValue::builtinConstructor(+[](const JSValue& target,const Args& args,const Args& captures)->JSValue{
        auto image=JSValue::object();own(image,"width",arg(args,0));own(image,"height",arg(args,1));
        auto self=captures[0].get("constructor").getPrototype().construct({image},target);
        self.set("isFramebufferTexture",true);self.set("magFilter",NearestFilter);self.set("minFilter",NearestFilter);self.set("generateMipmaps",false);self.set("needsUpdate",true);return self;
    },{state},"FramebufferTexture",2);
    state.set("constructor",ctor);ctor.setPrototype(base);auto p=ctor.get("prototype");p.setPrototype(base.get("prototype"));ctor.setFunctionSourceText(classSource);
    auto copy=JSValue::function(+[](const JSValue& receiver,const Args& args,const Args& captures)->JSValue{
        auto self=receiver;const auto base=captures[0].getPrototype(),method=base.get("copy",self),source=arg(args,0);method.call(self,{source});
        const auto sourceConstructor=captures[1],object=JSValue::intrinsicObjectConstructor(),assign=object.get("assign");
        const auto target=JSValue::object(),image=source.get("image");const auto descriptor=assign.call(object,{target,image});
        self.set("source",sourceConstructor.construct({descriptor}));return self;
    },{p,source},"copy",1);copy.setFunctionSourceText(copySource);JSPropertyDescriptor d;d.value=copy;d.writable=d.configurable=true;p.defineProperty("copy",d);
    auto bindings=JSValue::object(nullptr);own(bindings,"FramebufferTexture",ctor);bindings.retainForHeapLifetime();framebufferBindings()=JSWeakValue(bindings);
    auto module=JSValue::object();own(module,"FramebufferTexture",ctor);module.retainForHeapLifetime();weak=JSWeakValue(module);return module;
}
JSValue nativeFramebufferTextureBinding(const std::string& name){const auto keeper=nativeFramebufferTextureModule();return framebufferBindings().lock()->get(name);}
JSValue nativeFramebufferTextureConstructor(){return nativeFramebufferTextureBinding("FramebufferTexture");}
