#pragma once
#include <string>
#include <vector>
namespace MaterialsFixture {
inline const std::vector<std::string>& uniformsFields() {
    static const std::vector<std::string> fields{"fogColor",
                                                 "fogNear",
                                                 "fogFar",
                                                 "fogDensity",
                                                 "envMapIntensity",
                                                 "color",
                                                 "opacity",
                                                 "diffuse",
                                                 "emissive",
                                                 "map",
                                                 "mapTransform",
                                                 "alphaMap",
                                                 "alphaMapTransform",
                                                 "bumpMap",
                                                 "bumpMapTransform",
                                                 "bumpScale",
                                                 "normalMap",
                                                 "normalMapTransform",
                                                 "normalScale",
                                                 "displacementMap",
                                                 "displacementMapTransform",
                                                 "displacementScale",
                                                 "displacementBias",
                                                 "emissiveMap",
                                                 "emissiveMapTransform",
                                                 "specularMap",
                                                 "specularMapTransform",
                                                 "alphaTest",
                                                 "envMap",
                                                 "envMapRotation",
                                                 "reflectivity",
                                                 "ior",
                                                 "refractionRatio",
                                                 "lightMap",
                                                 "lightMapIntensity",
                                                 "lightMapTransform",
                                                 "aoMap",
                                                 "aoMapIntensity",
                                                 "aoMapTransform",
                                                 "dashSize",
                                                 "totalSize",
                                                 "scale",
                                                 "size",
                                                 "uvTransform",
                                                 "rotation",
                                                 "specular",
                                                 "shininess",
                                                 "gradientMap",
                                                 "metalness",
                                                 "metalnessMap",
                                                 "metalnessMapTransform",
                                                 "roughness",
                                                 "roughnessMap",
                                                 "roughnessMapTransform",
                                                 "diffuseRoughness",
                                                 "diffuseRoughnessMap",
                                                 "diffuseRoughnessMapTransform",
                                                 "sheenColor",
                                                 "sheenRoughness",
                                                 "sheenColorMap",
                                                 "sheenColorMapTransform",
                                                 "sheenRoughnessMap",
                                                 "sheenRoughnessMapTransform",
                                                 "clearcoat",
                                                 "clearcoatRoughness",
                                                 "clearcoatMap",
                                                 "clearcoatMapTransform",
                                                 "clearcoatRoughnessMap",
                                                 "clearcoatRoughnessMapTransform",
                                                 "clearcoatNormalMap",
                                                 "clearcoatNormalMapTransform",
                                                 "clearcoatNormalScale",
                                                 "dispersion",
                                                 "retroreflectivity",
                                                 "iridescence",
                                                 "iridescenceIOR",
                                                 "iridescenceThicknessMinimum",
                                                 "iridescenceThicknessMaximum",
                                                 "iridescenceMap",
                                                 "iridescenceMapTransform",
                                                 "iridescenceThicknessMap",
                                                 "iridescenceThicknessMapTransform",
                                                 "transmission",
                                                 "transmissionSamplerMap",
                                                 "transmissionSamplerSize",
                                                 "transmissionMap",
                                                 "transmissionMapTransform",
                                                 "thickness",
                                                 "thicknessMap",
                                                 "thicknessMapTransform",
                                                 "attenuationDistance",
                                                 "attenuationColor",
                                                 "anisotropyVector",
                                                 "anisotropyMap",
                                                 "anisotropyMapTransform",
                                                 "specularIntensity",
                                                 "specularColor",
                                                 "specularColorMap",
                                                 "specularColorMapTransform",
                                                 "specularIntensityMap",
                                                 "specularIntensityMapTransform",
                                                 "matcap",
                                                 "referencePosition",
                                                 "nearDistance",
                                                 "farDistance"};
    return fields;
}
inline const std::vector<std::string>& materialFields() {
    static const std::vector<std::string> fields{"isNodeMaterial",
                                                 "uniformsNeedUpdate",
                                                 "isMeshBasicMaterial",
                                                 "isMeshLambertMaterial",
                                                 "envMap",
                                                 "envMapIntensity",
                                                 "isMeshToonMaterial",
                                                 "isMeshPhongMaterial",
                                                 "isMeshStandardMaterial",
                                                 "isMeshPhysicalMaterial",
                                                 "isMeshMatcapMaterial",
                                                 "isMeshDepthMaterial",
                                                 "isMeshDistanceMaterial",
                                                 "isMeshNormalMaterial",
                                                 "isLineBasicMaterial",
                                                 "isLineDashedMaterial",
                                                 "isPointsMaterial",
                                                 "isSpriteMaterial",
                                                 "isShadowMaterial",
                                                 "color",
                                                 "opacity",
                                                 "isShaderMaterial",
                                                 "emissive",
                                                 "emissiveIntensity",
                                                 "map",
                                                 "alphaMap",
                                                 "bumpMap",
                                                 "bumpScale",
                                                 "side",
                                                 "normalMap",
                                                 "normalScale",
                                                 "displacementMap",
                                                 "displacementScale",
                                                 "displacementBias",
                                                 "emissiveMap",
                                                 "specularMap",
                                                 "alphaTest",
                                                 "reflectivity",
                                                 "ior",
                                                 "refractionRatio",
                                                 "lightMap",
                                                 "lightMapIntensity",
                                                 "aoMap",
                                                 "aoMapIntensity",
                                                 "dashSize",
                                                 "gapSize",
                                                 "scale",
                                                 "size",
                                                 "rotation",
                                                 "specular",
                                                 "shininess",
                                                 "gradientMap",
                                                 "metalness",
                                                 "metalnessMap",
                                                 "roughness",
                                                 "roughnessMap",
                                                 "diffuseRoughness",
                                                 "diffuseRoughnessMap",
                                                 "sheen",
                                                 "sheenColor",
                                                 "sheenRoughness",
                                                 "sheenColorMap",
                                                 "sheenRoughnessMap",
                                                 "clearcoat",
                                                 "clearcoatRoughness",
                                                 "clearcoatMap",
                                                 "clearcoatRoughnessMap",
                                                 "clearcoatNormalMap",
                                                 "clearcoatNormalScale",
                                                 "dispersion",
                                                 "retroreflectivity",
                                                 "iridescence",
                                                 "iridescenceIOR",
                                                 "iridescenceThicknessRange",
                                                 "iridescenceMap",
                                                 "iridescenceThicknessMap",
                                                 "transmission",
                                                 "transmissionMap",
                                                 "thickness",
                                                 "thicknessMap",
                                                 "attenuationDistance",
                                                 "attenuationColor",
                                                 "anisotropy",
                                                 "anisotropyRotation",
                                                 "anisotropyMap",
                                                 "specularIntensity",
                                                 "specularColor",
                                                 "specularColorMap",
                                                 "specularIntensityMap",
                                                 "matcap"};
    return fields;
}
} // namespace MaterialsFixture

