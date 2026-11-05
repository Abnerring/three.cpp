#include "textures/Data3DTexture.h"
Data3DTexture::Data3DTexture(TextureImage::Data data, double width, double height, double depth) : Texture(nullptr) {
    isData3DTexture = true;
    image = std::make_shared<TextureImage>(std::move(data), width, height, depth);
    magFilter = NearestFilter; minFilter = NearestFilter;
    generateMipmaps = false; flipY = false; unpackAlignment = 1;
}
Texture& Data3DTexture::copy(const Texture& source) {
    Texture::copy(source); wrapR = dynamic_cast<const Data3DTexture&>(source).wrapR; return *this;
}
std::shared_ptr<Texture> Data3DTexture::clone() const {
    auto result = std::make_shared<Data3DTexture>(); result->copy(*this); return result;
}

#include "core/JSModuleObject.h"
#include "textures/Data3DTextureSource.inc"
namespace {
using Args=std::vector<JSValue>;
JSWeakValue& data3DTextureLexical(){static thread_local JSWeakValue weak;return weak;}
JSValue volumeArg(const Args& args,unsigned i,const JSValue& fallback={}){return i<args.size()&&!args[i].isUndefined()?args[i]:fallback;}
void volumeOwn(JSValue object,const char* key,const JSValue& value){JSPropertyDescriptor d;d.value=value;d.writable=d.enumerable=d.configurable=true;object.defineProperty(key,d);}
}
JSValue nativeData3DTextureModule(){
    static thread_local JSWeakValue weak;if(const auto module=weak.lock();module&&module->isObject())return *module;
    static thread_local bool initializing=false;JSModuleInitializationScope scope(initializing,"Data3DTexture");
    const auto base=nativeTextureConstructor();auto holder=JSValue::object(nullptr);
    auto ctor=JSValue::builtinConstructor(+[](const JSValue& target,const Args& args,const Args& captures)->JSValue{
        const auto data=volumeArg(args,0,nullptr),width=volumeArg(args,1,1),height=volumeArg(args,2,1),depth=volumeArg(args,3,1);
        auto self=captures[0].get("constructor").getPrototype().construct({nullptr},target);
        self.set("isData3DTexture",true);
        auto image=JSValue::object();volumeOwn(image,"data",data);volumeOwn(image,"width",width);volumeOwn(image,"height",height);volumeOwn(image,"depth",depth);self.set("image",image);
        self.set("magFilter",captures[1]);self.set("minFilter",captures[1]);self.set("wrapR",captures[2]);
        self.set("generateMipmaps",false);self.set("flipY",false);self.set("unpackAlignment",1);return self;
    },{holder,nativeConstantsBinding("NearestFilter"),nativeConstantsBinding("ClampToEdgeWrapping")},"Data3DTexture",0);
    holder.set("constructor",ctor);ctor.setPrototype(base);auto prototype=ctor.get("prototype");prototype.setPrototype(base.get("prototype"));ctor.setFunctionSourceText(Data3DTextureSource::classSource);
    auto copy=JSValue::function(+[](const JSValue& receiver,const Args& args,const Args& captures)->JSValue{
        auto self=receiver;const auto source=volumeArg(args,0);
        captures[0].getPrototype().get("copy",self).call(self,{source});self.set("wrapR",source.get("wrapR"));return self;
    },{prototype},"copy",1);
    copy.setFunctionSourceText(Data3DTextureSource::copySource);JSPropertyDescriptor descriptor;descriptor.value=copy;descriptor.writable=descriptor.configurable=true;prototype.defineProperty("copy",descriptor);
    auto bindings=JSValue::object(nullptr);volumeOwn(bindings,"Data3DTexture",ctor);bindings.retainForHeapLifetime();data3DTextureLexical()=JSWeakValue(bindings);
    auto module=JSValue::object();volumeOwn(module,"Data3DTexture",ctor);module.retainForHeapLifetime();weak=JSWeakValue(module);return module;
}
JSValue nativeData3DTextureBinding(const std::string& name){const auto keeper=nativeData3DTextureModule();return data3DTextureLexical().lock()->get(name);}
JSValue nativeData3DTextureConstructor(){return nativeData3DTextureBinding("Data3DTexture");}
