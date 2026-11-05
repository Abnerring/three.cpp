// Port of three.js MaterialLoader.js at 11738a9d55e502fb484cefffcea3a66dc44e9fe9.
#include "loaders/MaterialLoader.h"
#include "loaders/MaterialLoaderSources.inc"
#include "loaders/FileLoader.h"
#include "materials/Materials.h"
#include "core/NativeObject.h"
#include "core/JSGlobal.h"
#include "core/JSPromise.h"
#include "nodes/lighting/LightingNodeRuntime.h"
#include "utils.h"

namespace {
using namespace LightingNodeRuntime;
JSWeakValue& lexical() { static thread_local JSWeakValue value; return value; }

JSValue load(const JSValue& self, const Args& args, const Args&) {
    const auto url=arg(args,0),onLoad=arg(args,1),onProgress=arg(args,2),onError=arg(args,3);
    FileLoader loader(self.get("manager"));
    invoke(loader.object(),"setPath")({self.get("path")});
    invoke(loader.object(),"setRequestHeader")({self.get("requestHeader")});
    invoke(loader.object(),"setWithCredentials")({self.get("withCredentials")});
    auto callback=JSValue::function(+[](const JSValue&,const Args& args,const Args& captures)->JSValue {
        const auto scope=captures[0],onLoad=captures[1],onError=captures[2],url=captures[3];
        try {
            const auto parse=invoke(scope,"parse");
            const auto json=nativeGlobalBinding("JSON");
            const auto data=json.get("parse").call(json,{arg(args,0)});
            onLoad.call({}, {parse({data})});
        } catch(...) {
            const auto reason=nativeExceptionValue(std::current_exception());
            if(onError.toBoolean())onError.call({}, {reason});
            else ThreeUtils::error({reason});
            invoke(scope.get("manager"),"itemError")({url});
        }
        return {};
    },{self,onLoad,onError,url},"",1);
    callback.setFunctionSourceText(MaterialLoaderSource::loadCallbackSource);
    loader.load(url,callback,onProgress,onError);
    return {};
}
JSValue parse(const JSValue& self,const Args& args,const Args&) {
    const auto json=arg(args,0);
    const auto material=invoke(self,"createMaterialFromType")({json.get("type")});
    invoke(material,"fromJSON")({json,self.get("textures")});
    return material;
}
JSValue staticCreate(const JSValue&,const Args& args,const Args& captures) {
    const auto type=arg(args,0),imports=captures[0];
    auto library=JSValue::object();
    for(const auto& name:imports.get("materials").ownPropertyKeys())
        library.defineProperty(name,JSPropertyDescriptor{.value=imports.get("materials").get(name),.writable=true,.enumerable=true,.configurable=true});
    const auto custom=imports.get("customMaterials");
    for(const auto& key:custom.ownPropertyKeys()) {
        const auto d=custom.getOwnPropertyDescriptor(key);
        if(d && d->enumerable.value_or(false))
            library.defineProperty(key,JSPropertyDescriptor{.value=custom.get(key),.writable=true,.enumerable=true,.configurable=true});
    }
    const auto materialType=library.get(type);
    if(materialType.isUndefined()) {
        ThreeUtils::warnOnce({"MaterialLoader: Unknown material type \""+type.toString()+"\". Use .registerMaterial() before starting the deserialization process."});
        return imports.get("materials").get("Material").construct();
    }
    return materialType.construct();
}
JSValue registerMaterial(const JSValue&,const Args& args,const Args& captures) {
    auto custom=captures[0].get("customMaterials");
    const auto key=arg(args,0).toPropertyKey();
    custom.set(key,arg(args,1));
    return {};
}
// Numeric renderer compatibility is kept at this explicit C++ boundary. The
// managed parse above always returns the actual source-class material object.
std::shared_ptr<Material> numericMaterial(const std::string& type) {
#define THREE_NUMERIC_MATERIAL(Type) if(type==#Type)return std::make_shared<Type>();
    THREE_NUMERIC_MATERIAL(ShadowMaterial) THREE_NUMERIC_MATERIAL(SpriteMaterial)
    THREE_NUMERIC_MATERIAL(RawShaderMaterial) THREE_NUMERIC_MATERIAL(ShaderMaterial)
    THREE_NUMERIC_MATERIAL(PointsMaterial) THREE_NUMERIC_MATERIAL(MeshPhysicalMaterial)
    THREE_NUMERIC_MATERIAL(MeshStandardMaterial) THREE_NUMERIC_MATERIAL(MeshPhongMaterial)
    THREE_NUMERIC_MATERIAL(MeshToonMaterial) THREE_NUMERIC_MATERIAL(MeshNormalMaterial)
    THREE_NUMERIC_MATERIAL(MeshLambertMaterial) THREE_NUMERIC_MATERIAL(MeshDepthMaterial)
    THREE_NUMERIC_MATERIAL(MeshDistanceMaterial) THREE_NUMERIC_MATERIAL(MeshBasicMaterial)
    THREE_NUMERIC_MATERIAL(MeshMatcapMaterial) THREE_NUMERIC_MATERIAL(LineDashedMaterial)
    THREE_NUMERIC_MATERIAL(LineBasicMaterial) THREE_NUMERIC_MATERIAL(Material)
#undef THREE_NUMERIC_MATERIAL
    throw JSTypeError("MaterialLoader.parseMaterial: material type has no numeric renderer adapter: "+type);
}
}
JSValue nativeMaterialLoaderModule() {
    static thread_local JSWeakValue weak;
    if(const auto value=weak.lock();value&&value->isObject())return *value;
    static thread_local bool active=false;JSModuleInitializationScope scope(active,"MaterialLoader");
    auto materials=JSValue::object();
    for(const auto* name:{"ShadowMaterial","SpriteMaterial","RawShaderMaterial","ShaderMaterial","PointsMaterial",
        "MeshPhysicalMaterial","MeshStandardMaterial","MeshPhongMaterial","MeshToonMaterial","MeshNormalMaterial",
        "MeshLambertMaterial","MeshDepthMaterial","MeshDistanceMaterial","MeshBasicMaterial","MeshMatcapMaterial",
        "LineDashedMaterial","LineBasicMaterial","Material"})own(materials,name,nativeMaterialsBinding(name));
    auto imports=JSValue::object(nullptr);own(imports,"materials",materials);own(imports,"customMaterials",JSValue::object());
    auto holder=JSValue::object(nullptr);
    auto ctor=JSValue::builtinConstructor(+[](const JSValue& target,const Args& args,const Args& captures)->JSValue {
        auto self=captures[0].get("constructor").getPrototype().construct({arg(args,0)},target);
        self.set("textures",JSValue::object());return self;
    },{holder},"MaterialLoader",1);
    own(holder,"constructor",ctor);ctor.setPrototype(Loader::prototype().get("constructor"));
    auto prototype=ctor.get("prototype");prototype.setPrototype(Loader::prototype());ctor.setFunctionSourceText(MaterialLoaderSource::classSource);
    method(prototype,"load",4,load,{},MaterialLoaderSource::loadSource);
    method(prototype,"parse",1,parse,{},MaterialLoaderSource::parseSource);
    method(prototype,"setTextures",1,+[](const JSValue& self,const Args& args,const Args&)->JSValue {auto value=self;value.set("textures",arg(args,0));return self;},{},MaterialLoaderSource::setTexturesSource);
    method(prototype,"createMaterialFromType",1,+[](const JSValue&,const Args& args,const Args& captures)->JSValue {
        return invoke(captures[0],"createMaterialFromType")({arg(args,0)});
    },{ctor},MaterialLoaderSource::createMaterialFromTypeSource);
    method(ctor,"createMaterialFromType",1,staticCreate,{imports},MaterialLoaderSource::staticCreateMaterialFromTypeSource);
    method(ctor,"registerMaterial",2,registerMaterial,{imports},MaterialLoaderSource::registerMaterialSource);
    auto bindings=JSValue::object(nullptr);own(bindings,"MaterialLoader",ctor);
    return publish(bindings,lexical(),weak);
}
JSValue nativeMaterialLoaderBinding(const std::string& name){const auto keeper=nativeMaterialLoaderModule();return lexical().lock()->get(name);}
JSValue nativeMaterialLoaderConstructor(){return nativeMaterialLoaderBinding("MaterialLoader");}
JSValue MaterialLoader::prototype(){return nativeMaterialLoaderConstructor().get("prototype");}
JSValue MaterialLoader::createMaterialFromType(JSValue type){return invoke(nativeMaterialLoaderConstructor(),"createMaterialFromType")({type});}
void MaterialLoader::registerMaterial(JSValue type,JSValue materialClass){invoke(nativeMaterialLoaderConstructor(),"registerMaterial")({type,materialClass});}
MaterialLoader::MaterialLoader(JSValue manager):Loader(nativeMaterialLoaderConstructor().construct({manager}),Existing{}){}
std::shared_ptr<Material> MaterialLoader::parseMaterial(JSValue json) {
    const auto managed=Loader::parse(json);
    if(managed.isHostObject("three.Material"))return std::static_pointer_cast<Material>(managed.hostPayload("three.Material"));
    auto material=numericMaterial(managed.get("type").toString());
    OrderedMap<std::shared_ptr<Texture>> dictionary;const auto textures=object_.get("textures");
    for(const auto& key:textures.keys()) {
        const auto texture=textures.get(key);
        dictionary[key]=texture.isNull()||texture.isUndefined()?nullptr:std::static_pointer_cast<Texture>(texture.hostPayload("three.Texture"));
    }
    material->fromJSON(json.toJSON().value(),dictionary);return material;
}
MaterialLoader& MaterialLoader::setTextures(JSValue value){invoke(object_,"setTextures")({value});return *this;}
MaterialLoader& MaterialLoader::setTextures(const OrderedMap<std::shared_ptr<Texture>>& value){auto object=JSValue::object();for(const auto& [key,texture]:value)object.set(key,nativeTexture(texture));return setTextures(object);}
