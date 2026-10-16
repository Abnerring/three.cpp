// Unit coverage for the ported examples/jsm/shaders cluster.
//
// The differential probe (JsmShadersParityProbe + compare-jsm-shaders.mjs) owns the
// byte-exact comparison against the pinned checkout. These assertions pin the shape of the
// native modules without an upstream process: export and field counts per module, exact
// string identities that must survive any refactor, and the deterministic helper results.
#include "catch.hpp"
#include "constants.h"
#include "core/JSValue.h"
#include "core/NativeObject.h"
#include "core/TypedArray.h"
#include "textures/DataTexture.h"
#include "math/Vector2.h"
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
#include <string>
#include <vector>

namespace {

struct ModuleCase {
    const char* file;
    JSValue (*resolve)();
    std::size_t exports;
    std::size_t fields;
    std::size_t uniforms;
    std::size_t glsl;
};

const ModuleCase kModules[] = {
    {"ACESFilmicToneMappingShader.js", &ACESFilmicToneMappingShader::nativeACESFilmicToneMappingShader, 1, 4, 2, 2},
    {"AfterimageShader.js", &AfterimageShader::nativeAfterimageShader, 1, 4, 3, 2},
    {"BasicShader.js", &BasicShader::nativeBasicShader, 1, 4, 0, 2},
    {"BleachBypassShader.js", &BleachBypassShader::nativeBleachBypassShader, 1, 4, 2, 2},
    {"BlendShader.js", &BlendShader::nativeBlendShader, 1, 4, 4, 2},
    {"BokehShader.js", &BokehShader::nativeBokehShader, 1, 5, 8, 2},
    {"BokehShader2.js", &BokehShader2::nativeBokehShader2, 2, 8, 25, 4},
    {"BrightnessContrastShader.js", &BrightnessContrastShader::nativeBrightnessContrastShader, 1, 4, 3, 2},
    {"ColorCorrectionShader.js", &ColorCorrectionShader::nativeColorCorrectionShader, 1, 4, 4, 2},
    {"ColorifyShader.js", &ColorifyShader::nativeColorifyShader, 1, 4, 2, 2},
    {"ConvolutionShader.js", &ConvolutionShader::nativeConvolutionShader, 1, 5, 3, 2},
    {"CopyShader.js", &CopyShader::nativeCopyShader, 1, 4, 2, 2},
    {"DOFMipMapShader.js", &DOFMipMapShader::nativeDOFMipMapShader, 1, 4, 4, 2},
    {"DepthLimitedBlurShader.js", &DepthLimitedBlurShader::nativeDepthLimitedBlurShader, 2, 8, 8, 2},
    {"DigitalGlitch.js", &DigitalGlitch::nativeDigitalGlitch, 1, 4, 11, 2},
    {"DotScreenShader.js", &DotScreenShader::nativeDotScreenShader, 1, 4, 5, 2},
    {"ExposureShader.js", &ExposureShader::nativeExposureShader, 1, 4, 2, 2},
    {"FXAAShader.js", &FXAAShader::nativeFXAAShader, 1, 4, 2, 2},
    {"FilmShader.js", &FilmShader::nativeFilmShader, 1, 4, 4, 2},
    {"FocusShader.js", &FocusShader::nativeFocusShader, 1, 4, 5, 2},
    {"FreiChenShader.js", &FreiChenShader::nativeFreiChenShader, 1, 4, 2, 2},
    {"GTAOShader.js", &GTAOShader::nativeGTAOShader, 4, 14, 21, 6},
    {"GammaCorrectionShader.js", &GammaCorrectionShader::nativeGammaCorrectionShader, 1, 4, 1, 2},
    {"HalftoneShader.js", &HalftoneShader::nativeHalftoneShader, 1, 4, 13, 2},
    {"HorizontalBlurShader.js", &HorizontalBlurShader::nativeHorizontalBlurShader, 1, 4, 2, 2},
    {"HorizontalTiltShiftShader.js", &HorizontalTiltShiftShader::nativeHorizontalTiltShiftShader, 1, 4, 3, 2},
    {"HueSaturationShader.js", &HueSaturationShader::nativeHueSaturationShader, 1, 4, 3, 2},
    {"KaleidoShader.js", &KaleidoShader::nativeKaleidoShader, 1, 4, 3, 2},
    {"LuminosityHighPassShader.js", &LuminosityHighPassShader::nativeLuminosityHighPassShader, 1, 4, 5, 2},
    {"LuminosityShader.js", &LuminosityShader::nativeLuminosityShader, 1, 4, 1, 2},
    {"MirrorShader.js", &MirrorShader::nativeMirrorShader, 1, 4, 2, 2},
    {"NormalMapShader.js", &NormalMapShader::nativeNormalMapShader, 1, 4, 4, 2},
    {"OutputShader.js", &OutputShader::nativeOutputShader, 1, 4, 2, 2},
    {"PoissonDenoiseShader.js", &PoissonDenoiseShader::nativePoissonDenoiseShader, 2, 5, 11, 2},
    {"RGBShiftShader.js", &RGBShiftShader::nativeRGBShiftShader, 1, 4, 3, 2},
    {"SAOShader.js", &SAOShader::nativeSAOShader, 1, 5, 14, 2},
    {"SMAAShader.js", &SMAAShader::nativeSMAAShader, 3, 14, 9, 6},
    {"SSAOShader.js", &SSAOShader::nativeSSAOShader, 3, 14, 17, 6},
    {"SSRShader.js", &SSRShader::nativeSSRShader, 3, 14, 19, 6},
    {"SepiaShader.js", &SepiaShader::nativeSepiaShader, 1, 4, 2, 2},
    {"SobelOperatorShader.js", &SobelOperatorShader::nativeSobelOperatorShader, 1, 4, 2, 2},
    {"SubsurfaceScatteringShader.js", &SubsurfaceScatteringShader::nativeSubsurfaceScatteringShader, 1, 4, 72, 2},
    {"TechnicolorShader.js", &TechnicolorShader::nativeTechnicolorShader, 1, 4, 1, 2},
    {"ToonShader.js", &ToonShader::nativeToonShader, 4, 13, 25, 8},
    {"TriangleBlurShader.js", &TriangleBlurShader::nativeTriangleBlurShader, 1, 4, 2, 2},
    {"UnpackDepthRGBAShader.js", &UnpackDepthRGBAShader::nativeUnpackDepthRGBAShader, 1, 4, 2, 2},
    {"VelocityShader.js", &VelocityShader::nativeVelocityShader, 1, 4, 14, 2},
    {"VerticalBlurShader.js", &VerticalBlurShader::nativeVerticalBlurShader, 1, 4, 2, 2},
    {"VerticalTiltShiftShader.js", &VerticalTiltShiftShader::nativeVerticalTiltShiftShader, 1, 4, 3, 2},
    {"VignetteShader.js", &VignetteShader::nativeVignetteShader, 1, 4, 3, 2},
    {"VolumeShader.js", &VolumeShader::nativeVolumeShader, 1, 4, 6, 2},
    {"WaterRefractionShader.js", &WaterRefractionShader::nativeWaterRefractionShader, 1, 4, 5, 2},
};

// The first line of a shader source, used as a compact exact-text identity.
std::string firstLine(const std::string& text) {
    const auto end = text.find('\n');
    return end == std::string::npos ? text : text.substr(0, end);
}

JSValue call(const JSValue& receiver, const char* key, const std::vector<JSValue>& args) {
    const auto function = receiver.get(key);
    return function.call(receiver, args);
}

} // namespace

