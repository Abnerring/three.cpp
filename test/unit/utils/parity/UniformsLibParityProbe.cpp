#include "renderers/shaders/UniformsLib.h"
#include "renderers/shaders/UniformsUtils.h"
#include <iostream>

namespace {
void identities(const JSValue& value,const std::string& path,
                std::vector<std::pair<std::string,JSValue>>& seen,JsonValue::Array& result) {
    if (!value.isObject()) return;
    for (const auto& [previous,object] : seen) {
        if (object.sameValue(value)) { result.push_back(JsonValue::Array{path,previous}); return; }
    }
    seen.emplace_back(path,value); result.push_back(JsonValue::Array{path,path});
    for (const auto& key : value.ownPropertyNames()) {
        const auto descriptor = value.getOwnPropertyDescriptor(key);
        if (descriptor && descriptor->enumerable.value_or(false)) identities(value.get(key),path+"."+key,seen,result);
    }
}
JsonValue encode(const JSValue& value) {
    if (!value.isObject()) return value.toJSON().value_or(JsonValue("undefined"));
    for (const char* flag : {"isColor","isVector2","isVector3","isMatrix3"}) {
        if (value.get(flag).toBoolean()) return JsonValue::Object{
            {"kind",flag},{"data",value.get("toArray").call(value).toJSON().value()}};
    }
    if (value.type() == JSValue::Type::Array) {
        JsonValue::Array result;
        for (double i = 0; i < value.get("length").toNumber(); ++i) result.push_back(encode(value.get(JSValue(i).toString())));
        return result;
    }
    // Pairs retain property order, so the oracle checks order as well as values.
    JsonValue::Array pairs;
    for (const auto& key : value.ownPropertyNames()) {
        const auto descriptor = value.getOwnPropertyDescriptor(key);
        if (descriptor && descriptor->enumerable.value_or(false)) pairs.push_back(JsonValue::Array{key,encode(value.get(key))});
    }
    return JsonValue::Object{{"properties",pairs}};
}
}
int main() {
    JsonValue::Object result{{"defaults",encode(UniformsLib)}};
    JsonValue::Array identityPaths; std::vector<std::pair<std::string,JSValue>> seen;
    identities(UniformsLib,"lib",seen,identityPaths); result["identities"] = identityPaths;
    JsonValue::Array groups;
    for (const auto& name : UniformsLib.ownPropertyNames()) {
        const auto source = UniformsLib.get(name), cloned = cloneUniforms(source);
        JsonValue::Array aliases;
        for (const auto& uniform : source.ownPropertyNames()) {
            const auto before = source.get(uniform), after = cloned.get(uniform);
            JsonValue::Array fields;
            for (const auto& field : before.ownPropertyNames()) fields.push_back(JsonValue::Array{field,before.get(field).sameValue(after.get(field))});
            aliases.push_back(JsonValue::Object{{"uniform",uniform},{"same",before.sameValue(after)},{"fields",fields}});
        }
        groups.push_back(JsonValue::Object{{"name",name},{"clone",encode(cloned)},{"aliases",aliases}});
    }
    result["clones"] = groups;
    const auto common = UniformsLib.get("common"), copied = cloneUniforms(common);
    auto color = common.get("diffuse").get("value"); color.set("r",0.25);
    UniformsLib.collectGarbage();
    result["mutation"] = JsonValue::Object{
        {"shared",UniformsLib.get("common").sameValue(common)},
        {"original",UniformsLib.get("common").get("diffuse").get("value").get("r").number()},
        {"clone",copied.get("diffuse").get("value").get("r").number()}};
    std::cout << JsonValue(result).dump() << '\n';
}