#include "animation/AnimationUtils.h"
#include "core/NativeObject.h"
#include "parity/WebGLClippingFixture.h"
#include "renderers/webgl/WebGLMaterials.h"
#include <limits>
namespace MaterialsFixture {
using ClippingFixture::A;
using ClippingFixture::Args;
using ClippingFixture::array;
using ClippingFixture::invoke;
using ClippingFixture::O;
using ClippingFixture::object;
using ClippingFixture::prop;
inline JSValue option(JSValue spec, const char* key, JSValue fallback = {}) {
    const auto value = spec.get(key);
    return value.isUndefined() ? fallback : value;
}
inline JsonValue describe(const JSValue& v) {
    if (!v.isObject())
        return ClippingFixture::encode(v);
    if (v.hasOwn("label"))
        return O{{"ref", v.get("label").toString()}};
    if (v.type() == JSValue::Type::Array) {
        A out;
        for (unsigned i = 0; i < v.get("length").toNumber(); ++i)
            out.push_back(describe(v.get(std::to_string(i))));
        return out;
    }
    if (v.has("elements"))
        return O{{"matrix", describe(v.get("elements"))}};
    return "object";
}
inline void tick(JSValue state, const std::string& text) {
    invoke(state.get("log"), "push", {text});
    const auto index = state.get("count").toNumber() + 1;
    state.set("count", index);
    if (index == state.get("throwAt").toNumber())
        throw JSTypeError("fixture");
    if (!state.get("nested").toBoolean() && index == state.get("reenterAt").toNumber()) {
        state.set("nested", true);
        try {
            invoke(state.get("nestedManager"), "refreshMaterialUniforms",
                   {state.get("nestedUniforms"), state.get("nestedMaterial"), 2, 64,
                    state.get("nestedTarget")});
        } catch (...) {
            state.set("nested", false);
            throw;
        }
        state.set("nested", false);
    }
}
inline JSValue numeric(JSValue state, const std::string& label, JSValue result,
                       bool throws = false) {
    auto v = object({{"label", label}});
    prop(v, "valueOf",
         JSValue::function(
             [](const JSValue&, const Args&, const Args& c) {
                 tick(c[0], "coerce " + c[1].toString());
                 if (c[3].toBoolean())
                     throw JSTypeError("number");
                 return c[2];
             },
             {state, label, result, throws}, "valueOf", 0));
    return v;
}
inline JSValue scalar(JSValue state, unsigned code, const std::string& label = "override") {
    switch (code) {
    case 0:
        return {};
    case 1:
        return nullptr;
    case 2:
        return false;
    case 3:
        return true;
    case 4:
        return 0;
    case 5:
        return -0.;
    case 6:
        return -2;
    case 7:
        return 3;
    case 8:
        return std::numeric_limits<double>::quiet_NaN();
    case 9:
        return std::numeric_limits<double>::infinity();
    case 10:
        return "";
    case 11:
        return "3";
    case 12:
        return array();
    case 13:
        return numeric(state, label, 2);
    default:
        return numeric(state, label, 2, true);
    }
}
inline void observed(JSValue owner, const std::string& key, JSValue store, JSValue state,
                     const std::string& label) {
    JSPropertyDescriptor d;
    d.enumerable = d.configurable = true;
    d.getter = JSValue::function(
        [](const JSValue&, const Args&, const Args& c) {
            tick(c[1], "get " + c[2].toString());
            return c[0].get(c[3].toString());
        },
        {store, state, label, key});
    d.setter = JSValue::function(
        [](const JSValue&, const Args& args, const Args& c) {
            const auto value = args.empty() ? JSValue() : args[0];
            tick(c[1], "set " + c[2].toString() + " " + describe(value).dump());
            auto store = c[0];
            store.set(c[3].toString(), value);
            return JSValue();
        },
        {store, state, label, key});
    owner.defineProperty(key, d);
}
inline JSValue sink(JSValue state, const std::string& label) {
    auto prototype = state.get("sinkPrototype");
    if (prototype.isUndefined()) {
        prototype = JSValue::object();
        for (const auto* name : {"copy", "multiplyScalar", "negate", "set", "setFromMatrix4",
                                 "transpose", "premultiply", "setFromMatrixPosition"}) {
            auto fn = JSValue::function(
                [](const JSValue& self, const Args& args, const Args& c) {
                    A values;
                    for (const auto& arg : args)
                        values.push_back(describe(arg));
                    tick(c[0], "call " + self.get("label").toString() + "." + c[1].toString() +
                                   " " + JsonValue(values).dump());
                    if (c[0].get("returnMethod").strictEquals(c[1]))
                        return scalar(c[0], c[0].get("returnValue").toNumber());
                    return self;
                },
                {state, name}, name, 1);
            JSPropertyDescriptor d;
            d.enumerable = d.configurable = true;
            d.getter = JSValue::function(
                [](const JSValue& self, const Args&, const Args& c) {
                    tick(c[0], "get " + self.get("label").toString() + "." + c[1].toString());
                    if (c[0].get("badMethod").strictEquals(c[1]))
                        return JSValue(17);
                    return c[2];
                },
                {state, name, fn});
            prototype.defineProperty(name, d);
        }
        prop(state, "sinkPrototype", prototype);
    }
    auto result = JSValue::object(prototype);
    prop(result, "label", label);
    return result;
}
inline JSValue texture(JSValue state, const std::string& label, unsigned profile) {
    auto result = object({{"label", label}});
    auto store = object({{"matrixAutoUpdate", profile == 3 ? JSValue(1) : JSValue(true)},
                         {"matrix", object({{"label", label + ".matrix"}})},
                         {"isCubeTexture", true},
                         {"isRenderTargetTexture", false}});
    prop(store, "updateMatrix",
         JSValue::function(
             [](const JSValue&, const Args&, const Args& c) {
                 tick(c[0], "call " + c[1].toString() + ".updateMatrix");
                 return JSValue();
             },
             {state, label}, "updateMatrix", 0));
    for (const auto* key :
         {"matrixAutoUpdate", "matrix", "isCubeTexture", "isRenderTargetTexture", "updateMatrix"})
        observed(result, key, store, state, label + "." + key);
    return result;
}
inline bool flag(const std::string& name, unsigned kind) {
    static const std::vector<std::string> names{"",
                                                "isNodeMaterial",
                                                "isMeshBasicMaterial",
                                                "isMeshLambertMaterial",
                                                "isMeshToonMaterial",
                                                "isMeshPhongMaterial",
                                                "isMeshStandardMaterial",
                                                "isMeshPhysicalMaterial",
                                                "isMeshMatcapMaterial",
                                                "isMeshDepthMaterial",
                                                "isMeshDistanceMaterial",
                                                "isMeshNormalMaterial",
                                                "isLineBasicMaterial",
                                                "isLineDashedMaterial",
                                                "isPointsMaterial",
                                                "isSpriteMaterial",
                                                "isShadowMaterial",
                                                "isShaderMaterial"};
    if (kind == 18)
        return true;
    if (kind == 19)
        return name == "isMeshBasicMaterial" || name == "isShaderMaterial";
    return name == names[kind] || (kind == 7 && name == "isMeshStandardMaterial") ||
           (kind == 13 && name == "isLineBasicMaterial");
}
struct Fixture {
    JSValue state, uniforms = JSValue::object(), uniformStore = JSValue::object(),
                   material = JSValue::object(), materialStore = JSValue::object(),
                   properties = JSValue::object(), renderer = JSValue::object(), target,
                   fog = JSValue::object(), manager;
    Fixture(JSValue state, JSValue spec, const std::string& prefix = "") : state(state) {
        const unsigned kind = option(spec, "kind", 0).toNumber(),
                       profile = option(spec, "profile", 1).toNumber();
        const auto labeled = [&](const std::string& key) {
            return object({{"label", prefix + key}});
        };
        for (const auto& name : uniformsFields()) {
            auto holder = JSValue::object(),
                 store = object({{"value", sink(state, prefix + name + "Value")}});
            prop(uniformStore, name, store);
            observed(holder, "value", store, state, prefix + "uniform." + name + ".value");
            auto wrapper = object({{name.c_str(), holder}});
            if (name == option(spec, "omitUniform", "").toString())
                wrapper.set(name, JSValue());
            observed(uniforms, name, wrapper, state, prefix + "uniforms." + name);
        }
        for (const auto& name : materialFields()) {
            JSValue value;
            if (name.starts_with("is"))
                value = flag(name, kind);
            else if (name == "uniformsNeedUpdate")
                value = true;
            else if (name == "color" || name == "emissive" || name == "specular" ||
                     name == "normalScale" || name == "sheenColor" ||
                     name == "clearcoatNormalScale" || name == "attenuationColor" ||
                     name == "specularColor")
                value = labeled("material." + name);
            else if (name.ends_with("Map") || name == "map" || name == "matcap")
                value = profile == 0 ? JSValue(nullptr) : texture(state, prefix + name, profile);
            else if (name == "iridescenceThicknessRange")
                value = array({50, 400});
            else if (name == "side")
                value = profile == 3 ? JSValue("1") : JSValue(1);
            else
                value = profile == 0   ? JSValue(0)
                        : profile == 2 ? numeric(state, prefix + name, 2)
                        : profile == 3 ? JSValue("3")
                                       : JSValue(2);
            prop(materialStore, name, value);
            observed(material, name, materialStore, state, prefix + "material." + name);
        }
        const auto overrideName = option(spec, "override", "").toString();
        if (!overrideName.empty())
            materialStore.set(overrideName, scalar(state, option(spec, "value", 0).toNumber(),
                                                   prefix + overrideName + "Override"));
        auto cache = JSValue::object(),
             cacheStore =
                 object({{"envMap", profile == 0 ? JSValue(nullptr)
                                                 : texture(state, prefix + "cachedEnv", profile)},
                         {"envMapRotation",
                          object({{"x", 0.25}, {"y", -0.5}, {"z", 0.125}, {"order", "XYZ"}})}});
        auto light = JSValue::object(),
             lightStore =
                 object({{"matrixWorld", labeled("light.matrixWorld")},
                         {"shadow", object({{"camera", object({{"near", 1}, {"far", 99}})}})}});
        for (const auto* key : {"matrixWorld", "shadow"})
            observed(light, key, lightStore, state, prefix + "light." + key);
        prop(cacheStore, "light", light);
        for (const auto* key : {"envMap", "envMapRotation", "light"})
            observed(cache, key, cacheStore, state, prefix + "cache." + key);
        prop(properties, "get",
             JSValue::function(
                 [](const JSValue&, const Args& args, const Args& c) {
                     tick(c[0], "call " + c[1].toString() + "properties.get");
                     return c[2];
                 },
                 {state, prefix, cache}, "get", 1));
        auto propertiesStore = object({{"get", properties.get("get")}});
        observed(properties, "get", propertiesStore, state, prefix + "properties.get");
        target = JSValue::object();
        auto targetStore =
            object({{"texture", labeled("transmissionTexture")}, {"width", 128}, {"height", 64}});
        for (const auto* key : {"texture", "width", "height"})
            observed(target, key, targetStore, state, prefix + "target." + key);
        auto rendererStore = object({{"outputColorSpace", "srgb"}});
        const unsigned renderMode = option(spec, "renderMode", 0).toNumber();
        JSValue renderTarget = renderMode == 0
                                   ? JSValue(nullptr)
                                   : object({{"isXRRenderTarget", renderMode == 2},
                                             {"texture", object({{"colorSpace", "display-p3"}})}});
        prop(rendererStore, "getRenderTarget",
             JSValue::function(
                 [](const JSValue&, const Args&, const Args& c) {
                     tick(c[0], "call " + c[1].toString() + "renderer.getRenderTarget");
                     return c[2];
                 },
                 {state, prefix, renderTarget}, "getRenderTarget", 0));
        for (const auto* key : {"getRenderTarget", "outputColorSpace"})
            observed(renderer, key, rendererStore, state, prefix + "renderer." + key);
        auto fogColor = labeled("fogColor");
        auto fogColorStore =
            object({{"getRGB", JSValue::function(
                                   [](const JSValue&, const Args& args, const Args& c) {
                                       A values;
                                       for (const auto& a : args)
                                           values.push_back(describe(a));
                                       tick(c[0], "call " + c[1].toString() + "fogColor.getRGB " +
                                                      JsonValue(values).dump());
                                       return args.empty() ? JSValue() : args[0];
                                   },
                                   {state, prefix}, "getRGB", 1)}});
        observed(fogColor, "getRGB", fogColorStore, state, prefix + "fogColor.getRGB");
        const unsigned fogMode = option(spec, "fog", 0).toNumber();
        auto fogStore = object({{"color", fogColor},
                                {"isFog", fogMode == 1 || fogMode == 3},
                                {"isFogExp2", fogMode == 2 || fogMode == 3},
                                {"near", 1},
                                {"far", 80},
                                {"density", 0.01}});
        for (const auto* key : {"color", "isFog", "isFogExp2", "near", "far", "density"})
            observed(fog, key, fogStore, state, prefix + "fog." + key);
        manager = WebGLMaterials(renderer, properties).object();
    }
    JsonValue snapshot() {
        O out;
        for (const auto& name : uniformsFields())
            out[name] = describe(uniformStore.get(name).get("value"));
        return out;
    }
};
inline JsonValue scenario(JSValue spec) {
    auto state = object({{"log", array()},
                         {"count", 0},
                         {"throwAt", option(spec, "throwAt", -1)},
                         {"reenterAt", option(spec, "reenterAt", -1)},
                         {"nested", false},
                         {"badMethod", spec.get("badMethod")},
                         {"returnMethod", spec.get("returnMethod")},
                         {"returnValue", spec.get("returnValue")}});
    Fixture f(state, spec);
    Fixture nested(state, object({{"kind", 2}, {"profile", 1}}), "nested.");
    prop(state, "nestedManager", nested.manager);
    prop(state, "nestedUniforms", nested.uniforms);
    prop(state, "nestedMaterial", nested.material);
    prop(state, "nestedTarget", nested.target);
    JSValue result;
    JsonValue status = "undefined";
    try {
        const auto fog = option(spec, "fog", 0).toNumber();
        Args args =
            fog ? Args{f.uniforms, f.fog} : Args{f.uniforms, f.material, 1.5, 100, f.target};
        if (spec.hasOwn("argIndex"))
            args.at(spec.get("argIndex").toNumber()) =
                scalar(state, spec.get("argValue").toNumber());
        result = invoke(f.manager, fog ? "refreshFogUniforms" : "refreshMaterialUniforms", args);
        if (option(spec, "repeat", false).toBoolean())
            invoke(f.manager, "refreshMaterialUniforms",
                   {f.uniforms, f.material, 0.75, 200, f.target});
        status = describe(result);
    } catch (const JSTypeError&) {
        status = O{{"error", "TypeError"}};
    } catch (const JSRangeError&) {
        status = O{{"error", "RangeError"}};
    } catch (const std::exception& e) {
        status = O{{"error", "Error"}, {"message", e.what()}};
    }
    return O{{"status", status},
             {"log", ClippingFixture::encode(state.get("log"))},
             {"uniforms", f.snapshot()},
             {"needsUpdate", describe(f.materialStore.get("uniformsNeedUpdate"))}};
}
} // namespace MaterialsFixture