TEST_CASE("pinned JSM shader modules expose their export, field, uniform and GLSL counts", "[jsmshaders]") {
    for (const auto& entry : kModules) {
        INFO(entry.file);
        const auto module = entry.resolve();
        REQUIRE(module.keys().size() == entry.exports);
        std::size_t fields = 0, uniforms = 0, glsl = 0;
        for (const auto& name : module.keys()) {
            const auto value = module.get(name);
            REQUIRE(value.isObject());
            const auto keys = value.keys();
            fields += keys.size();
            for (const auto& key : keys) {
                const auto field = value.get(key);
                if (key == "uniforms") {
                    uniforms += field.keys().size();
                } else if (key == "vertexShader" || key == "fragmentShader") {
                    glsl ++;
                    // No shader source may be empty or whitespace-only, and every one of
                    // them must still carry its entry point.
                    REQUIRE(field.type() == JSValue::Type::String);
                    REQUIRE(! field.string().empty());
                    REQUIRE(field.string().find_first_not_of(" \t\r\n") != std::string::npos);
                    // FreiChenShader writes the fragment entry point as `void main(void)`.
                    REQUIRE(field.string().find("void main(") != std::string::npos);
                } else if (key == "defines") {
                    REQUIRE(field.keys().size() > 0);
                } else if (key == "name") {
                    REQUIRE(! field.string().empty());
                }
            }
        }
        REQUIRE(fields == entry.fields);
        REQUIRE(uniforms == entry.uniforms);
        REQUIRE(glsl == entry.glsl);
    }
}

