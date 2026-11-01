#include "materials/ShaderMaterial.h"
#include "renderers/shaders/UniformsUtils.h"
#include "core/NativeObject.h"
#include "textures/Texture.h"
#include "math/Vector4.h"
#include "math/Matrix3.h"
#include "math/Matrix4.h"
#include <iostream>
#include <unordered_set>

namespace {
JSValue enumerableObject(const JSValue& value) {
    if (value.isObject()) return value;
    if (value.isNull() || value.isUndefined()) return JSValue::object(nullptr);
    auto result = JSValue::object();
    if (value.type() == JSValue::Type::String) {
        const auto length = value.get("length").toNumber();
        for (double index = 0; index < length; ++index) {
            const auto key = JSValue(index).toString();
            JSPropertyDescriptor descriptor; descriptor.value = value.get(key);
            descriptor.enumerable = true; descriptor.writable = false; descriptor.configurable = false;
            result.defineProperty(key,descriptor);
        }
        JSPropertyDescriptor descriptor; descriptor.value = length;
        descriptor.enumerable = false; descriptor.writable = false; descriptor.configurable = false;
        result.defineProperty("length",descriptor);
    }
    return result;
}
std::vector<std::string> enumerableKeys(JSValue object) {
    std::vector<std::string> keys;
    std::unordered_set<std::string> seen;
    if (!object.isObject()) return keys;
    for (auto current = object; current.isObject(); current = current.getPrototype()) {
        for (const auto& key : current.ownPropertyNames()) {
            if (!seen.insert(key).second) continue;
            const auto descriptor = current.getOwnPropertyDescriptor(key);
            if (descriptor && descriptor->enumerable.value_or(false)) keys.push_back(key);
        }
    }
    return keys;
}
JSValue shallowCopy(const JSValue& source) {
    auto result = JSValue::object();
    const auto object = enumerableObject(source);
    for (const auto& key : object.ownPropertyNames()) {
        const auto descriptor = object.getOwnPropertyDescriptor(key);
        if (descriptor && descriptor->enumerable.value_or(false)) result.set(key, object.get(key,source));
    }
    return result;
}

}

