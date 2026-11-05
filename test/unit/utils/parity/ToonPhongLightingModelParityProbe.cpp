#include "nodes/functions/PhongLightingModel.h"
#include "nodes/functions/ToonLightingModel.h"

#include "core/JSValue.h"
#include "nodes/tsl/TSLBase.h"

#include <iostream>
#include <string>
#include <vector>

namespace {
using ArgList = std::vector<JSValue>;

JSValue vec3Node(double x, double y, double z) {
    return nativeTSLBaseBinding("vec3").call({}, {JSValue(x), JSValue(y), JSValue(z)});
}

JSValue counter(const char* first, const char* second) {
    auto value = JSValue::object(nullptr);
    value.set("addAssignCount", 0);
    value.set("mulAssignCount", 0);
    for (const char* name : {first, second}) {
        const std::string key = name;
        value.set(name, JSValue::function(
            +[](const JSValue&, const ArgList&, const ArgList& captures) -> JSValue {
                auto holder = captures[0];
                const std::string field = captures[1].string() + "Count";
                holder.set(field.c_str(), JSValue(holder.get(field.c_str()).toNumber() + 1));
                return {};
            },
            {value, JSValue(key)}, name, 1));
    }
    return value;
}

struct ReflectedLight {
    JSValue object, diffuse, specular, indirectDiffuse;
    ReflectedLight() {
        diffuse = counter("addAssign", "mulAssign");
        specular = counter("addAssign", "mulAssign");
        indirectDiffuse = counter("addAssign", "mulAssign");
        object = JSValue::object(nullptr);
        object.set("directDiffuse", diffuse);
        object.set("directSpecular", specular);
        object.set("indirectDiffuse", indirectDiffuse);
    }
};

JSValue directInputs(const ReflectedLight& reflected) {
    auto inputs = JSValue::object(nullptr);
    inputs.set("lightDirection", vec3Node(0, 0, 1));
    inputs.set("lightColor", vec3Node(1, 1, 1));
    inputs.set("reflectedLight", reflected.object);
    return inputs;
}

JSValue builderWithContext(const ReflectedLight& reflected) {
    auto context = JSValue::object(nullptr);
    context.set("reflectedLight", reflected.object);
    context.set("ambientOcclusion", nativeTSLBaseBinding("float").call({}, {JSValue(1.0)}));
    context.set("irradiance", vec3Node(1, 1, 1));
    auto material = JSValue::object(nullptr);
    auto builder = JSValue::object(nullptr);
    builder.set("context", context);
    builder.set("material", material);
    return builder;
}
}  // namespace

int main() {
    try {
        const auto toonCtor = nativeToonLightingModelConstructor();
        const auto phongCtor = nativePhongLightingModelConstructor();
        const auto toon = toonCtor.construct();
        const auto phong = phongCtor.construct();
        const auto phongPlain = phongCtor.construct({JSValue(false)});

        ReflectedLight toonReflected;
        toon.get("direct").call(toon, {directInputs(toonReflected), builderWithContext(toonReflected)});
        toon.get("indirect").call(toon, {builderWithContext(toonReflected)});

        ReflectedLight phongReflected;
        phong.get("direct").call(phong, {directInputs(phongReflected), builderWithContext(phongReflected)});
        phong.get("indirect").call(phong, {builderWithContext(phongReflected)});

        ReflectedLight plainReflected;
        phongPlain.get("direct").call(phongPlain,
                                      {directInputs(plainReflected), builderWithContext(plainReflected)});

        auto output = JSValue::object(nullptr);
        output.set("toonName", toonCtor.get("name"));
        output.set("toonDirectLength", toon.get("direct").get("length"));
        output.set("toonIndirectLength", toon.get("indirect").get("length"));
        output.set("toonStartLength", toon.get("start").get("length"));
        output.set("toonFinishLength", toon.get("finish").get("length"));
        output.set("toonDirectDiffuse", toonReflected.diffuse.get("addAssignCount"));
        output.set("toonIndirectAdd", toonReflected.indirectDiffuse.get("addAssignCount"));
        output.set("toonIndirectMul", toonReflected.indirectDiffuse.get("mulAssignCount"));

        output.set("phongName", phongCtor.get("name"));
        output.set("phongCtorLength", phongCtor.get("length"));
        output.set("phongSpecular", phong.get("specular"));
        output.set("phongDirectLength", phong.get("direct").get("length"));
        output.set("phongIndirectLength", phong.get("indirect").get("length"));
        output.set("phongDirectDiffuse", phongReflected.diffuse.get("addAssignCount"));
        output.set("phongDirectSpecular", phongReflected.specular.get("addAssignCount"));
        output.set("phongIndirectAdd", phongReflected.indirectDiffuse.get("addAssignCount"));
        output.set("phongIndirectMul", phongReflected.indirectDiffuse.get("mulAssignCount"));
        output.set("plainSpecular", phongPlain.get("specular"));
        output.set("plainSpecularAssigns", plainReflected.specular.get("addAssignCount"));
        output.set("toonSource", toonCtor.get("toString").call(toonCtor));
        output.set("phongSource", phongCtor.get("toString").call(phongCtor));
        std::cout << output.toJSON()->dump() << '\n';
        return 0;
    } catch (...) {
        return 1;
    }
}