TEST_CASE("pinned JSM shader modules keep exact GLSL and field identities", "[jsmshaders]") {
    const auto copy = CopyShader::nativeCopyShader().get("CopyShader");
    REQUIRE(copy.get("name").string() == "CopyShader");
    // CopyShader keeps the pinned leading blank lines of the fragment source.
    REQUIRE(copy.get("fragmentShader").string().rfind("\n\n\t\tuniform float opacity;", 0) == 0);

    // The generic full-screen vertex shader is shared verbatim across most modules.
    REQUIRE(copy.get("vertexShader").string() ==
        ACESFilmicToneMappingShader::nativeACESFilmicToneMappingShader()
            .get("ACESFilmicToneMappingShader")
            .get("vertexShader")
            .string());
    REQUIRE(BasicShader::nativeBasicShader().get("BasicShader").get("vertexShader").string() ==
        "\n\n\t\tvoid main() {\n\n\t\t\tgl_Position = projectionMatrix * modelViewMatrix * vec4( position, 1.0 );\n\n\t\t}");

    const auto convolution = ConvolutionShader::nativeConvolutionShader().get("ConvolutionShader");
    REQUIRE(convolution.get("defines").get("KERNEL_SIZE_FLOAT").string() == "25.0");
    REQUIRE(convolution.get("defines").get("KERNEL_SIZE_INT").string() == "25");
    REQUIRE(convolution.get("uniforms").get("uImageIncrement").get("value").get("x").toNumber() == 0.001953125);
    REQUIRE(convolution.get("uniforms").get("cKernel").get("value").keys().empty());

    const auto smaaModule = SMAAShader::nativeSMAAShader();
    REQUIRE(smaaModule.keys() == std::vector<std::string>({"SMAABlendShader", "SMAAEdgesShader", "SMAAWeightsShader"}));
    const auto weights = smaaModule.get("SMAAWeightsShader");
    REQUIRE(weights.get("defines").get("SMAA_AREATEX_PIXEL_SIZE").string() == "( 1.0 / vec2( 160.0, 560.0 ) )");
    REQUIRE(weights.get("defines").get("SMAA_THRESHOLD").isUndefined());
    REQUIRE(smaaModule.get("SMAAEdgesShader").get("defines").get("SMAA_THRESHOLD").string() == "0.1");

    // VelocityShader composes the pinned UniformsLib groups and keeps their order.
    const auto velocity = VelocityShader::nativeVelocityShader().get("VelocityShader");
    REQUIRE(velocity.get("uniforms").keys() ==
        std::vector<std::string>({"diffuse", "opacity", "map", "mapTransform", "alphaMap", "alphaMapTransform", "alphaTest",
            "displacementMap", "displacementMapTransform", "displacementScale", "displacementBias", "modelMatrixPrev",
            "currentProjectionViewMatrix", "previousProjectionViewMatrix"}));

    // SubsurfaceScatteringShader rewrites the phong chunk at module scope.
    const auto subsurface = SubsurfaceScatteringShader::nativeSubsurfaceScatteringShader().get("SubsurfaceScatteringShader");
    REQUIRE(subsurface.get("vertexShader").string().rfind("#define USE_UV\n", 0) == 0);
    REQUIRE(subsurface.get("fragmentShader").string().rfind("#define USE_UV\n#define SUBSURFACE\n", 0) == 0);
    REQUIRE(subsurface.get("fragmentShader").string().find("\tvec3 thickness = thicknessColor * texture2D(thicknessMap, uv).r;") !=
        std::string::npos);
    REQUIRE(subsurface.get("uniforms").keys().size() == 72);

    // ToonShader keeps one shared vertex source for three of its four definitions.
    const auto toon = ToonShader::nativeToonShader();
    REQUIRE(toon.keys() == std::vector<std::string>({"ToonShader1", "ToonShader2", "ToonShaderDotted", "ToonShaderHatching"}));
    const auto toonVertex = toon.get("ToonShader2").get("vertexShader").string();
    REQUIRE(toon.get("ToonShaderHatching").get("vertexShader").string() == toonVertex);
    REQUIRE(toon.get("ToonShaderDotted").get("vertexShader").string() == toonVertex);
    REQUIRE(toon.get("ToonShader1").get("vertexShader").string() != toonVertex);

    // Working colour space components of the pinned `new Color( hex )` uniforms.
    const auto colorify = ColorifyShader::nativeColorifyShader().get("ColorifyShader");
    REQUIRE(colorify.get("uniforms").get("color").get("value").get("r").toNumber() == 1);
    REQUIRE(colorify.get("uniforms").get("color").get("value").get("g").toNumber() == 1);
    REQUIRE(colorify.get("uniforms").get("color").get("value").get("b").toNumber() == 1);
    const auto highPassColor =
        LuminosityHighPassShader::nativeLuminosityHighPassShader().get("LuminosityHighPassShader").get("uniforms").get("defaultColor").get("value");
    REQUIRE(highPassColor.get("isColor").toBoolean());
    REQUIRE(highPassColor.get("r").toNumber() == 0);
    REQUIRE(highPassColor.get("b").toNumber() == 0);

    const auto gtao = GTAOShader::nativeGTAOShader().get("GTAOShader");
    REQUIRE(gtao.get("uniforms").get("sceneBoxMin").get("value").get("x").toNumber() == -1);
    REQUIRE(gtao.get("defines").get("SCREEN_SPACE_RADIUS_SCALE").toNumber() == 100);
    REQUIRE(gtao.get("uniforms").get("cameraProjectionMatrix").get("value").get("elements").get("15").toNumber() == 1);
}

