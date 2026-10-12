#include "materials/Materials.h"
#include "animation/AnimationUtils.h"
#include "core/NativeObject.h"
#include "textures/Texture.h"
#include "math/Plane.h"
#include <iostream>
#include <limits>

namespace {
struct CallbackTexture : Texture {
    std::function<void()> callback;
    JsonValue toJSON(TextureJSONContext* meta=nullptr) const override {
        if(callback)callback();return Texture::toJSON(meta);
    }
};
std::shared_ptr<Material> create(const std::string& type) {
#define MAKE(T) if (type == #T) return std::make_shared<T>();
    MAKE(Material) MAKE(LineBasicMaterial) MAKE(LineDashedMaterial) MAKE(MeshBasicMaterial)
    MAKE(MeshDepthMaterial) MAKE(MeshDistanceMaterial) MAKE(MeshLambertMaterial) MAKE(MeshMatcapMaterial)
    MAKE(MeshNormalMaterial) MAKE(MeshPhongMaterial) MAKE(MeshStandardMaterial) MAKE(MeshPhysicalMaterial)
    MAKE(MeshToonMaterial) MAKE(PointsMaterial) MAKE(ShadowMaterial) MAKE(SpriteMaterial)
    MAKE(ShaderMaterial) MAKE(RawShaderMaterial)
#undef MAKE
    throw std::invalid_argument(type);
}
JsonValue encode(const JSValue& value) {
    if (value.isUndefined()) return "$undefined";
    if (value.type() == JSValue::Type::Number) {
        const auto number = value.number();
        if (std::isnan(number)) return "$NaN";
        if (std::isinf(number)) return number < 0 ? "$-Infinity" : "$Infinity";
        if (number == 0 && std::signbit(number)) return "$-0";
    }
    if (!value.isObject()) return *value.toJSON();
    if (value.get("isTexture").toBoolean()) return JsonValue::Object{{"texture", value.get("uuid").string()}};
    for (const auto* flag : {"isColor", "isVector2", "isVector3", "isVector4", "isMatrix3", "isMatrix4", "isQuaternion"})
        if (value.get(flag).toBoolean()) return JsonValue::Object{{"kind",flag}, {"components", *value.get("toArray").call(value).toJSON()}};
    if (value.type() == JSValue::Type::Array) {
        JsonValue::Array result;
        for (std::uint32_t i=0;i<value.length();++i) result.push_back(encode(value.get(std::to_string(i))));
        return result;
    }
    JsonValue result = JsonValue::Object{};
    for (const auto& key : value.keys()) if (!value.get(key).isCallable()) result[key] = encode(value.get(key));
    return result;
}
JsonValue snapshot(Material& material) {
    JsonValue fields = JsonValue::Object{{"isMaterial", true}};
    material.visitProperties([&](std::string_view name, Material::PropertyReference property) {
        if (name == "uuid") return;
        fields[std::string(name)] = std::visit([](auto* field)->JsonValue {
            using T = std::remove_cvref_t<decltype(*field)>;
            if constexpr (std::is_same_v<T, Color>) return JsonValue::Object{{"kind","isColor"},{"components",JsonValue::Array{field->r,field->g,field->b}}};
            else if constexpr (std::is_same_v<T, Vector2>) return JsonValue::Object{{"kind","isVector2"},{"components",JsonValue::Array{field->x,field->y}}};
            else if constexpr (std::is_same_v<T, Euler>) return JsonValue::Array{field->x,field->y,field->z,field->getOrderString()};
            else if constexpr (std::is_same_v<T, Material::AlphaTest> || std::is_same_v<T, Material::Reflectivity>) return encode(JSValue(double(*field)));
            else if constexpr (std::is_same_v<T, std::shared_ptr<Texture>>) return *field ? JsonValue::Object{{"texture",(*field)->uuid}} : JsonValue();
            else if constexpr (std::is_same_v<T, JSValue>) return encode(*field);
            else if constexpr (std::is_same_v<T, JsonValue>) return field->copyJSON();
            else if constexpr (std::is_same_v<T, std::optional<int>> || std::is_same_v<T,std::optional<std::string>>) return *field ? JsonValue(**field) : JsonValue();
            else if constexpr (std::is_same_v<T,Material::ClippingPlanes>) {
                if (!*field) return nullptr;
                JsonValue::Array a;for(const auto& p:**field)a.push_back(p->toJSON());return a;
            } else if constexpr (std::is_same_v<T,double>) return encode(JSValue(*field));
            else return *field;
        },property);
    });
    for(const auto& name:material.extraProperties.keys())fields[name]=encode(material.extraProperties.get(name));
    const auto saved = material.uuid; material.uuid = "material";
    auto json = material.toJSON(); material.uuid = saved;
    return JsonValue::Object{{"fields",fields},{"json",json}};
}
JsonValue runtimeCase(const JSValue& request) {
    const auto mode=request.get("runtime").string();
    if(mode=="physical") {
        auto material=std::make_shared<MeshPhysicalMaterial>();auto object=nativeMaterial(material);JsonValue::Array versions;
        for(const auto* key:{"anisotropy","clearcoat","diffuseRoughness","dispersion","iridescence","retroreflectivity","sheen","transmission","alphaTest"})
            for(double value:{0.,.4,.6,0.,-1.,1.}){object.set(key,value);versions.push_back(object.get("version").number());}
        object.set("reflectivity",.8);const auto d=*object.getOwnPropertyDescriptor("reflectivity");
        return JsonValue::Object{{"versions",versions},{"ior",material->ior},{"reflectivity",double(material->reflectivity)},
            {"own",object.hasOwn("reflectivity")},{"enumerable",d.enumerable.value_or(false)},{"configurable",d.configurable.value_or(false)},
            {"alphaOwn",object.hasOwn("alphaTest")},{"backingOwn",object.hasOwn("_alphaTest")},{"same",object.sameValue(nativeMaterial(material))}};
    }
    if(mode=="setValues") {
        auto material=std::make_shared<MeshPhysicalMaterial>();auto object=nativeMaterial(material);
        const auto color=object.get("color"),normal=object.get("normalScale"),rotation=object.get("envMapRotation");
        auto parameters=JSValue::object();parameters.set("color",0x123456);parameters.set("normalScale",nativeVector(std::make_shared<Vector2>(3,4)));
        parameters.set("envMapRotation",nativeEuler(std::make_shared<Euler>(.2,.3,.4)));parameters.set("needsUpdate",true);
        parameters.set("clearcoat",.5);parameters.set("opacity",.2);
        object.get("setValues").call(object,{parameters});
        const auto saved=material->version;object.set("needsUpdate",true);object.set("needsUpdate",1);
        auto callback=JSValue::function([](const JSValue&,const std::vector<JSValue>&,const std::vector<JSValue>&){return JSValue();});
        callback.set("toString",JSValue::function([](const JSValue&,const std::vector<JSValue>&,const std::vector<JSValue>&){return JSValue("custom program");}));
        object.set("onBeforeCompile",callback);
        return JsonValue::Object{{"colorSame",color.sameValue(object.get("color"))},{"vectorSame",normal.sameValue(object.get("normalScale"))},
            {"eulerSame",rotation.sameValue(object.get("envMapRotation"))},{"color",material->color.getHex()},
            {"normal",JsonValue::Array{material->normalScale.x,material->normalScale.y}},
            {"rotation",JsonValue::Array{material->envMapRotation.x,material->envMapRotation.y,material->envMapRotation.z}},
            {"versionBeforeDirect",saved},{"version",material->version},{"key",object.get("customProgramCacheKey").call(object).string()}};
    }
    if(mode=="extra") {
        auto material=std::make_shared<Material>();material->fromJSON(JsonValue::Object{{"roughness",.3}});auto object=nativeMaterial(material);
        auto shared=JSValue::object();shared.set("value",4);object.set("custom",shared);
        object.set("roughness",.7);auto trace=JSValue::array();JSPropertyDescriptor descriptor;descriptor.enumerable=true;descriptor.configurable=true;
        descriptor.getter=JSValue::function([](const JSValue& self,const std::vector<JSValue>&,const std::vector<JSValue>& c){c[0].get("push").call(c[0],{self.get("roughness")});return self.get("roughness");},{trace});
        object.defineProperty("getter",descriptor);
        const auto got=object.get("getter");const bool removed=object.erase("roughness");
        return JsonValue::Object{{"same",material->extraProperties.get("custom").sameValue(shared)},
            {"got",got.number()},{"trace",*trace.toJSON()},{"removed",removed},{"gone",!object.has("roughness")},
            {"descriptor",object.getOwnPropertyDescriptor("getter")->enumerable.value_or(false)}};
    }
    auto material=std::make_shared<ShaderMaterial>();auto object=nativeMaterial(material);auto uniforms=JSValue::object();auto value=JSValue::object();value.set("x",2);
    auto u=JSValue::object();u.set("value",value);uniforms.set("value",u);object.set("uniforms",uniforms);
    material->uniforms.get("value").get("value").get("x");auto clone=object.get("clone").call(object);
    return JsonValue::Object{{"same",object.get("uniforms").sameValue(uniforms)},{"nativeSame",material->uniforms.sameValue(uniforms)},
        {"cloneDifferent",!clone.get("uniforms").sameValue(uniforms)}, {"ordinaryShared",clone.get("uniforms").get("value").get("value").sameValue(value)}};
}
JsonValue run(JSValue request) {
    if(request.hasOwn("runtime"))return runtimeCase(request);
    auto material=create(request.get("type").string());
    if(request.hasOwn("parameters"))material->setValues(*request.get("parameters").toJSON());
    OrderedMap<std::shared_ptr<Texture>> textures;
    auto texture=std::make_shared<CallbackTexture>();texture->uuid="texture";texture->source->uuid="image";textures[texture->uuid]=texture;
    if(request.get("textures").toBoolean())material->visitProperties([&](std::string_view,Material::PropertyReference p){if(auto slot=std::get_if<std::shared_ptr<Texture>*>(&p))**slot=texture;});
    if(request.get("reentrantTexture").toBoolean()) {
        const std::weak_ptr<Material> weak=material;
        texture->callback=[weak] {
            const auto m=std::dynamic_pointer_cast<MeshPhysicalMaterial>(weak.lock());
            if(!m)return;
            m->opacity=.3;m->clearcoat=.9;m->dispersion=.4;m->sheen=.2;
            m->clearcoatNormalScale.set(2,3);m->envMapIntensity=7;
            m->userData["callback"]=true;m->fog=false;
        };
    }
    if(request.hasOwn("load"))material->fromJSON(*request.get("load").toJSON(),textures);
    JsonValue::Array versions;
    if(request.hasOwn("toggles"))for(std::uint32_t i=0;i<request.get("toggles").length();++i){
        const auto step=request.get("toggles").get(std::to_string(i));material->setValues(*step.toJSON());versions.push_back(encode(material->version));
    }
    if(auto shader=std::dynamic_pointer_cast<ShaderMaterial>(material);shader && request.get("uniformTypes").toBoolean()) {
        auto add=[&](const char* name,JSValue value){auto uniform=JSValue::object();uniform.set("value",value);shader->uniforms.set(name,uniform);};
        add("texture",nativeTexture(texture));add("color",nativeColor(std::make_shared<Color>(0x123456)));
        add("vector",nativeVector(std::make_shared<Vector2>(3,4)));
        auto nested=JSValue::object();nested.set("a",2);add("object",nested);
        auto a=JSValue::array(3);a.set("1",nested);add("array",a);
        auto vectors=JSValue::array();vectors.set("0",nativeVector(std::make_shared<Vector2>(7,8)));vectors.set("1",nativeVector(std::make_shared<Vector2>(9,10)));add("vectors",vectors);
    }
    JsonValue out=JsonValue::Object{{"source",snapshot(*material)}, {"versions",versions}};
    auto clone=material->clone();
    auto target=create(request.get("type").string());target->name="target";target->version=100;target->alphaTest=.7;target->copy(*material);
    if(request.get("uniformTypes").toBoolean())for(const auto& m:{clone,target}) {
        const auto s=std::dynamic_pointer_cast<ShaderMaterial>(m);
        std::static_pointer_cast<Texture>(s->uniforms.get("texture").get("value").hostPayload("three.Texture"))->uuid="texture-clone";
    }
    out["clone"]=snapshot(*clone);out["copy"]=snapshot(*target);
    JsonValue::Object identity{{"uuidDifferent",clone->uuid!=material->uuid},{"idDifferent",clone->id!=material->id}};
    material->visitProperties([&](std::string_view name,Material::PropertyReference p){
        if(const auto slot=std::get_if<std::shared_ptr<Texture>*>(&p)) {
            const auto other=clone->property(name);identity[std::string(name)]=**slot==*std::get<std::shared_ptr<Texture>*>(*other);
        }
    });
    for(const auto& name:material->extraProperties.keys())if(name=="map"||name=="matcap"||name.ends_with("Map"))
        identity[name]=material->extraProperties.get(name).sameValue(clone->extraProperties.get(name));
    if(auto shader=std::dynamic_pointer_cast<ShaderMaterial>(material)) {
        auto other=std::dynamic_pointer_cast<ShaderMaterial>(clone);
        identity["uniformsDifferent"]=!shader->uniforms.sameValue(other->uniforms);
        identity["definesDifferent"]=!shader->defines.sameValue(other->defines);
        identity["attributeArrayShared"]=shader->defaultAttributeValues.get("color").sameValue(other->defaultAttributeValues.get("color"));
        if(request.get("uniformTypes").toBoolean())for(const char* key:{"texture","color","vector","object","array","vectors"})
            identity[std::string("uniform:")+key]=shader->uniforms.get(key).get("value").sameValue(other->uniforms.get(key).get("value"));
    }
    if(auto physical=std::dynamic_pointer_cast<MeshPhysicalMaterial>(material))identity["iridescenceRangeDifferent"]=!physical->iridescenceThicknessRange.sameValue(std::dynamic_pointer_cast<MeshPhysicalMaterial>(clone)->iridescenceThicknessRange);
    out["identity"]=identity;
    return out;
}
}
int main() {
    try {
        const std::string input(std::istreambuf_iterator<char>(std::cin),{});
        const auto cases=Animation_detail::parseJSON(input);JsonValue::Array out;
        for(std::uint32_t i=0;i<cases.length();++i)out.push_back(run(cases.get(std::to_string(i))));
        std::cout<<JsonValue(out).dump()<<'\n';
    }catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
}
