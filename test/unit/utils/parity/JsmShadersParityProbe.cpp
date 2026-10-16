// Differential probe for the pinned examples/jsm/shaders cluster.
//
// Protocol: the probe prints ONE JSON object describing every ported shader module and
// exits. compare-jsm-shaders.mjs imports the real upstream modules, builds the same
// description and compares both trees with assert.deepEqual (no tolerances).
//
// Per module the description contains the module namespace keys in export order and,
// for every exported value, the own key list in field order. String values are escaped
// character by character (`\uXXXX` for everything outside printable ASCII) so the
// comparison is byte-exact and the JSON round-trips. Numbers are written with the ES
// Number::toString spelling, so `-0`, `NaN` and the infinities are distinguishable.
//
// The exported helper functions cannot be compared as source text, so they are called
// with fixed inputs and their results are compared instead.
#include "core/JSValue.h"
#include "core/JsonValue.h"
#include "core/NativeObject.h"
#include "math/Vector2.h"
#include "textures/DataTexture.h"
#include "shaders/ACESFilmicToneMappingShader.h"
#include "shaders/AfterimageShader.h"
#include "shaders/BasicShader.h"
#include "shaders/BleachBypassShader.h"
#include "shaders/BlendShader.h"
#include "shaders/BokehShader.h"
#include "shaders/BokehShader2.h"
#include "shaders/BrightnessContrastShader.h"
#include "shaders/ColorCorrectionShader.h"
#include "shaders/ColorifyShader.h"
#include "shaders/ConvolutionShader.h"
#include "shaders/CopyShader.h"
#include "shaders/DOFMipMapShader.h"
#include "shaders/DepthLimitedBlurShader.h"
#include "shaders/DigitalGlitch.h"
#include "shaders/DotScreenShader.h"
#include "shaders/ExposureShader.h"
#include "shaders/FXAAShader.h"
#include "shaders/FilmShader.h"
#include "shaders/FocusShader.h"
#include "shaders/FreiChenShader.h"
#include "shaders/GTAOShader.h"
#include "shaders/GammaCorrectionShader.h"
#include "shaders/HalftoneShader.h"
#include "shaders/HorizontalBlurShader.h"
#include "shaders/HorizontalTiltShiftShader.h"
#include "shaders/HueSaturationShader.h"
#include "shaders/KaleidoShader.h"
#include "shaders/LuminosityHighPassShader.h"
#include "shaders/LuminosityShader.h"
#include "shaders/MirrorShader.h"
#include "shaders/NormalMapShader.h"
#include "shaders/OutputShader.h"
#include "shaders/PoissonDenoiseShader.h"
#include "shaders/RGBShiftShader.h"
#include "shaders/SAOShader.h"
#include "shaders/SMAAShader.h"
#include "shaders/SSAOShader.h"
#include "shaders/SSRShader.h"
#include "shaders/SepiaShader.h"
#include "shaders/SobelOperatorShader.h"
#include "shaders/SubsurfaceScatteringShader.h"
#include "shaders/TechnicolorShader.h"
#include "shaders/ToonShader.h"
#include "shaders/TriangleBlurShader.h"
#include "shaders/UnpackDepthRGBAShader.h"
#include "shaders/VelocityShader.h"
#include "shaders/VolumeShader.h"
#include "shaders/VerticalBlurShader.h"
#include "shaders/VerticalTiltShiftShader.h"
#include "shaders/VignetteShader.h"
#include "shaders/WaterRefractionShader.h"
#include <cmath>
#include <cstdint>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