ShaderMaterial::ShaderMaterial(const JsonValue& parameters) {
    type = "ShaderMaterial";
    vertexShader = "\nvoid main() {\n\tgl_Position = projectionMatrix * modelViewMatrix * vec4( position, 1.0 );\n}\n";
    fragmentShader = "\nvoid main() {\n\tgl_FragColor = vec4( 1.0, 0.0, 0.0, 1.0 );\n}\n";
    forceSinglePass = true;
    extensions = JSValue::fromJSON(JsonValue::Object{{"clipCullDistance", false}, {"multiDraw", false}});
    defaultAttributeValues = JSValue::fromJSON(JsonValue::Object{
        {"color", JsonValue::Array{1, 1, 1}}, {"uv", JsonValue::Array{0, 0}}, {"uv1", JsonValue::Array{0, 0}}});
    setValues(parameters);
}
void ShaderMaterial::visitProperties(const PropertyVisitor& visitor) {
    Material::visitProperties(visitor);
    visitor("isShaderMaterial", &isShaderMaterial);
    visitor("defines", &defines);
    visitor("uniforms", &uniforms);
    visitor("uniformsGroups", &uniformsGroups);
    visitor("vertexShader", &vertexShader);
    visitor("fragmentShader", &fragmentShader);
    visitor("linewidth", &linewidth);
    visitor("wireframe", &wireframe);
    visitor("wireframeLinewidth", &wireframeLinewidth);
    visitor("fog", &fog);
    visitor("lights", &lights);
    visitor("clipping", &clipping);
    visitor("extensions", &extensions);
    visitor("defaultAttributeValues", &defaultAttributeValues);
    visitor("index0AttributeName", &index0AttributeName);
    visitor("uniformsNeedUpdate", &uniformsNeedUpdate);
    visitor("glslVersion", &glslVersion);
}
Material& ShaderMaterial::copy(const Material& source) {
    const auto& s = dynamic_cast<const ShaderMaterial&>(source);
    Material::copy(source);
    fragmentShader = s.fragmentShader;
    vertexShader = s.vertexShader;
    uniforms = cloneUniforms(s.uniforms);
    uniformsGroups = cloneUniformsGroups(s.uniformsGroups);
    defines = shallowCopy(s.defines);
    wireframe = s.wireframe;
    wireframeLinewidth = s.wireframeLinewidth;
    fog = s.fog;
    lights = s.lights;
    clipping = s.clipping;
    extensions = shallowCopy(s.extensions);
    glslVersion = s.glslVersion;
    defaultAttributeValues = shallowCopy(s.defaultAttributeValues);
    index0AttributeName = s.index0AttributeName;
    uniformsNeedUpdate = s.uniformsNeedUpdate;
    return *this;
}
std::shared_ptr<Material> ShaderMaterial::clone() const {
    auto result = std::make_shared<ShaderMaterial>();
    result->copy(*this);
    return result;
}
JsonValue ShaderMaterial::toJSON(TextureJSONContext* meta) const {
    auto result = Material::toJSON(meta);
    if (const auto version = glslVersion.toJSON()) result["glslVersion"] = *version;
    result["uniforms"] = JsonValue::Object{};
    const auto uniformObject = enumerableObject(uniforms);
    for (const auto& name : enumerableKeys(uniformObject)) {
        if (!uniformObject.has(name)) continue;
        const auto value = uniformObject.get(name,uniforms).get("value");
        JsonValue entry = JsonValue::Object{};
        if (value.toBoolean() && value.get("isTexture").toBoolean()) {
            entry["type"] = "t";
            if (value.isHostObject("three.Texture"))
                entry["value"] = std::static_pointer_cast<Texture>(value.hostPayload("three.Texture"))->toJSON(meta).at("uuid");
            else entry["value"] = *value.get("toJSON").call(value).get("uuid").toJSON();
        } else if (value.toBoolean() && value.get("isColor").toBoolean()) {
            entry["type"] = "c";
            entry["value"] = *value.get("getHex").call(value).toJSON();
        } else {
            const char* type = nullptr;
            if (value.toBoolean()) for (const auto& [flag, code] : {
                    std::pair{"isVector2", "v2"}, {"isVector3", "v3"}, {"isVector4", "v4"}, {"isMatrix3", "m3"}, {"isMatrix4", "m4"}})
                if (value.get(flag).toBoolean()) { type = code; break; }
            if (type) { entry["type"] = type; entry["value"] = *value.get("toArray").call(value).toJSON(); }
            else if (const auto json = value.toJSON()) entry["value"] = *json;
        }
        result["uniforms"][name] = std::move(entry);
    }
    if (defines.isNull() || defines.isUndefined()) throw JSTypeError("Cannot convert undefined or null to object");
    if (!enumerableObject(defines).keys().empty()) result["defines"] = *defines.toJSON();
    result["vertexShader"] = vertexShader;
    result["fragmentShader"] = fragmentShader;
    result["lights"] = lights;
    result["clipping"] = clipping;
    JsonValue selected = JsonValue::Object{};
    const auto extensionObject = enumerableObject(extensions);
    for (const auto& key : enumerableKeys(extensionObject))
        if (extensionObject.has(key) && extensionObject.get(key,extensions).strictEquals(true)) selected[key] = true;
    if (selected.size()) result["extensions"] = std::move(selected);
    return result;
}
Material& ShaderMaterial::fromJSON(const JsonValue& json, const OrderedMap<std::shared_ptr<Texture>>& textures) {
    Material::fromJSON(json, textures);
    if (json.contains("uniforms")) for (const auto& [name, uniform] : json.at("uniforms").get<JsonValue::Object>()) {
        uniforms.set(name, JSValue::object());
        const auto typeValue = uniform.contains("type") ? JSValue::fromJSON(uniform.at("type")) : JSValue();
        const auto code = typeValue.type() == JSValue::Type::String ? typeValue.string() : "";
        const auto value = uniform.contains("value") ? JSValue::fromJSON(uniform.at("value")) : JSValue();
        JSValue target;
        if (code == "t") {
            const auto texture = textures.find(value.toString());
            target = texture == textures.end() ? JSValue(nullptr) : nativeTexture(texture->second);
        } else if (code == "c") target = nativeColor(std::make_shared<Color>(value.toNumber()));
        else if (code == "v2") target = nativeVector(std::make_shared<Vector2>());
        else if (code == "v3") target = nativeVector(std::make_shared<Vector3>());
        else if (code == "v4") target = nativeVector(std::make_shared<Vector4>());
        else if (code == "m3") target = nativeMatrix(std::make_shared<Matrix3>());
        else if (code == "m4") target = nativeMatrix(std::make_shared<Matrix4>());
        else target = value;
        if (code == "v2" || code == "v3" || code == "v4" || code == "m3" || code == "m4") target.get("fromArray").call(target, {value});
        uniforms.get(name).set("value", target);
    }
    if (json.contains("defines")) defines = JSValue::fromJSON(json.at("defines"));
    if (json.contains("vertexShader")) vertexShader = json.at("vertexShader").get<std::string>();
    if (json.contains("fragmentShader")) fragmentShader = json.at("fragmentShader").get<std::string>();
    if (json.contains("glslVersion")) glslVersion = JSValue::fromJSON(json.at("glslVersion"));
    if (json.contains("extensions")) for (const auto& [key, value] : json.at("extensions").get<JsonValue::Object>()) extensions.set(key, JSValue::fromJSON(value));
    if (json.contains("lights")) lights = json.at("lights").get<bool>();
    if (json.contains("clipping")) clipping = json.at("clipping").get<bool>();
    return *this;
}

