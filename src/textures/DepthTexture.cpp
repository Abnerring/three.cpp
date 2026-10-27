#include "textures/DepthTexture.h"
#include "core/JSModuleObject.h"
#include "core/JSError.h"
#include "core/JSPromise.h"
namespace {
Texture::Image depthImage(double width, double height, double depth, double format) {
    if (format != DepthFormat && format != DepthStencilFormat)
        throw std::invalid_argument("THREE.DepthTexture: format must be either THREE.DepthFormat or THREE.DepthStencilFormat");
    return std::make_shared<TextureImage>(width, height, depth);
}
}
DepthTexture::DepthTexture(double width, double height, double type, double mapping, double wrapS, double wrapT,
                          double magFilter, double minFilter, double anisotropy, double format, double depth)
    : Texture(depthImage(width, height, depth, format), mapping, wrapS, wrapT, magFilter, minFilter, format, type, anisotropy) {
    isDepthTexture = true; flipY = false; generateMipmaps = false;
}
Texture& DepthTexture::copy(const Texture& other) {
    Texture::copy(other);
    auto descriptor = other.image.get() ? std::make_shared<TextureImage>(*other.image.get()) : std::make_shared<TextureImage>();
    descriptor->isArray = false; // Object.assign({}, array) keeps indexed fields but loses Array branding.
    source = std::make_shared<TextureSource>(std::move(descriptor));
    compareFunction = dynamic_cast<const DepthTexture&>(other).compareFunction;
    return *this;
}
std::shared_ptr<Texture> DepthTexture::clone() const {
    auto result = std::make_shared<DepthTexture>(); result->copy(*this); return result;
}
JsonValue DepthTexture::toJSON(TextureJSONContext* meta) const {
    auto result = Texture::toJSON(meta);
    result["compareFunction"] = compareFunction ? JsonValue(*compareFunction) : JsonValue{};
    return result;
}

namespace {
using Args=std::vector<JSValue>;
#include "textures/DepthTextureSource.inc"
JSWeakValue& depthBindings(){static thread_local JSWeakValue value;return value;}
JSValue arg(const Args& args,unsigned i){return i<args.size()?args[i]:JSValue{};}
void own(JSValue object,const char* key,const JSValue& value){JSPropertyDescriptor d;d.value=value;d.writable=d.enumerable=d.configurable=true;object.defineProperty(key,d);}
JSValue depthMethod(const JSValue& receiver,const Args& args,const Args& captures){
    auto self=receiver;const auto base=captures[0].getPrototype();
    if(captures[1].strictEquals("copy")){
        const auto superCopy=base.get("copy",self);const auto source=arg(args,0);superCopy.call(self,{source});
        // Resolve new-expression callee, then Object.assign callee, then image.
        const auto sourceConstructor=captures[2],object=JSValue::intrinsicObjectConstructor(),assign=object.get("assign");
        const auto target=JSValue::object(),image=source.get("image");
        const auto descriptor=assign.call(object,{target,image});self.set("source",sourceConstructor.construct({descriptor}));
        self.set("compareFunction",source.get("compareFunction"));return self;
    }
    const auto superJSON=base.get("toJSON",self);auto data=superJSON.call(self,{arg(args,0)});
    data.set("compareFunction",self.get("compareFunction"));return data;
}
}
JSValue nativeDepthTextureModule(){
    static thread_local JSWeakValue weak;if(const auto module=weak.lock();module&&module->isObject())return *module;
    static thread_local bool initializing=false;JSModuleInitializationScope scope(initializing,"DepthTexture");
    const auto source=nativeTextureSourceConstructor(),base=nativeTextureConstructor();auto state=JSValue::object(nullptr);
    auto ctor=JSValue::builtinConstructor(+[](const JSValue& target,const Args& args,const Args& captures)->JSValue{
        const auto type=arg(args,2).isUndefined()?JSValue(UnsignedIntType):arg(args,2);
        const auto mag=arg(args,6).isUndefined()?JSValue(NearestFilter):arg(args,6);
        const auto min=arg(args,7).isUndefined()?JSValue(NearestFilter):arg(args,7);
        const auto format=arg(args,9).isUndefined()?JSValue(DepthFormat):arg(args,9);
        const auto depth=arg(args,10).isUndefined()?JSValue(1):arg(args,10);
        if(!format.strictEquals(DepthFormat)&&!format.strictEquals(DepthStencilFormat))throw JSThrownValue(nativeErrorConstructor().construct({"THREE.DepthTexture: format must be either THREE.DepthFormat or THREE.DepthStencilFormat"}));
        auto image=JSValue::object();own(image,"width",arg(args,0));own(image,"height",arg(args,1));own(image,"depth",depth);
        auto self=captures[0].get("constructor").getPrototype().construct({image,arg(args,3),arg(args,4),arg(args,5),mag,min,format,type,arg(args,8)},target);
        self.set("isDepthTexture",true);self.set("flipY",false);self.set("generateMipmaps",false);self.set("compareFunction",nullptr);return self;
    },{state},"DepthTexture",2);
    state.set("constructor",ctor);ctor.setPrototype(base);auto p=ctor.get("prototype");p.setPrototype(base.get("prototype"));ctor.setFunctionSourceText(classSource);
    for(const auto& [name,text]:std::initializer_list<std::pair<const char*,const char*>>{{"copy",copySource},{"toJSON",toJSONSource}}){
        auto fn=JSValue::function(depthMethod,{p,name,source},name,1);fn.setFunctionSourceText(text);JSPropertyDescriptor d;d.value=fn;d.writable=d.configurable=true;p.defineProperty(name,d);
    }
    auto bindings=JSValue::object(nullptr);own(bindings,"DepthTexture",ctor);bindings.retainForHeapLifetime();depthBindings()=JSWeakValue(bindings);
    auto module=JSValue::object();own(module,"DepthTexture",ctor);module.retainForHeapLifetime();weak=JSWeakValue(module);return module;
}
JSValue nativeDepthTextureBinding(const std::string& name){const auto keeper=nativeDepthTextureModule();return depthBindings().lock()->get(name);}
JSValue nativeDepthTextureConstructor(){return nativeDepthTextureBinding("DepthTexture");}