namespace {

// One `\uXXXX` escape per non-printable/non-ASCII code unit, exactly like the script.
std::string escapeText(const std::string& text) {
    static const char* digits = "0123456789abcdef";
    std::string result;
    for (std::size_t index = 0; index < text.size(); ++index) {
        const auto code = static_cast<unsigned char>(text[index]);
        if (code == '\\') result += "\\\\";
        else if (code == '"') result += "\\\"";
        else if (code >= 0x20 && code <= 0x7e) result += char(code);
        else {
            result += "\\u00";
            result += digits[(code >> 4) & 0xf];
            result += digits[code & 0xf];
        }
    }
    return result;
}

JsonValue numberNode(double value) {
    std::string text;
    if (std::isnan(value)) text = "NaN";
    else if (std::isinf(value)) text = value < 0 ? "-Infinity" : "Infinity";
    else if (value == 0 && std::signbit(value)) text = "-0";
    else text = JSValue(value).toString();
    return JsonValue::Object{{"t", "num"}, {"v", text}};
}

JsonValue stringNode(const std::string& value) {
    return JsonValue::Object{{"t", "str"}, {"v", escapeText(value)}};
}

JsonValue elementsNode(const JSValue& object, const char* type, const char* key) {
    JsonValue::Array values;
    const auto elements = object.get(key);
    for (const auto& index : elements.keys()) values.push_back(numberNode(elements.get(index).toNumber()));
    return JsonValue::Object{{"t", type}, {"values", std::move(values)}};
}

// The DataTexture of GTAOShader.generateMagicSquareNoise, read through the native side of
// the wrapper so the pixel bytes are compared, not only the wrapper shape.
JsonValue textureNode(const JSValue& value) {
    const auto texture = std::static_pointer_cast<Texture>(value.hostPayload("three.Texture"));
    const TextureImage::Ptr image = texture ? texture->image.get() : nullptr;
    JsonValue::Array data;
    if (image) {
        if (const auto* bytes = std::get_if<Uint8Array>(&image->data))
            for (std::size_t index = 0; index < bytes->size(); ++index) data.push_back(numberNode(double((*bytes)[index])));
    }
    JsonValue::Object node;
    node["t"] = "DataTexture";
    node["width"] = numberNode(image ? image->width : 0);
    node["height"] = numberNode(image ? image->height : 0);
    node["wrapS"] = numberNode(texture ? texture->wrapS : 0);
    node["wrapT"] = numberNode(texture ? texture->wrapT : 0);
    node["version"] = numberNode(texture ? texture->version : 0);
    node["data"] = std::move(data);
    return node;
}

JsonValue describe(const JSValue& value) {
    switch (value.type()) {
        case JSValue::Type::Undefined: return JsonValue::Object{{"t", "undefined"}};
        case JSValue::Type::Null: return JsonValue::Object{{"t", "null"}};
        case JSValue::Type::Boolean: return JsonValue::Object{{"t", "bool"}, {"v", value.toBoolean()}};
        case JSValue::Type::Number: return numberNode(value.toNumber());
        case JSValue::Type::String: return stringNode(value.string());
        case JSValue::Type::Function:
            return JsonValue::Object{{"t", "fn"},
                {"name", value.get("name").toString()},
                {"length", double(value.get("length").toNumber())}};
        default: break;
    }
    const auto hasFlag = [&](const char* key) {
        return value.has(key) && value.get(key).toBoolean();
    };
    if (value.type() == JSValue::Type::Array) {
        JsonValue::Array values;
        for (const auto& index : value.keys()) values.push_back(describe(value.get(index)));
        return JsonValue::Object{{"t", "array"}, {"values", std::move(values)}};
    }
    if (hasFlag("isTexture")) return textureNode(value);
    if (hasFlag("isVector2"))
        return JsonValue::Object{{"t", "Vector2"}, {"values", JsonValue::Array{numberNode(value.get("x").toNumber()), numberNode(value.get("y").toNumber())}}};
    if (hasFlag("isVector3"))
        return JsonValue::Object{{"t", "Vector3"},
            {"values", JsonValue::Array{numberNode(value.get("x").toNumber()), numberNode(value.get("y").toNumber()),
                            numberNode(value.get("z").toNumber())}}};
    if (hasFlag("isVector4"))
        return JsonValue::Object{{"t", "Vector4"},
            {"values", JsonValue::Array{numberNode(value.get("x").toNumber()), numberNode(value.get("y").toNumber()),
                            numberNode(value.get("z").toNumber()), numberNode(value.get("w").toNumber())}}};
    if (hasFlag("isColor"))
        return JsonValue::Object{{"t", "Color"},
            {"values", JsonValue::Array{numberNode(value.get("r").toNumber()), numberNode(value.get("g").toNumber()),
                            numberNode(value.get("b").toNumber())}}};
    if (hasFlag("isMatrix3")) return elementsNode(value, "Matrix3", "elements");
    if (hasFlag("isMatrix4")) return elementsNode(value, "Matrix4", "elements");
    JsonValue::Array keys, values;
    for (const auto& key : value.keys()) {
        keys.push_back(key);
        values.push_back(describe(value.get(key)));
    }
    return JsonValue::Object{{"t", "object"}, {"keys", std::move(keys)}, {"values", std::move(values)}};
}

JsonValue moduleNode(const char* file, const JSValue& module) {
    JsonValue::Array exports;
    for (const auto& name : module.keys()) exports.push_back(JsonValue::Object{{"name", name}, {"value", describe(module.get(name))}});
    return JsonValue::Object{{"file", file}, {"exports", std::move(exports)}};
}

// A plain material stand-in for BlurShaderUtils.configure.
JSValue makeMaterial() {
    auto uniforms = JSValue::object();
    auto offsetsEntry = JSValue::object();
    offsetsEntry.set("value", JSValue{});
    auto weightsEntry = JSValue::object();
    weightsEntry.set("value", JSValue{});
    uniforms.set("sampleUvOffsets", offsetsEntry);
    uniforms.set("sampleWeights", weightsEntry);
    auto material = JSValue::object();
    material.set("defines", JSValue::object());
    material.set("uniforms", uniforms);
    return material;
}

JSValue callHelper(const JSValue& owner, const char* key, const std::vector<JSValue>& args) {
    const auto fn = owner.get(key);
    return fn.call(owner, args);
}

} // namespace