// Managed source-class authority. Original typed implementation above remains byte-exact.
#include "core/JSModuleObject.h"
#include "materials/MaterialClassRuntime.h"
#include "materials/Material.h"
#include "renderers/shaders/UniformsUtils.h"
#include "math/Color.h"
#include "math/Vector2.h"
#include "math/Vector3.h"
#include "math/Vector4.h"
#include "math/Matrix3.h"
#include "math/Matrix4.h"
namespace ShaderMaterialManaged {
using namespace MaterialClassRuntime;
#include "ShaderMaterialManagedSource.inc"
#include "ShaderMaterialManagedStatements.inc"
JSWeakValue& lexical(){static thread_local JSWeakValue value;return value;}
}
JSValue nativeShaderMaterialConstructor(){
using namespace ShaderMaterialManaged;static thread_local JSWeakValue weak;if(const auto cached=weak.lock();cached&&cached->isConstructible())return *cached;static thread_local bool active=false;JSModuleInitializationScope scope(active,"ShaderMaterial");
auto imports=JSValue::object(nullptr);own(imports,"Material",nativeMaterialConstructor());
own(imports,"cloneUniforms",nativeCloneUniforms());
own(imports,"cloneUniformsGroups",nativeCloneUniformsGroups());
own(imports,"Color",nativeColorConstructor());
own(imports,"Vector2",nativeVector2Constructor());
own(imports,"Vector3",nativeVector3Constructor());
own(imports,"Vector4",nativeVector4Constructor());
own(imports,"Matrix3",nativeMatrix3Constructor());
own(imports,"Matrix4",nativeMatrix4Constructor());
own(imports,"default_vertex",JSValue("\nvoid main() {\n\tgl_Position = projectionMatrix * modelViewMatrix * vec4( position, 1.0 );\n}\n"));
own(imports,"default_fragment",JSValue("\nvoid main() {\n\tgl_FragColor = vec4( 1.0, 0.0, 0.0, 1.0 );\n}\n"));
const auto base=imports.get("Material");auto holder=JSValue::object(nullptr);own(holder,"imports",imports);
auto ctor=JSValue::builtinConstructor(+[](const JSValue& target,const Args& args,const Args& captures)->JSValue{const auto holder=captures[0];const auto parent=holder.get("constructor").getPrototype();auto self=parent.construct({},target);return ShaderMaterialManaged::constructor(self,args,holder.get("imports"),JSValue{});},{holder},"ShaderMaterial",1);own(holder,"constructor",ctor);ctor.setPrototype(base);auto prototype=ctor.get("prototype");prototype.setPrototype(base.get("prototype"));ctor.setFunctionSourceText(ShaderMaterialSource::Class);
{auto method=JSValue::function(+[](const JSValue& self,const Args& args,const Args& captures)->JSValue{return ShaderMaterialManaged::copy(self,args,captures[0],captures[1]);},{imports,prototype},"copy",1);method.setFunctionSourceText(ShaderMaterialSource::copy);auto descriptor=prototype.getOwnPropertyDescriptor("copy").value_or(JSPropertyDescriptor{});descriptor.configurable=true;descriptor.value=method;descriptor.writable=true;prototype.defineProperty("copy",descriptor);}
{auto method=JSValue::function(+[](const JSValue& self,const Args& args,const Args& captures)->JSValue{return ShaderMaterialManaged::toJSON(self,args,captures[0],captures[1]);},{imports,prototype},"toJSON",1);method.setFunctionSourceText(ShaderMaterialSource::toJSON);auto descriptor=prototype.getOwnPropertyDescriptor("toJSON").value_or(JSPropertyDescriptor{});descriptor.configurable=true;descriptor.value=method;descriptor.writable=true;prototype.defineProperty("toJSON",descriptor);}
{auto method=JSValue::function(+[](const JSValue& self,const Args& args,const Args& captures)->JSValue{return ShaderMaterialManaged::fromJSON(self,args,captures[0],captures[1]);},{imports,prototype},"fromJSON",2);method.setFunctionSourceText(ShaderMaterialSource::fromJSON);auto descriptor=prototype.getOwnPropertyDescriptor("fromJSON").value_or(JSPropertyDescriptor{});descriptor.configurable=true;descriptor.value=method;descriptor.writable=true;prototype.defineProperty("fromJSON",descriptor);}
ctor.retainForHeapLifetime();weak=JSWeakValue(ctor);return ctor;}
JSValue nativeShaderMaterialModule(){using namespace ShaderMaterialManaged;static thread_local JSWeakValue weak;if(const auto cached=weak.lock();cached&&cached->isObject())return *cached;auto bindings=JSValue::object(nullptr);own(bindings,"ShaderMaterial",nativeShaderMaterialConstructor());auto facade=JSValue::object();own(facade,"ShaderMaterial",bindings.get("ShaderMaterial"));bindings.retainForHeapLifetime();lexical()=JSWeakValue(bindings);facade.retainForHeapLifetime();weak=JSWeakValue(facade);return facade;}
JSValue nativeShaderMaterialBinding(const std::string& name){const auto keeper=nativeShaderMaterialModule();return ShaderMaterialManaged::lexical().lock()->get(name);}
