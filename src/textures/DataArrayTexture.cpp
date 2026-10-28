#include "textures/DataArrayTexture.h"
#include <cmath>
#include <tuple>
DataArrayTexture::DataArrayTexture(TextureImage::Data data, double width, double height, double depth) : Texture(nullptr) {
    isDataArrayTexture = true;
    image = std::make_shared<TextureImage>(std::move(data), width, height, depth);
    magFilter = NearestFilter; minFilter = NearestFilter;
    generateMipmaps = false; flipY = false; unpackAlignment = 1;
}
Texture& DataArrayTexture::copy(const Texture& source) {
    Texture::copy(source); wrapR = dynamic_cast<const DataArrayTexture&>(source).wrapR; return *this;
}
std::shared_ptr<Texture> DataArrayTexture::clone() const {
    auto result = std::make_shared<DataArrayTexture>(); result->copy(*this); return result;
}
void DataArrayTexture::addLayerUpdate(double layerIndex) {
    for (double value : *layerUpdates)
        if (value == layerIndex || (std::isnan(value) && std::isnan(layerIndex))) return;
    layerUpdates->push_back(layerIndex == 0 ? 0 : layerIndex);
}
void DataArrayTexture::clearLayerUpdates() { layerUpdates->clear(); }

#include "core/JSGlobal.h"
#include "core/JSModuleObject.h"
#include "textures/DataArrayTextureSource.inc"
namespace {
using Args=std::vector<JSValue>;
JSWeakValue& dataArrayTextureLexical(){static thread_local JSWeakValue weak;return weak;}
JSValue arrayArg(const Args& args,unsigned i,const JSValue& fallback={}){return i<args.size()&&!args[i].isUndefined()?args[i]:fallback;}
void arrayOwn(JSValue object,const char* key,const JSValue& value){JSPropertyDescriptor d;d.value=value;d.writable=d.enumerable=d.configurable=true;object.defineProperty(key,d);}
}
JSValue nativeDataArrayTextureModule(){
    static thread_local JSWeakValue weak;if(const auto module=weak.lock();module&&module->isObject())return *module;
    static thread_local bool initializing=false;JSModuleInitializationScope scope(initializing,"DataArrayTexture");
    const auto base=nativeTextureConstructor();const auto constants=nativeConstantsModule();auto holder=JSValue::object(nullptr);
    auto ctor=JSValue::builtinConstructor(+[](const JSValue& target,const Args& args,const Args& captures)->JSValue{
        const auto data=arrayArg(args,0,nullptr),width=arrayArg(args,1,1),height=arrayArg(args,2,1),depth=arrayArg(args,3,1);
        auto self=captures[0].get("constructor").getPrototype().construct({nullptr},target);self.set("isDataArrayTexture",true);
        auto image=JSValue::object();arrayOwn(image,"data",data);arrayOwn(image,"width",width);arrayOwn(image,"height",height);arrayOwn(image,"depth",depth);self.set("image",image);
        self.set("magFilter",captures[1]);self.set("minFilter",captures[1]);self.set("wrapR",captures[2]);self.set("generateMipmaps",false);self.set("flipY",false);self.set("unpackAlignment",1);
        self.set("layerUpdates",nativeGlobalBinding("Set").construct());return self;
    },{holder,nativeConstantsBinding("NearestFilter"),nativeConstantsBinding("ClampToEdgeWrapping")},"DataArrayTexture",0);
    holder.set("constructor",ctor);ctor.setPrototype(base);auto p=ctor.get("prototype");p.setPrototype(base.get("prototype"));ctor.setFunctionSourceText(DataArrayTextureSource::classSource);
    for(const auto& [name,source,arity]:std::initializer_list<std::tuple<const char*,const char*,unsigned>>{{"copy",DataArrayTextureSource::copySource,1},{"addLayerUpdate",DataArrayTextureSource::addLayerUpdateSource,1},{"clearLayerUpdates",DataArrayTextureSource::clearLayerUpdatesSource,0}}){
        auto fn=JSValue::function(+[](const JSValue& receiver,const Args& args,const Args& captures)->JSValue{
            auto self=receiver;if(captures[0].strictEquals("copy")){const auto superCopy=captures[1].getPrototype().get("copy",self),source=arrayArg(args,0);superCopy.call(self,{source});self.set("wrapR",source.get("wrapR"));return self;}
            const auto layers=self.get("layerUpdates");if(captures[0].strictEquals("addLayerUpdate")){const auto add=layers.get("add");add.call(layers,{arrayArg(args,0)});}else layers.get("clear").call(layers);return {};
        },{name,p},name,arity);fn.setFunctionSourceText(source);JSPropertyDescriptor d;d.value=fn;d.writable=d.configurable=true;p.defineProperty(name,d);
    }
    auto bindings=JSValue::object(nullptr);arrayOwn(bindings,"DataArrayTexture",ctor);bindings.retainForHeapLifetime();dataArrayTextureLexical()=JSWeakValue(bindings);
    auto module=JSValue::object();arrayOwn(module,"DataArrayTexture",ctor);module.retainForHeapLifetime();weak=JSWeakValue(module);return module;
}
JSValue nativeDataArrayTextureBinding(const std::string& name){const auto keeper=nativeDataArrayTextureModule();return dataArrayTextureLexical().lock()->get(name);}
JSValue nativeDataArrayTextureConstructor(){return nativeDataArrayTextureBinding("DataArrayTexture");}