#include "cameras/PerspectiveCamera.h"
#include "lights/PointLight.h"
#include "materials/Materials.h"
#include "renderers/webgl/WebGLProperties.h"
#include "textures/CubeTexture.h"
namespace MaterialsFixture {
inline const std::vector<std::string>& nativeKinds() {
    static const std::vector<std::string> names{
        "Material",           "MeshBasicMaterial",    "MeshLambertMaterial",  "MeshToonMaterial",
        "MeshPhongMaterial",  "MeshStandardMaterial", "MeshPhysicalMaterial", "MeshMatcapMaterial",
        "MeshDepthMaterial",  "MeshDistanceMaterial", "MeshNormalMaterial",   "LineBasicMaterial",
        "LineDashedMaterial", "PointsMaterial",       "SpriteMaterial",       "ShadowMaterial",
        "ShaderMaterial",     "RawShaderMaterial"};
    return names;
}
inline std::shared_ptr<Material> makeNativeMaterial(unsigned kind) {
    switch (kind) {
#define MATERIAL_CASE(N, T)                                                                        \
    case N:                                                                                        \
        return std::make_shared<T>();
        MATERIAL_CASE(0, Material)
        MATERIAL_CASE(1, MeshBasicMaterial)
        MATERIAL_CASE(2, MeshLambertMaterial) MATERIAL_CASE(3, MeshToonMaterial)
            MATERIAL_CASE(4, MeshPhongMaterial) MATERIAL_CASE(5, MeshStandardMaterial)
                MATERIAL_CASE(6, MeshPhysicalMaterial) MATERIAL_CASE(7, MeshMatcapMaterial)
                    MATERIAL_CASE(8, MeshDepthMaterial) MATERIAL_CASE(9, MeshDistanceMaterial)
                        MATERIAL_CASE(10, MeshNormalMaterial) MATERIAL_CASE(11, LineBasicMaterial)
                            MATERIAL_CASE(12, LineDashedMaterial) MATERIAL_CASE(13, PointsMaterial)
                                MATERIAL_CASE(14, SpriteMaterial) MATERIAL_CASE(15, ShadowMaterial)
                                    MATERIAL_CASE(16, ShaderMaterial)
                                        MATERIAL_CASE(17, RawShaderMaterial)
#undef MATERIAL_CASE
    }
    throw std::runtime_error("Invalid native material fixture");
}
inline bool isColorName(const std::string& name) {
    return name == "diffuse" || name == "emissive" || name == "specular" || name == "sheenColor" ||
           name == "attenuationColor" || name == "specularColor" || name == "color" ||
           name == "fogColor";
}
inline bool isVector2Name(const std::string& name) {
    return name == "normalScale" || name == "clearcoatNormalScale" ||
           name == "transmissionSamplerSize" || name == "anisotropyVector";
}
inline JsonValue nativeDescription(JSValue value) {
    if (!value.isObject())
        return describe(value);
    if (value.hasOwn("label"))
        return describe(value);
    if (value.get("isColor").toBoolean())
        return O{{"color",
                  A{value.get("r").number(), value.get("g").number(), value.get("b").number()}}};
    if (value.get("isVector2").toBoolean())
        return O{{"vector", A{value.get("x").number(), value.get("y").number()}}};
    if (value.get("isVector3").toBoolean())
        return O{{"vector",
                  A{value.get("x").number(), value.get("y").number(), value.get("z").number()}}};
    return describe(value);
}
inline JSValue nativeUniforms() {
    auto uniforms = JSValue::object();
    for (const auto& name : uniformsFields()) {
        JSValue value = -99;
        if (isColorName(name))
            value = nativeColor(std::make_shared<Color>(0.1, 0.2, 0.3));
        else if (isVector2Name(name))
            value = nativeVector(std::make_shared<Vector2>(1.5, 2.5));
        else if (name == "referencePosition")
            value = nativeVector(std::make_shared<Vector3>(0.25, 0.5, 0.75));
        else if (name.ends_with("Transform") || name == "envMapRotation")
            value = nativeMatrix(std::make_shared<Matrix3>());
        prop(uniforms, name, object({{"value", value}}));
    }
    return uniforms;
}
inline JsonValue nativeScenario(JSValue spec) {
    const auto kind = option(spec, "kind", 0).toNumber();
    const bool full = option(spec, "full", false).toBoolean();
    auto material = makeNativeMaterial(kind);
    auto managed = nativeMaterial(material), uniforms = nativeUniforms();
    auto texture = std::make_shared<Texture>();
    texture->offset.set(0.125, -0.25);
    texture->repeat.set(2, 3);
    texture->center.set(0.5, 0.25);
    texture->rotation = 0.5;
    auto map = nativeTexture(texture);
    prop(map, "label", "native-map");
    auto cube = std::make_shared<CubeTexture>();
    auto environment = nativeTexture(cube);
    prop(environment, "label", "native-env");
    if (full)
        for (const auto& name : materialFields()) {
            if (name.starts_with("is") || name == "uniformsNeedUpdate")
                continue;
            JSValue value = 2;
            if (isColorName(name))
                value = nativeColor(std::make_shared<Color>(0.25, 0.5, 0.75));
            else if (isVector2Name(name))
                value = nativeVector(std::make_shared<Vector2>(1.5, 2.5));
            else if (name.ends_with("Map") || name == "map" || name == "matcap")
                value = map;
            else if (name == "iridescenceThicknessRange")
                value = array({50, 400});
            else if (name == "side")
                value = 1;
            const auto existing = managed.get(name);
            if (existing.isObject() && (isColorName(name) || isVector2Name(name)))
                invoke(existing, "copy", {value});
            else
                managed.set(name, value);
        }
    WebGLProperties properties;
    auto p = properties.object(), cache = invoke(p, "get", {managed});
    if (full) {
        prop(cache, "envMap", environment);
        prop(cache, "envMapRotation", nativeEuler(std::make_shared<Euler>(0.25, -0.5, 0.125)));
    }
    auto light = std::make_shared<PointLight>();
    light->matrixWorld.makeTranslation(4, 5, 6);
    auto lightObject = nativeObject(light);
    const auto camera = std::dynamic_pointer_cast<PerspectiveCamera>(light->shadow->camera);
    camera->near = 2;
    camera->far = 80;
    prop(cache, "light", lightObject);
    const auto renderMode = option(spec, "renderMode", 0).toNumber();
    auto renderTarget = renderMode == 0 ? JSValue(nullptr)
                                        : object({{"isXRRenderTarget", renderMode == 2},
                                                  {"texture", object({{"colorSpace", "srgb"}})}});
    auto renderer =
        object({{"getRenderTarget",
                 JSValue::function([](const JSValue&, const Args&, const Args& c) { return c[0]; },
                                   {renderTarget})},
                {"outputColorSpace", "srgb"}});
    WebGLMaterials manager(renderer, p);
    auto target = object({{"texture", map}, {"width", 128}, {"height", 64}});
    JsonValue status = "undefined";
    try {
        if (spec.get("fogNative").toBoolean()) {
            auto fog = object({{"color", nativeColor(std::make_shared<Color>(0.25, 0.5, 0.75))},
                               {"isFog", spec.get("fogNative").strictEquals(1)},
                               {"isFogExp2", spec.get("fogNative").strictEquals(2)},
                               {"near", 1},
                               {"far", 80},
                               {"density", 0.01}});
            status = describe(manager.refreshFogUniforms(uniforms, fog));
        } else
            status = describe(manager.refreshMaterialUniforms(uniforms, managed, 1.5, 100, target));
    } catch (const JSTypeError&) {
        status = O{{"error", "TypeError"}};
    } catch (const std::exception& e) {
        status = O{{"error", "Error"}, {"message", e.what()}};
    }
    O values;
    for (const auto& name : uniformsFields())
        values[name] = nativeDescription(uniforms.get(name).get("value"));
    return O{{"status", status},
             {"uniforms", values},
             {"needsUpdate", describe(managed.get("uniformsNeedUpdate"))},
             {"textureMatrix", describe(map.get("matrix"))}};
}
} // namespace MaterialsFixture

