// Native port of the pinned examples/jsm/shaders/SubsurfaceScatteringShader.js (see upstream.json).
// The GLSL and defines strings are the byte-exact template literal values of that
// revision; the exported objects keep the upstream field order.
#include "shaders/SubsurfaceScatteringShader.h"
#include "core/NativeObject.h"
#include "math/Color.h"
#include "renderers/shaders/ShaderChunk.h"
#include "renderers/shaders/ShaderLib.h"
#include "renderers/shaders/UniformsUtils.h"
#include <initializer_list>
#include <utility>

namespace {
JSValue object(std::initializer_list<std::pair<const char*,JSValue>> fields) {
    auto result=JSValue::object();
    for(const auto& [key,value]:fields){JSPropertyDescriptor d;d.value=value;d.writable=d.enumerable=d.configurable=true;result.defineProperty(key,d);}
    return result;
}
JSValue array(std::initializer_list<JSValue> values) {
    auto result=JSValue::array();std::size_t index=0;
    for(const auto& value:values){JSPropertyDescriptor d;d.value=value;d.writable=d.enumerable=d.configurable=true;result.defineProperty(std::to_string(index++),d);}
    return result;
}
} // namespace

namespace SubsurfaceScatteringShader {
namespace {

// replaceAll( string, find, replace ): String.prototype.split/join replaces every
// occurrence; String.prototype.replace with a string pattern replaces the first.
std::string replaceAllText(const std::string& text, const std::string& find, const std::string& replace) {
    if (find.empty()) return text;
    std::string result;
    std::size_t cursor = 0;
    while (true) {
        const std::size_t at = text.find(find, cursor);
        if (at == std::string::npos) { result.append(text, cursor, std::string::npos); return result; }
        result.append(text, cursor, at - cursor);
        result.append(replace);
        cursor = at + find.size();
    }
}
std::string replaceFirstText(const std::string& text, const std::string& find, const std::string& replace) {
    const std::size_t at = text.find(find);
    if (at == std::string::npos) return text;
    std::string result = text;
    result.replace(at, find.size(), replace);
    return result;
}
std::string joinLines(const std::vector<std::string>& lines) {
    std::string result;
    for (std::size_t i = 0; i < lines.size(); ++i) {
        if (i != 0) result += '\n';
        result += lines[i];
    }
    return result;
}
std::string subsurfaceScatteringFragmentShader(const JSValue& chunks) {
    const std::string phongFragment = chunks.get("meshphong_frag").string();
    const std::size_t bodyAt = phongFragment.find("void main() {");
    const std::string head = phongFragment.substr(0, bodyAt);
    std::string body = phongFragment.substr(bodyAt);
    const std::string scattering = joinLines({
        "RE_Direct( directLight, geometryPosition, geometryNormal, geometryViewDir, geometryClearcoatNormal, material, reflectedLight );",
        "#if defined( SUBSURFACE ) && defined( USE_UV )",
        " RE_Direct_Scattering(directLight, vUv, geometryPosition, geometryNormal, geometryViewDir, geometryClearcoatNormal, reflectedLight);",
        "#endif"});
    body = replaceFirstText(body, "#include <lights_fragment_begin>",
        replaceAllText(chunks.get("lights_fragment_begin").string(),
            "RE_Direct( directLight, geometryPosition, geometryNormal, geometryViewDir, geometryClearcoatNormal, material, reflectedLight );",
            scattering));
    return joinLines({
        "#define USE_UV",
        "#define SUBSURFACE",
        head,
        "uniform sampler2D thicknessMap;",
        "uniform float thicknessPower;",
        "uniform float thicknessScale;",
        "uniform float thicknessDistortion;",
        "uniform float thicknessAmbient;",
        "uniform float thicknessAttenuation;",
        "uniform vec3 thicknessColor;",
        "void RE_Direct_Scattering(const in IncidentLight directLight, const in vec2 uv, const in vec3 geometryPosition, const in vec3 geometryNormal, const in vec3 geometryViewDir, const in vec3 geometryClearcoatNormal, inout ReflectedLight reflectedLight) {",
        "\tvec3 thickness = thicknessColor * texture2D(thicknessMap, uv).r;",
        "\tvec3 scatteringHalf = normalize(directLight.direction + (geometryNormal * thicknessDistortion));",
        "\tfloat scatteringDot = pow(saturate(dot(geometryViewDir, -scatteringHalf)), thicknessPower) * thicknessScale;",
        "\tvec3 scatteringIllu = (scatteringDot + thicknessAmbient) * thickness;",
        "\treflectedLight.directDiffuse += scatteringIllu * thicknessAttenuation * directLight.color;",
        "}",
        body});
}
} // namespace

JSValue nativeSubsurfaceScatteringShader() {
    static thread_local JSWeakValue cache;
    if (const auto existing = cache.lock(); existing && existing->isObject()) return *existing;
    static thread_local bool initializing = false;
    JSModuleInitializationScope initialization(initializing, "SubsurfaceScatteringShader");
    const auto chunks = nativeShaderChunk();
    JSValue result = object({
        {"SubsurfaceScatteringShader", object({
            {"name", "SubsurfaceScatteringShader"},
            {"uniforms", mergeUniforms(array({
        ShaderLib.get("phong").get("uniforms"),
        object({
            {"thicknessMap", object({{"value", nullptr}})},
            {"thicknessColor", object({{"value", nativeColor(std::make_shared<Color>(1, 1, 1))}})},
            {"thicknessDistortion", object({{"value", 0.1}})},
            {"thicknessAmbient", object({{"value", 0}})},
            {"thicknessAttenuation", object({{"value", 0.1}})},
            {"thicknessPower", object({{"value", 2}})},
            {"thicknessScale", object({{"value", 10}})}
        })
    }))},
            {"vertexShader", std::string("#define USE_UV\n") + chunks.get("meshphong_vert").string()},
            {"fragmentShader", subsurfaceScatteringFragmentShader(chunks)}
        })}
    });
    result.retainForHeapLifetime();cache=JSWeakValue(result);return result;
}
} // namespace SubsurfaceScatteringShader
