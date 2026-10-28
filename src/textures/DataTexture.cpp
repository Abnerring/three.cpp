#include "textures/DataTexture.h"
DataTexture::DataTexture(TextureImage::Data data, double width, double height, double format, double type,
                         double mapping, double wrapS, double wrapT, double magFilter, double minFilter,
                         double anisotropy, std::string colorSpace)
    : Texture(nullptr, mapping, wrapS, wrapT, magFilter, minFilter, format, type, anisotropy, std::move(colorSpace)) {
    isDataTexture = true;
    image = std::make_shared<TextureImage>(std::move(data), width, height);
    generateMipmaps = false; flipY = false; unpackAlignment = 1;
}
std::shared_ptr<Texture> DataTexture::clone() const {
    auto result = std::make_shared<DataTexture>(); result->copy(*this); return result;
}

#include "core/JSModuleObject.h"
#include "textures/DataTextureSource.inc"
namespace {
using Args=std::vector<JSValue>;
JSWeakValue& dataTextureLexical(){static thread_local JSWeakValue weak;return weak;}
JSValue dataArg(const Args& args,unsigned i,const JSValue& fallback={}){return i<args.size()&&!args[i].isUndefined()?args[i]:fallback;}
void dataOwn(JSValue object,const char* key,const JSValue& value){JSPropertyDescriptor d;d.value=value;d.writable=d.enumerable=d.configurable=true;object.defineProperty(key,d);}
}
JSValue nativeDataTextureModule(){
    static thread_local JSWeakValue weak;if(const auto module=weak.lock();module&&module->isObject())return *module;
    static thread_local bool initializing=false;JSModuleInitializationScope scope(initializing,"DataTexture");
    const auto base=nativeTextureConstructor();const auto constants=nativeConstantsModule();
    auto holder=JSValue::object(nullptr);const auto nearest=nativeConstantsBinding("NearestFilter");
    auto ctor=JSValue::builtinConstructor(+[](const JSValue& target,const Args& args,const Args& captures)->JSValue{
        const auto data=dataArg(args,0,nullptr),width=dataArg(args,1,1),height=dataArg(args,2,1);
        const auto mag=dataArg(args,8,captures[1]),min=dataArg(args,9,captures[1]);
        auto self=captures[0].get("constructor").getPrototype().construct({nullptr,dataArg(args,5),dataArg(args,6),dataArg(args,7),mag,min,dataArg(args,3),dataArg(args,4),dataArg(args,10),dataArg(args,11)},target);
        self.set("isDataTexture",true);auto image=JSValue::object();dataOwn(image,"data",data);dataOwn(image,"width",width);dataOwn(image,"height",height);self.set("image",image);
        self.set("generateMipmaps",false);self.set("flipY",false);self.set("unpackAlignment",1);return self;
    },{holder,nearest},"DataTexture",0);
    holder.set("constructor",ctor);ctor.setPrototype(base);auto p=ctor.get("prototype");p.setPrototype(base.get("prototype"));ctor.setFunctionSourceText(DataTextureSource::classSource);
    auto bindings=JSValue::object(nullptr);dataOwn(bindings,"DataTexture",ctor);bindings.retainForHeapLifetime();dataTextureLexical()=JSWeakValue(bindings);
    auto module=JSValue::object();dataOwn(module,"DataTexture",ctor);module.retainForHeapLifetime();weak=JSWeakValue(module);return module;
}
JSValue nativeDataTextureBinding(const std::string& name){const auto keeper=nativeDataTextureModule();return dataTextureLexical().lock()->get(name);}
JSValue nativeDataTextureConstructor(){return nativeDataTextureBinding("DataTexture");}