TEST_CASE("pinned JSM shader helper functions reproduce the upstream results", "[jsmshaders]") {
    const auto poissonModule = PoissonDenoiseShader::nativePoissonDenoiseShader();
    const auto initializer = poissonModule.get("generatePdSamplePointInitializer");
    REQUIRE(initializer.get("name").string() == "generatePdSamplePointInitializer");
    REQUIRE(initializer.get("length").toNumber() == 3);
    const auto sampleVectors = initializer.call(JSValue{}, {JSValue(16), JSValue(2), JSValue(1)});
    REQUIRE(sampleVectors.string() ==
        poissonModule.get("PoissonDenoiseShader").get("defines").get("SAMPLE_VECTORS").string());
    REQUIRE(sampleVectors.string().rfind("vec3[SAMPLES](vec3(1, 0, 0),", 0) == 0);

    const auto blurUtils = DepthLimitedBlurShader::nativeDepthLimitedBlurShader().get("BlurShaderUtils");
    const auto weights = call(blurUtils, "createSampleWeights", {JSValue(4), JSValue(1)});
    REQUIRE(weights.keys() == std::vector<std::string>({"0", "1", "2", "3", "4"}));
    REQUIRE(weights.get("0").toNumber() == 0.3989422804014327);
    REQUIRE(weights.get("4").toNumber() == 0.00013383022576488537);
    const auto offsets = call(blurUtils, "createSampleOffsets",
        {JSValue(2), nativeVector(std::make_shared<Vector2>(0.25, -0.5))});
    REQUIRE(offsets.keys().size() == 3);
    REQUIRE(offsets.get("1").get("x").toNumber() == 0.25);
    REQUIRE(offsets.get("2").get("y").toNumber() == -1);

    const auto noise = call(GTAOShader::nativeGTAOShader(), "generateMagicSquareNoise", {});
    const auto texture = std::static_pointer_cast<Texture>(noise.hostPayload("three.Texture"));
    REQUIRE(texture != nullptr);
    REQUIRE(texture->image->width == 5);
    REQUIRE(texture->image->height == 5);
    REQUIRE(texture->wrapS == RepeatWrapping);
    REQUIRE(texture->wrapT == RepeatWrapping);
    REQUIRE(texture->version == 1);
    const auto* bytes = std::get_if<Uint8Array>(&texture->image->data);
    REQUIRE(bytes != nullptr);
    REQUIRE(bytes->size() == 100);
    REQUIRE(double((*bytes)[0]) == 46);
    REQUIRE(double((*bytes)[2]) == 127);
    REQUIRE(double((*bytes)[3]) == 255);
}