int main() {
    try {
        struct ModuleEntry {
            const char* file;
            JSValue (*resolve)();
        };
        const ModuleEntry modules[] = {
            {"ACESFilmicToneMappingShader.js", &ACESFilmicToneMappingShader::nativeACESFilmicToneMappingShader},
            {"AfterimageShader.js", &AfterimageShader::nativeAfterimageShader},
            {"BasicShader.js", &BasicShader::nativeBasicShader},
            {"BleachBypassShader.js", &BleachBypassShader::nativeBleachBypassShader},
            {"BlendShader.js", &BlendShader::nativeBlendShader},
            {"BokehShader.js", &BokehShader::nativeBokehShader},
            {"BokehShader2.js", &BokehShader2::nativeBokehShader2},
            {"BrightnessContrastShader.js", &BrightnessContrastShader::nativeBrightnessContrastShader},
            {"ColorCorrectionShader.js", &ColorCorrectionShader::nativeColorCorrectionShader},
            {"ColorifyShader.js", &ColorifyShader::nativeColorifyShader},
            {"ConvolutionShader.js", &ConvolutionShader::nativeConvolutionShader},
            {"CopyShader.js", &CopyShader::nativeCopyShader},
            {"DOFMipMapShader.js", &DOFMipMapShader::nativeDOFMipMapShader},
            {"DepthLimitedBlurShader.js", &DepthLimitedBlurShader::nativeDepthLimitedBlurShader},
            {"DigitalGlitch.js", &DigitalGlitch::nativeDigitalGlitch},
            {"DotScreenShader.js", &DotScreenShader::nativeDotScreenShader},
            {"ExposureShader.js", &ExposureShader::nativeExposureShader},
            {"FXAAShader.js", &FXAAShader::nativeFXAAShader},
            {"FilmShader.js", &FilmShader::nativeFilmShader},
            {"FocusShader.js", &FocusShader::nativeFocusShader},
            {"FreiChenShader.js", &FreiChenShader::nativeFreiChenShader},
            {"GTAOShader.js", &GTAOShader::nativeGTAOShader},
            {"GammaCorrectionShader.js", &GammaCorrectionShader::nativeGammaCorrectionShader},
            {"HalftoneShader.js", &HalftoneShader::nativeHalftoneShader},
            {"HorizontalBlurShader.js", &HorizontalBlurShader::nativeHorizontalBlurShader},
            {"HorizontalTiltShiftShader.js", &HorizontalTiltShiftShader::nativeHorizontalTiltShiftShader},
            {"HueSaturationShader.js", &HueSaturationShader::nativeHueSaturationShader},
            {"KaleidoShader.js", &KaleidoShader::nativeKaleidoShader},
            {"LuminosityHighPassShader.js", &LuminosityHighPassShader::nativeLuminosityHighPassShader},
            {"LuminosityShader.js", &LuminosityShader::nativeLuminosityShader},
            {"MirrorShader.js", &MirrorShader::nativeMirrorShader},
            {"NormalMapShader.js", &NormalMapShader::nativeNormalMapShader},
            {"OutputShader.js", &OutputShader::nativeOutputShader},
            {"PoissonDenoiseShader.js", &PoissonDenoiseShader::nativePoissonDenoiseShader},
            {"RGBShiftShader.js", &RGBShiftShader::nativeRGBShiftShader},
            {"SAOShader.js", &SAOShader::nativeSAOShader},
            {"SMAAShader.js", &SMAAShader::nativeSMAAShader},
            {"SSAOShader.js", &SSAOShader::nativeSSAOShader},
            {"SSRShader.js", &SSRShader::nativeSSRShader},
            {"SepiaShader.js", &SepiaShader::nativeSepiaShader},
            {"SobelOperatorShader.js", &SobelOperatorShader::nativeSobelOperatorShader},
            {"SubsurfaceScatteringShader.js", &SubsurfaceScatteringShader::nativeSubsurfaceScatteringShader},
            {"TechnicolorShader.js", &TechnicolorShader::nativeTechnicolorShader},
            {"ToonShader.js", &ToonShader::nativeToonShader},
            {"TriangleBlurShader.js", &TriangleBlurShader::nativeTriangleBlurShader},
            {"UnpackDepthRGBAShader.js", &UnpackDepthRGBAShader::nativeUnpackDepthRGBAShader},
            {"VelocityShader.js", &VelocityShader::nativeVelocityShader},
            {"VerticalBlurShader.js", &VerticalBlurShader::nativeVerticalBlurShader},
            {"VerticalTiltShiftShader.js", &VerticalTiltShiftShader::nativeVerticalTiltShiftShader},
            {"VignetteShader.js", &VignetteShader::nativeVignetteShader},
            {"VolumeShader.js", &VolumeShader::nativeVolumeShader},
            {"WaterRefractionShader.js", &WaterRefractionShader::nativeWaterRefractionShader},
        };

        JsonValue::Array moduleNodes;
        for (const auto& entry : modules) moduleNodes.push_back(moduleNode(entry.file, entry.resolve()));

        JsonValue::Array helperNodes;
        const auto pushHelper = [&](const std::string& name, JsonValue value) {
            helperNodes.push_back(JsonValue::Object{{"name", name}, {"value", std::move(value)}});
        };

        const auto blurModule = DepthLimitedBlurShader::nativeDepthLimitedBlurShader();
        const auto blurUtils = blurModule.get("BlurShaderUtils");
        const auto halfPixel = nativeVector(std::make_shared<Vector2>(1.0 / 512.0, 0.0));
        pushHelper("BlurShaderUtils.createSampleWeights(4, 1)",
            describe(callHelper(blurUtils, "createSampleWeights", {JSValue(4), JSValue(1)})));
        pushHelper("BlurShaderUtils.createSampleWeights(0, 2.5)",
            describe(callHelper(blurUtils, "createSampleWeights", {JSValue(0), JSValue(2.5)})));
        pushHelper("BlurShaderUtils.createSampleOffsets(4, Vector2(1/512, 0))",
            describe(callHelper(blurUtils, "createSampleOffsets", {JSValue(4), halfPixel})));
        const auto material = makeMaterial();
        callHelper(blurUtils, "configure", {material, JSValue(4), JSValue(1), halfPixel});
        pushHelper("BlurShaderUtils.configure(material, 4, 1, Vector2(1/512, 0))", describe(material));

        const auto gtaoModule = GTAOShader::nativeGTAOShader();
        const auto magicSquareNoise = gtaoModule.get("generateMagicSquareNoise");
        pushHelper("generateMagicSquareNoise()", describe(magicSquareNoise.call(JSValue{}, {})));
        pushHelper("generateMagicSquareNoise(4)", describe(magicSquareNoise.call(JSValue{}, {JSValue(4)})));
        pushHelper("generateMagicSquareNoise(5)", describe(magicSquareNoise.call(JSValue{}, {JSValue(5)})));

        const auto poissonModule = PoissonDenoiseShader::nativePoissonDenoiseShader();
        const auto samplePoints = poissonModule.get("generatePdSamplePointInitializer");
        pushHelper("generatePdSamplePointInitializer(16, 2, 1)",
            describe(samplePoints.call(JSValue{}, {JSValue(16), JSValue(2), JSValue(1)})));
        pushHelper("generatePdSamplePointInitializer(8, 3, 1.5)",
            describe(samplePoints.call(JSValue{}, {JSValue(8), JSValue(3), JSValue(1.5)})));
        pushHelper("generatePdSamplePointInitializer(4, 1, 1)",
            describe(samplePoints.call(JSValue{}, {JSValue(4), JSValue(1), JSValue(1)})));

        JsonValue output = JsonValue::Object{};
        output["modules"] = std::move(moduleNodes);
        output["helpers"] = std::move(helperNodes);
        std::cout << output.dump() << '\n';
    } catch (const std::exception& error) {
        std::cout << JsonValue(JsonValue::Object{{"probeError", error.what()}}).dump() << '\n';
        return 1;
    }
    return 0;
}
