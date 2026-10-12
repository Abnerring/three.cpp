// Port of three.js MaterialLoader.js at 11738a9d55e502fb484cefffcea3a66dc44e9fe9.
#include "loaders/MaterialLoader.h"
#include "materials/Materials.h"
#include "core/NativeObject.h"
#include <iostream>
#include <set>
namespace {
using Args=std::vector<JSValue>;
JSValue arg(const Args& args,std::size_t index){return index<args.size()?args[index]:JSValue{};}
JSValue& customMaterials(){static thread_local auto value=JSValue::object();return value;}
template<class T> JSValue materialConstructor(const char* name) {
    return JSValue::constructor([](const JSValue&,const Args&,const Args&)->JSValue{return nativeMaterial(std::make_shared<T>());},{},name,0);
}
JSValue materialLibrary() {
    auto value=JSValue::object();
#define THREE_MATERIAL_TYPE(Type) value.set(#Type,materialConstructor<Type>(#Type));
    THREE_MATERIAL_TYPE(ShadowMaterial) THREE_MATERIAL_TYPE(SpriteMaterial) THREE_MATERIAL_TYPE(RawShaderMaterial)
    THREE_MATERIAL_TYPE(ShaderMaterial) THREE_MATERIAL_TYPE(PointsMaterial) THREE_MATERIAL_TYPE(MeshPhysicalMaterial)
    THREE_MATERIAL_TYPE(MeshStandardMaterial) THREE_MATERIAL_TYPE(MeshPhongMaterial) THREE_MATERIAL_TYPE(MeshToonMaterial)
    THREE_MATERIAL_TYPE(MeshNormalMaterial) THREE_MATERIAL_TYPE(MeshLambertMaterial) THREE_MATERIAL_TYPE(MeshDepthMaterial)
    THREE_MATERIAL_TYPE(MeshDistanceMaterial) THREE_MATERIAL_TYPE(MeshBasicMaterial) THREE_MATERIAL_TYPE(MeshMatcapMaterial)
    THREE_MATERIAL_TYPE(LineDashedMaterial) THREE_MATERIAL_TYPE(LineBasicMaterial) THREE_MATERIAL_TYPE(Material)
#undef THREE_MATERIAL_TYPE
    for(const auto& key:customMaterials().keys()) {
        JSPropertyDescriptor descriptor;descriptor.configurable=descriptor.enumerable=descriptor.writable=true;
        descriptor.value=customMaterials().get(key);value.defineProperty(key,descriptor);
    }
    return value;
}
}
JSValue MaterialLoader::createMaterialFromType(JSValue type) {
    const auto materialLib=materialLibrary();const auto property=type.toPrimitive(JSValue::PreferredType::String).toString();const auto materialType=materialLib.get(property);
    if(materialType.isUndefined()) {
        const auto message="MaterialLoader: Unknown material type \""+type.toString()+"\". Use .registerMaterial() before starting the deserialization process.";
        static thread_local std::set<std::string> warned;if(warned.insert(message).second)std::cerr<<"THREE."<<message<<'\n';
        return nativeMaterial(std::make_shared<Material>());
    }
    return materialType.construct();
}
void MaterialLoader::registerMaterial(JSValue type,JSValue materialClass){customMaterials().set(type.toPrimitive(JSValue::PreferredType::String).toString(),materialClass);}
JSValue MaterialLoader::prototype() {
    static thread_local auto value=[] {
        auto constructor=JSValue::constructor([](const JSValue& self,const Args& args,const Args&)->JSValue{initialize(self,arg(args,0));auto result=self;result.set("textures",JSValue::object());return {};},{},"MaterialLoader",1);
        constructor.setPrototype(Loader::prototype().get("constructor"));auto result=constructor.get("prototype");result.setPrototype(Loader::prototype());
        JSPropertyDescriptor descriptor;descriptor.configurable=descriptor.writable=true;
        descriptor.value=JSValue::function([](const JSValue&,const Args& args,const Args&)->JSValue{return createMaterialFromType(arg(args,0));},{},"createMaterialFromType",1);
        result.defineProperty("createMaterialFromType",descriptor);constructor.defineProperty("createMaterialFromType",descriptor);
        descriptor.value=JSValue::function([](const JSValue&,const Args& args,const Args&)->JSValue{registerMaterial(arg(args,0),arg(args,1));return {};},{},"registerMaterial",2);constructor.defineProperty("registerMaterial",descriptor);
        descriptor.value=JSValue::function([](const JSValue& self,const Args& args,const Args&)->JSValue {
            const auto json=arg(args,0),creator=self.get("createMaterialFromType"),type=json.get("type");
            const auto material=creator.call(self,{type});const auto fromJSON=material.get("fromJSON");
            const auto textures=self.get("textures");
            if(!fromJSON.isUndefined())fromJSON.call(material,{json,textures});
            else {
                const auto native=std::static_pointer_cast<Material>(material.hostPayload("three.Material"));
                OrderedMap<std::shared_ptr<Texture>> dictionary;
                for(const auto& key:textures.keys()) {
                    const auto texture=textures.get(key);if(texture.isNull()||texture.isUndefined())dictionary[key]=nullptr;
                    else dictionary[key]=std::static_pointer_cast<Texture>(texture.hostPayload("three.Texture"));
                }
                native->fromJSON(json.toJSON().value(),dictionary);
            }
            return material;
        },{},"parse",1);result.defineProperty("parse",descriptor);
        descriptor.value=JSValue::function([](const JSValue& self,const Args& args,const Args&)->JSValue{auto result=self;result.set("textures",arg(args,0));return result;},{},"setTextures",1);result.defineProperty("setTextures",descriptor);
        descriptor.value=JSValue::function([](const JSValue&,const Args&,const Args&)->JSValue{throw std::runtime_error("MaterialLoader.load requires the pending native FileLoader transport");},{},"load",4);result.defineProperty("load",descriptor);return result;
    }();return value;
}
MaterialLoader::MaterialLoader(JSValue manager):Loader(JSValue::object(prototype()),Existing{}){initialize(object_,manager);object_.set("textures",JSValue::object());}
std::shared_ptr<Material> MaterialLoader::parseMaterial(JSValue json){return std::static_pointer_cast<Material>(Loader::parse(json).hostPayload("three.Material"));}
MaterialLoader& MaterialLoader::setTextures(JSValue value){object_.get("setTextures").call(object_,{value});return *this;}
MaterialLoader& MaterialLoader::setTextures(const OrderedMap<std::shared_ptr<Texture>>& value){auto object=JSValue::object();for(const auto& [key,texture]:value)object.set(key,nativeTexture(texture));return setTextures(object);}