#include "WebGLClosureConstructibility.h"
namespace MaterialsFixture {
inline JsonValue constructScenario(JSValue spec) {
    const int method = option(spec, "method", 0).toNumber(),
              mode = option(spec, "mode", 0).toNumber();
    auto state = object({{"log", array()},
                         {"count", 0},
                         {"throwAt", mode == 10 ? 2 : -1},
                         {"reenterAt", -1},
                         {"nested", false}});
    Fixture fixture(state, object({{"kind", 7}, {"profile", 1}, {"fog", 1}}));
    auto fn = method == 0 ? nativeWebGLMaterialsConstructor()
                          : fixture.manager.get(method == 1 ? "refreshFogUniforms"
                                                            : "refreshMaterialUniforms");
    const auto metadata = WebGLClosureParity::metadata(fn);
    const auto original = fn.get("prototype");
    auto targetCalls = array(), custom = object({{"marker", 17}});
    auto alternate = JSValue::constructor(
        [](const JSValue&, const Args&, const Args& c) -> JSValue {
            invoke(c[0], "push", {"targetBody"});
            return {};
        },
        {targetCalls}, "Alternate", 0, JSValue::ConstructorKind::Callable);
    JSValue target = fn;
    if (mode == 1 || mode == 9) {
        alternate.set("prototype", custom);
        target = alternate;
        if (mode == 9)
            custom.preventExtensions();
    }
    if (mode == 2 || mode == 3) {
        alternate.set("prototype", mode == 2 ? JSValue(17) : JSValue(nullptr));
        target = alternate;
    }
    if (mode == 4)
        fn.set("prototype", custom);
    if (mode == 5)
        fn.set("prototype", 17);
    if (mode == 6)
        target = JSValue::function(
            [](const JSValue&, const Args&, const Args&) -> JSValue { return {}; });
    if (mode == 7)
        target = JSValue::object();
    Args args = method == 0   ? Args{fixture.renderer, fixture.properties}
                : method == 1 ? Args{fixture.uniforms, fixture.fog}
                              : Args{fixture.uniforms, fixture.material, 1.5, 100, fixture.target};
    JsonValue result;
    try {
        const auto value = mode == 8 ? fn.call(nullptr, args) : fn.construct(args, target);
        result = O{{"object", value.isObject()},
                   {"fnPrototype",
                    value.isObject() && value.getPrototype().sameValue(fn.get("prototype"))},
                   {"customPrototype", value.isObject() && value.getPrototype().sameValue(custom)},
                   {"objectPrototype", value.isObject() && value.getPrototype().sameValue(
                                                               JSValue::object().getPrototype())},
                   {"keys", value.isObject()
                                ? JsonValue(WebGLClosureParity::names(value.ownPropertyNames()))
                                : describe(value)}};
    } catch (const JSTypeError&) {
        result = O{{"error", "TypeError"}};
    }
    fn.set("prototype", original);
    return O{{"metadata", metadata},
             {"result", result},
             {"targetCalls", describe(targetCalls)},
             {"log", describe(state.get("log"))},
             {"uniforms", fixture.snapshot()}};
}
} // namespace MaterialsFixture
