// Direct port of three.js WebGLMaterials.js at
// 11738a9d55e502fb484cefffcea3a66dc44e9fe9. See THREEJS-LICENSE.
#include "renderers/webgl/WebGLMaterials.h"
#include "constants.h"
#include "core/NativeObject.h"
#include "math/JSNumberMath.h"
#include "renderers/shaders/UniformsUtils.h"
#include <algorithm>
#include <cmath>

namespace {
using Args = std::vector<JSValue>;
JSValue arg(const Args& args, std::size_t i) {
    return i < args.size() ? args[i] : JSValue();
}
void literal(JSValue object, const char* key, const JSValue& value) {
    JSPropertyDescriptor d;
    d.value = value;
    d.writable = d.enumerable = d.configurable = true;
    object.defineProperty(key, d);
}
JSValue add(const JSValue& left, const JSValue& right) {
    const auto a = left.toPrimitive(), b = right.toPrimitive();
    if (a.type() == JSValue::Type::String || b.type() == JSValue::Type::String)
        return a.toString() + b.toString();
    const auto x = a.toNumber();
    const auto y = b.toNumber();
    return x + y;
}
double product(const JSValue& left, const JSValue& right) {
    const auto x = left.toNumber();
    const auto y = right.toNumber();
    return x * y;
}
JSValue uniformValue(const JSValue& uniforms, const char* name) {
    return uniforms.get(name).get("value");
}
// Resolve the assignment reference or method before evaluating its RHS/args.
// Lambdas contain the observable property reads; no C++ argument evaluation
// order is relied on for two independent JavaScript expressions.
template <class F> void write(const JSValue& uniforms, const char* name, F&& rhs) {
    auto target = uniforms.get(name);
    const auto value = rhs();
    target.set("value", value);
}
void field(const JSValue& uniforms, const JSValue& material, const char* name) {
    write(uniforms, name, [&] { return material.get(name); });
}
template <class F> JSValue call(const JSValue& receiver, const char* name, F&& arguments) {
    const auto method = receiver.get(name);
    const auto values = arguments();
    return method.call(receiver, values);
}
JSValue call(const JSValue& receiver, const char* name) {
    return receiver.get(name).call(receiver);
}
JSValue copyField(const JSValue& uniforms, const JSValue& material, const char* target,
                  const char* source) {
    return call(uniformValue(uniforms, target), "copy", [&] { return Args{material.get(source)}; });
}
void scaledColor(const JSValue& uniforms, const JSValue& material, const char* name,
                 const char* intensity) {
    const auto copied = copyField(uniforms, material, name, name);
    call(copied, "multiplyScalar", [&] { return Args{material.get(intensity)}; });
}
void refreshTransformUniform(const JSValue& map, const JSValue& uniform) {
    if (map.get("matrixAutoUpdate").strictEquals(true))
        call(map, "updateMatrix");
    call(uniform.get("value"), "copy", [&] { return Args{map.get("matrix")}; });
}
void transform(const JSValue& uniforms, const JSValue& material, const char* mapName,
               const char* transformName) {
    const auto map = material.get(mapName);
    const auto uniform = uniforms.get(transformName);
    refreshTransformUniform(map, uniform);
}
void mapBranch(const JSValue& uniforms, const JSValue& material, const char* mapName,
               const char* transformName) {
    if (material.get(mapName).toBoolean()) {
        field(uniforms, material, mapName);
        transform(uniforms, material, mapName, transformName);
    }
}
void refreshUniformsCommon(const JSValue& uniforms, const JSValue& material,
                           const JSValue& properties, const JSValue& module) {
    field(uniforms, material, "opacity");
    if (material.get("color").toBoolean())
        copyField(uniforms, material, "diffuse", "color");
    if (material.get("emissive").toBoolean())
        scaledColor(uniforms, material, "emissive", "emissiveIntensity");
    mapBranch(uniforms, material, "map", "mapTransform");
    mapBranch(uniforms, material, "alphaMap", "alphaMapTransform");
    if (material.get("bumpMap").toBoolean()) {
        field(uniforms, material, "bumpMap");
        transform(uniforms, material, "bumpMap", "bumpMapTransform");
        field(uniforms, material, "bumpScale");
        if (material.get("side").strictEquals(BackSide)) {
            auto scale = uniforms.get("bumpScale");
            const auto value = scale.get("value");
            scale.set("value", product(value, -1));
        }
    }
    if (material.get("normalMap").toBoolean()) {
        field(uniforms, material, "normalMap");
        transform(uniforms, material, "normalMap", "normalMapTransform");
        copyField(uniforms, material, "normalScale", "normalScale");
        if (material.get("side").strictEquals(BackSide))
            call(uniformValue(uniforms, "normalScale"), "negate");
    }
    if (material.get("displacementMap").toBoolean()) {
        field(uniforms, material, "displacementMap");
        transform(uniforms, material, "displacementMap", "displacementMapTransform");
        field(uniforms, material, "displacementScale");
        field(uniforms, material, "displacementBias");
    }
    mapBranch(uniforms, material, "emissiveMap", "emissiveMapTransform");
    mapBranch(uniforms, material, "specularMap", "specularMapTransform");
    if (material.get("alphaTest").toNumber() > 0)
        field(uniforms, material, "alphaTest");
    const auto materialProperties = call(properties, "get", [&] { return Args{material}; });
    const auto envMap = materialProperties.get("envMap");
    const auto envMapRotation = materialProperties.get("envMapRotation");
    if (envMap.toBoolean()) {
        write(uniforms, "envMap", [&] { return envMap; });
        const auto rotation = uniformValue(uniforms, "envMapRotation");
        const auto fromMatrix = rotation.get("setFromMatrix4");
        const auto m1 = module.get("m1");
        const auto rotated =
            call(m1, "makeRotationFromEuler", [&] { return Args{envMapRotation}; });
        const auto copied = fromMatrix.call(rotation, {rotated});
        call(copied, "transpose");
        if (envMap.get("isCubeTexture").toBoolean() &&
            envMap.get("isRenderTargetTexture").strictEquals(false))
            call(uniformValue(uniforms, "envMapRotation"), "premultiply",
                 [&] { return Args{module.get("m")}; });
        field(uniforms, material, "reflectivity");
        field(uniforms, material, "ior");
        field(uniforms, material, "refractionRatio");
    }
    if (material.get("lightMap").toBoolean()) {
        field(uniforms, material, "lightMap");
        field(uniforms, material, "lightMapIntensity");
        transform(uniforms, material, "lightMap", "lightMapTransform");
    }
    if (material.get("aoMap").toBoolean()) {
        field(uniforms, material, "aoMap");
        field(uniforms, material, "aoMapIntensity");
        transform(uniforms, material, "aoMap", "aoMapTransform");
    }
}
void refreshUniformsLine(const JSValue& uniforms, const JSValue& material) {
    copyField(uniforms, material, "diffuse", "color");
    field(uniforms, material, "opacity");
    mapBranch(uniforms, material, "map", "mapTransform");
}
void refreshUniformsDash(const JSValue& uniforms, const JSValue& material) {
    field(uniforms, material, "dashSize");
    write(uniforms, "totalSize", [&] {
        const auto dash = material.get("dashSize"), gap = material.get("gapSize");
        return add(dash, gap);
    });
    field(uniforms, material, "scale");
}
void refreshUniformsPoints(const JSValue& uniforms, const JSValue& material,
                           const JSValue& pixelRatio, const JSValue& height) {
    copyField(uniforms, material, "diffuse", "color");
    field(uniforms, material, "opacity");
    write(uniforms, "size", [&] { return product(material.get("size"), pixelRatio); });
    write(uniforms, "scale", [&] { return product(height, 0.5); });
    mapBranch(uniforms, material, "map", "uvTransform");
    mapBranch(uniforms, material, "alphaMap", "alphaMapTransform");
    if (material.get("alphaTest").toNumber() > 0)
        field(uniforms, material, "alphaTest");
}
void refreshUniformsSprites(const JSValue& uniforms, const JSValue& material) {
    copyField(uniforms, material, "diffuse", "color");
    field(uniforms, material, "opacity");
    field(uniforms, material, "rotation");
    mapBranch(uniforms, material, "map", "mapTransform");
    mapBranch(uniforms, material, "alphaMap", "alphaMapTransform");
    if (material.get("alphaTest").toNumber() > 0)
        field(uniforms, material, "alphaTest");
}
void refreshUniformsPhong(const JSValue& uniforms, const JSValue& material) {
    copyField(uniforms, material, "specular", "specular");
    write(uniforms, "shininess", [&] {
        const auto value = material.get("shininess").toNumber();
        return std::isnan(value) ? value : std::max(value, 1e-4);
    });
}
void refreshUniformsToon(const JSValue& uniforms, const JSValue& material) {
    if (material.get("gradientMap").toBoolean())
        field(uniforms, material, "gradientMap");
}
void refreshUniformsStandard(const JSValue& uniforms, const JSValue& material) {
    field(uniforms, material, "metalness");
    mapBranch(uniforms, material, "metalnessMap", "metalnessMapTransform");
    field(uniforms, material, "roughness");
    mapBranch(uniforms, material, "roughnessMap", "roughnessMapTransform");
    if (material.get("envMap").toBoolean())
        field(uniforms, material, "envMapIntensity");
}
void refreshUniformsPhysical(const JSValue& uniforms, const JSValue& material,
                             const JSValue& transmissionRenderTarget) {
    field(uniforms, material, "ior");
    if (material.get("diffuseRoughness").toNumber() > 0) {
        field(uniforms, material, "diffuseRoughness");
        mapBranch(uniforms, material, "diffuseRoughnessMap", "diffuseRoughnessMapTransform");
    }
    if (material.get("sheen").toNumber() > 0) {
        scaledColor(uniforms, material, "sheenColor", "sheen");
        field(uniforms, material, "sheenRoughness");
        mapBranch(uniforms, material, "sheenColorMap", "sheenColorMapTransform");
        mapBranch(uniforms, material, "sheenRoughnessMap", "sheenRoughnessMapTransform");
    }
    if (material.get("clearcoat").toNumber() > 0) {
        field(uniforms, material, "clearcoat");
        field(uniforms, material, "clearcoatRoughness");
        mapBranch(uniforms, material, "clearcoatMap", "clearcoatMapTransform");
        mapBranch(uniforms, material, "clearcoatRoughnessMap", "clearcoatRoughnessMapTransform");
        if (material.get("clearcoatNormalMap").toBoolean()) {
            field(uniforms, material, "clearcoatNormalMap");
            transform(uniforms, material, "clearcoatNormalMap", "clearcoatNormalMapTransform");
            copyField(uniforms, material, "clearcoatNormalScale", "clearcoatNormalScale");
            if (material.get("side").strictEquals(BackSide))
                call(uniformValue(uniforms, "clearcoatNormalScale"), "negate");
        }
    }
    if (material.get("dispersion").toNumber() > 0)
        field(uniforms, material, "dispersion");
    if (material.get("retroreflectivity").toNumber() > 0)
        field(uniforms, material, "retroreflectivity");
    if (material.get("iridescence").toNumber() > 0) {
        field(uniforms, material, "iridescence");
        field(uniforms, material, "iridescenceIOR");
        write(uniforms, "iridescenceThicknessMinimum",
              [&] { return material.get("iridescenceThicknessRange").get("0"); });
        write(uniforms, "iridescenceThicknessMaximum",
              [&] { return material.get("iridescenceThicknessRange").get("1"); });
        mapBranch(uniforms, material, "iridescenceMap", "iridescenceMapTransform");
        mapBranch(uniforms, material, "iridescenceThicknessMap",
                  "iridescenceThicknessMapTransform");
    }
    if (material.get("transmission").toNumber() > 0) {
        field(uniforms, material, "transmission");
        write(uniforms, "transmissionSamplerMap",
              [&] { return transmissionRenderTarget.get("texture"); });
        call(uniformValue(uniforms, "transmissionSamplerSize"), "set", [&] {
            const auto width = transmissionRenderTarget.get("width"),
                       height = transmissionRenderTarget.get("height");
            return Args{width, height};
        });
        mapBranch(uniforms, material, "transmissionMap", "transmissionMapTransform");
        field(uniforms, material, "thickness");
        mapBranch(uniforms, material, "thicknessMap", "thicknessMapTransform");
        field(uniforms, material, "attenuationDistance");
        copyField(uniforms, material, "attenuationColor", "attenuationColor");
    }
    if (material.get("anisotropy").toNumber() > 0) {
        call(uniformValue(uniforms, "anisotropyVector"), "set", [&] {
            const auto xValue = material.get("anisotropy");
            const auto xRotation = material.get("anisotropyRotation");
            const auto cosine = JSNumberMath::cos(xRotation.toNumber());
            const auto x = product(xValue, cosine);
            const auto yValue = material.get("anisotropy");
            const auto yRotation = material.get("anisotropyRotation");
            const auto sine = JSNumberMath::sin(yRotation.toNumber());
            const auto y = product(yValue, sine);
            return Args{x, y};
        });
        mapBranch(uniforms, material, "anisotropyMap", "anisotropyMapTransform");
    }
    field(uniforms, material, "specularIntensity");
    copyField(uniforms, material, "specularColor", "specularColor");
    mapBranch(uniforms, material, "specularColorMap", "specularColorMapTransform");
    mapBranch(uniforms, material, "specularIntensityMap", "specularIntensityMapTransform");
}
void refreshUniformsMatcap(const JSValue& uniforms, const JSValue& material) {
    if (material.get("matcap").toBoolean())
        field(uniforms, material, "matcap");
}
void refreshUniformsDistance(const JSValue& uniforms, const JSValue& material,
                             const JSValue& properties) {
    const auto light = call(properties, "get", [&] { return Args{material}; }).get("light");
    call(uniformValue(uniforms, "referencePosition"), "setFromMatrixPosition",
         [&] { return Args{light.get("matrixWorld")}; });
    write(uniforms, "nearDistance", [&] { return light.get("shadow").get("camera").get("near"); });
    write(uniforms, "farDistance", [&] { return light.get("shadow").get("camera").get("far"); });
}
JSValue refreshFog(const JSValue&, const Args& args, const Args& captures) {
    const auto uniforms = arg(args, 0), fog = arg(args, 1), renderer = captures[0];
    const auto color = fog.get("color"), getRGB = color.get("getRGB");
    const auto target = uniformValue(uniforms, "fogColor");
    const auto colorSpace = getUnlitUniformColorSpace(renderer);
    getRGB.call(color, {target, colorSpace});
    if (fog.get("isFog").toBoolean()) {
        write(uniforms, "fogNear", [&] { return fog.get("near"); });
        write(uniforms, "fogFar", [&] { return fog.get("far"); });
    } else if (fog.get("isFogExp2").toBoolean()) {
        write(uniforms, "fogDensity", [&] { return fog.get("density"); });
    }
    return {};
}
JSValue refreshMaterial(const JSValue&, const Args& args, const Args& captures) {
    const auto uniforms = arg(args, 0), material = arg(args, 1), pixelRatio = arg(args, 2),
               height = arg(args, 3), target = arg(args, 4);
    const auto properties = captures[0], module = captures[1];
    auto writable = material;
    if (material.get("isNodeMaterial").toBoolean())
        writable.set("uniformsNeedUpdate", false);
    else if (material.get("isMeshBasicMaterial").toBoolean())
        refreshUniformsCommon(uniforms, material, properties, module);
    else if (material.get("isMeshLambertMaterial").toBoolean()) {
        refreshUniformsCommon(uniforms, material, properties, module);
        if (material.get("envMap").toBoolean())
            field(uniforms, material, "envMapIntensity");
    } else if (material.get("isMeshToonMaterial").toBoolean()) {
        refreshUniformsCommon(uniforms, material, properties, module);
        refreshUniformsToon(uniforms, material);
    } else if (material.get("isMeshPhongMaterial").toBoolean()) {
        refreshUniformsCommon(uniforms, material, properties, module);
        refreshUniformsPhong(uniforms, material);
        if (material.get("envMap").toBoolean())
            field(uniforms, material, "envMapIntensity");
    } else if (material.get("isMeshStandardMaterial").toBoolean()) {
        refreshUniformsCommon(uniforms, material, properties, module);
        refreshUniformsStandard(uniforms, material);
        if (material.get("isMeshPhysicalMaterial").toBoolean())
            refreshUniformsPhysical(uniforms, material, target);
    } else if (material.get("isMeshMatcapMaterial").toBoolean()) {
        refreshUniformsCommon(uniforms, material, properties, module);
        refreshUniformsMatcap(uniforms, material);
    } else if (material.get("isMeshDepthMaterial").toBoolean())
        refreshUniformsCommon(uniforms, material, properties, module);
    else if (material.get("isMeshDistanceMaterial").toBoolean()) {
        refreshUniformsCommon(uniforms, material, properties, module);
        refreshUniformsDistance(uniforms, material, properties);
    } else if (material.get("isMeshNormalMaterial").toBoolean())
        refreshUniformsCommon(uniforms, material, properties, module);
    else if (material.get("isLineBasicMaterial").toBoolean()) {
        refreshUniformsLine(uniforms, material);
        if (material.get("isLineDashedMaterial").toBoolean())
            refreshUniformsDash(uniforms, material);
    } else if (material.get("isPointsMaterial").toBoolean())
        refreshUniformsPoints(uniforms, material, pixelRatio, height);
    else if (material.get("isSpriteMaterial").toBoolean())
        refreshUniformsSprites(uniforms, material);
    else if (material.get("isShadowMaterial").toBoolean()) {
        copyField(uniforms, material, "color", "color");
        field(uniforms, material, "opacity");
    } else if (material.get("isShaderMaterial").toBoolean())
        writable.set("uniformsNeedUpdate", false);
    return {};
}
JSValue matrix(bool four) {
    auto result = JSValue::object(four ? nativeMatrix4Prototype() : nativeMatrix3Prototype());
    auto elements = JSValue::array();
    const int count = four ? 16 : 9, diagonal = four ? 5 : 4;
    for (int i = 0; i < count; ++i)
        literal(elements, std::to_string(i).c_str(), i % diagonal == 0 ? JSValue(1) : JSValue(0));
    result.set("elements", elements);
    return result;
}
JSValue constructor(const JSValue&, const Args& args, const Args& captures) {
    const auto renderer = arg(args, 0), properties = arg(args, 1), module = captures[0];
    auto result = JSValue::object();
    literal(result, "refreshFogUniforms",
            JSValue::constructor(refreshFog, {renderer, properties, module}, "refreshFogUniforms", 2,
                                 JSValue::ConstructorKind::Callable));
    literal(result, "refreshMaterialUniforms",
            JSValue::constructor(refreshMaterial, {properties, module, renderer}, "refreshMaterialUniforms",
                                 5, JSValue::ConstructorKind::Callable));
    return result;
}
void validate(const JSValue& value) {
    static_cast<void>(JSWeakValue(value));
}
} // namespace
JSValue nativeWebGLMaterialsConstructor() {
    static thread_local JSWeakValue weak;
    if (const auto value = weak.lock(); value && value->isCallable())
        return *value;
    auto module = JSValue::object();
    literal(module, "m1", matrix(true));
    literal(module, "m", matrix(false));
    call(module.get("m"), "set", [] { return Args{-1, 0, 0, 0, 1, 0, 0, 0, 1}; });
    auto value = JSValue::constructor(constructor, {module}, "WebGLMaterials", 2,
                                      JSValue::ConstructorKind::Callable);
    value.retainForHeapLifetime();
    weak = JSWeakValue(value);
    return value;
}
WebGLMaterials::WebGLMaterials(const JSValue& renderer, const JSValue& properties) {
    validate(renderer);
    validate(properties);
    object_ = nativeWebGLMaterialsConstructor().call({}, {renderer, properties});
}
JSValue WebGLMaterials::refreshFogUniforms(const JSValue& uniforms, const JSValue& fog) {
    validate(object_);
    validate(uniforms);
    validate(fog);
    return object_.get("refreshFogUniforms").call(object_, {uniforms, fog});
}
JSValue WebGLMaterials::refreshMaterialUniforms(const JSValue& uniforms, const JSValue& material,
                                                const JSValue& pixelRatio, const JSValue& height,
                                                const JSValue& transmissionRenderTarget) {
    validate(object_);
    validate(uniforms);
    validate(material);
    validate(pixelRatio);
    validate(height);
    validate(transmissionRenderTarget);
    return object_.get("refreshMaterialUniforms")
        .call(object_, {uniforms, material, pixelRatio, height, transmissionRenderTarget});
}
