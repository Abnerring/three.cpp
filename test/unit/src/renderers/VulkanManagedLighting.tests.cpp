#include "renderers/VulkanRenderer.h"
#include "core/BufferGeometry.h"
#include "core/BufferAttribute.h"
#include "scenes/Scene.h"
#include "cameras/OrthographicCamera.h"
#include "geometries/PlaneGeometry.h"
#include "materials/MeshLambertMaterial.h"
#include "materials/MeshPhongMaterial.h"
#include "materials/MeshNormalMaterial.h"
#include "materials/MeshStandardMaterial.h"
#include "materials/MeshPhysicalMaterial.h"
#include "materials/MeshToonMaterial.h"
#include "materials/MeshDepthMaterial.h"
#include "materials/MeshDistanceMaterial.h"
#include "lights/AmbientLight.h"
#include "lights/DirectionalLight.h"
#include "lights/HemisphereLight.h"
#include "lights/PointLight.h"
#include "lights/SpotLight.h"
#include "lights/LightShadow.h"
#include "objects/Mesh.h"
#include "constants.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <iostream>
#include <numbers>
#include <stdexcept>
#include <string>

namespace {
using Args = std::vector<JSValue>;
using RGB = std::array<double, 3>;
constexpr double pi = std::numbers::pi;
int assertions = 0, images = 0, validationRequested = 0, validationEnabled = 0;
void require(bool value, const char* message) { ++assertions; if (!value) throw std::runtime_error(message); }
JSValue call(JSValue object, const char* method, const Args& args = {}) { return object.get(method).call(object, args); }
void rgb(JSValue color, RGB value) { call(color, "setRGB", {value[0], value[1], value[2]}); }
void xyz(JSValue vector, double x, double y, double z) { call(vector, "set", {x, y, z}); }
struct World {
    JSValue scene = nativeSceneConstructor().construct();
    JSValue camera = nativeOrthographicCameraConstructor().construct({-1, 1, 1, -1, .1, 10});
    JSValue geometry = nativePlaneGeometryConstructor().construct({1.6, 1.6});
    JSValue paint, mesh;
    explicit World(JSValue material) : paint(material), mesh(nativeMeshConstructor().construct({geometry, paint})) {
        xyz(camera.get("position"), 0, 0, 3); mesh.set("frustumCulled", false); call(scene, "add", {mesh});
        require(!paint.isHostObject("three.Material") && !mesh.isHostObject("three.Object3D"), "lighting uses original canonical source objects");
    }
};
void expectPixel(const VulkanRenderer& renderer, RGB linear, const char* message, unsigned x = 32, unsigned y = 32) {
    const auto bytes = renderer.readPixels();
    require(bytes.size() == renderer.getWidth() * renderer.getHeight() * 4, "complete lighting RGBA image readback");
    const auto offset = (y * renderer.getWidth() + x) * 4;
    bool matches = true;
    for (unsigned c = 0; c < 3; ++c) {
        const auto expected = int(std::lround(std::clamp(linear[c], 0., 1.) * 255));
        matches = matches && std::abs(int(bytes.at(offset + c)) - expected) <= 2;
    }
    if (!matches) {
        std::cerr << message << " actual=" << int(bytes.at(offset)) << ',' << int(bytes.at(offset + 1)) << ',' << int(bytes.at(offset + 2))
                  << " expectedLinear=" << linear[0] << ',' << linear[1] << ',' << linear[2] << '\n';
    }
    require(matches, message); require(bytes.at(offset + 3) == 255, "opaque source lighting alpha is one"); ++images;
}
void validate(const VulkanRenderer& renderer, bool requested) {
    if (requested) ++validationRequested; if (renderer.getInfo().validationEnabled) ++validationEnabled;
    const auto messages = renderer.getValidationMessages(); for (const auto& message : messages) std::cerr << message << '\n';
    require(messages.empty(), "no collected lighting Vulkan validation errors");
}
void lightClasses() {
    const std::array constructors{nativeAmbientLightConstructor(), nativeDirectionalLightConstructor(), nativeHemisphereLightConstructor(), nativePointLightConstructor(), nativeSpotLightConstructor()};
    for (const auto& constructor : constructors) {
        const auto light = constructor.construct();
        require(light.instanceOf(nativeLightConstructor()) && light.get("isLight").strictEquals(true), "concrete source light inherits the canonical Light class");
        require(light.get("intensity").strictEquals(1) && light.get("castShadow").strictEquals(false), "source light preserves default intensity and disabled shadows");
        require(!light.isHostObject("three.Object3D"), "source light is not a typed graph adapter");
        const auto copy = call(light, "clone");
        require(copy.instanceOf(constructor) && !copy.sameValue(light), "source inherited clone preserves the actual derived constructor");
        require(!copy.get("color").sameValue(light.get("color")), "source light clone owns its copied Color");
        if (light.get("shadow").isObject()) {
            require(!copy.get("shadow").sameValue(light.get("shadow")), "source concrete light copy clones its shadow configuration");
            const auto shadow = light.get("shadow");
            require(shadow.instanceOf(nativeLightShadowConstructor()), "concrete shadow inherits the canonical source LightShadow");
            require(shadow.get("map").isNull() && shadow.get("mapPass").isNull(), "source shadow starts without allocated GPU maps");
            require(shadow.get("camera").get("near").strictEquals(.5) && shadow.get("camera").get("far").strictEquals(500), "source light shadow camera preserves near and far defaults");
            require(call(shadow, "getViewportCount").strictEquals(1) && call(shadow, "getCamera").sameValue(shadow.get("camera")), "source shadow accessors preserve camera identity and viewport count");
        }
    }
    auto directional = constructors[1].construct(), hemi = constructors[2].construct(), point = constructors[3].construct(), spot = constructors[4].construct();
    require(directional.get("position").get("y").strictEquals(1) && hemi.get("position").get("y").strictEquals(1) && spot.get("position").get("y").strictEquals(1), "source directional hemisphere and spot lights use Object3D.DEFAULT_UP");
    require(point.get("decay").strictEquals(2) && spot.get("decay").strictEquals(2) && point.get("distance").strictEquals(0), "source punctual lights preserve inverse-square and unlimited-range defaults");
    require(std::abs(spot.get("angle").number() - pi / 3) < 1e-12 && spot.get("penumbra").strictEquals(0), "source SpotLight cone and penumbra defaults match the pin");
    point.set("power", 8 * pi); spot.set("power", 3 * pi);
    require(std::abs(point.get("intensity").number() - 2) < 1e-12 && std::abs(point.get("power").number() - 8 * pi) < 1e-12, "source PointLight power getter and setter execute their original units conversion");
    require(std::abs(spot.get("intensity").number() - 3) < 1e-12 && std::abs(spot.get("power").number() - 3 * pi) < 1e-12, "source SpotLight power getter and setter execute their original units conversion");
    xyz(spot.get("position"), 0, 0, 2); call(spot, "updateMatrixWorld"); call(spot.get("target"), "updateMatrixWorld");
    call(spot.get("shadow"), "updateMatrices", {spot});
    require(std::abs(spot.get("shadow").get("camera").get("fov").number() - 120) < 1e-10, "original SpotLightShadow updates its real PerspectiveCamera field of view");
    require(std::abs(spot.get("shadow").get("camera").get("position").get("z").number() - 2) < 1e-12, "original shadow updateMatrices copies the actual world position");
}
void normalMatrices(bool validation) {
    VulkanRenderer renderer({64, 64, validation}); renderer.srgbOutput = false;
    World world(nativeMeshNormalMaterialConstructor().construct());
    require(world.paint.get("flatShading").strictEquals(false), "source Normal default flatShading is false");
    require(world.paint.get("normalMap").isNull(), "source Normal default normalMap is null");
    renderer.render(world.scene, world.camera);
    // meshNormal fragment: normalize(normal) * .5 + .5, without an output transform.
    expectPixel(renderer, {.5, .5, 1}, "original Normal shader packs the +Z view normal");
    world.mesh.get("rotation").set("y", pi / 6);
    renderer.render(world.scene, world.camera);
    expectPixel(renderer, {.75, .5, .5 + std::sqrt(3.) / 4}, "source object rotation updates the real normalMatrix");
    world.mesh.get("rotation").set("y", 0);
    xyz(world.camera.get("position"), 1.5, 0, 1.5 * std::sqrt(3.)); call(world.camera, "lookAt", {0, 0, 0});
    renderer.render(world.scene, world.camera);
    expectPixel(renderer, {.25, .5, .5 + std::sqrt(3.) / 4}, "camera matrixWorldInverse rotates the actual shader normal");
    xyz(world.camera.get("position"), 0, 0, 3); call(world.camera, "lookAt", {0, 0, 0});
    auto normal = world.geometry.get("attributes").get("normal");
    for (unsigned i = 0; i < unsigned(normal.get("count").number()); ++i) call(normal, "setXYZ", {i, std::sqrt(.5), 0, std::sqrt(.5)});
    normal.set("needsUpdate", true); xyz(world.mesh.get("scale"), 2, 1, 1);
    renderer.render(world.scene, world.camera);
    expectPixel(renderer, {.5 + .5 / std::sqrt(5.), .5, .5 + 1 / std::sqrt(5.)}, "inverse-transpose normalMatrix handles nonuniform scale and source needsUpdate");
    validate(renderer, validation);
}
void ambientDiffuse(bool validation, bool phong) {
    VulkanRenderer renderer({64, 64, validation}); renderer.srgbOutput = false;
    World world((phong ? nativeMeshPhongMaterialConstructor() : nativeMeshLambertMaterialConstructor()).construct());
    auto light = nativeAmbientLightConstructor().construct();
    require(light.get("intensity").strictEquals(1), "source AmbientLight default intensity is one");
    require(light.get("color").get("r").strictEquals(1) && light.get("color").get("g").strictEquals(1) && light.get("color").get("b").strictEquals(1), "source AmbientLight default color is linear white");
    require(light.instanceOf(nativeAmbientLightConstructor()), "AmbientLight keeps its canonical source prototype");
    require(world.paint.get("color").get("r").strictEquals(1), "source lit material default diffuse is white");
    require(world.paint.get("emissiveIntensity").strictEquals(1), "source lit material default emissiveIntensity is one");
    if (phong) require(world.paint.get("shininess").strictEquals(30), "source Phong default shininess is thirty");
    renderer.render(world.scene, world.camera); expectPixel(renderer, {0, 0, 0}, "lit source material without light has zero reflected radiance");
    call(world.scene, "add", {light}); renderer.render(world.scene, world.camera);
    // lights_pars_begin returns ambient irradiance unchanged; BRDF_Lambert = diffuse / PI.
    expectPixel(renderer, {1 / pi, 1 / pi, 1 / pi}, "source default ambient irradiance uses the Lambert reciprocal-PI BRDF");
    const auto version = world.paint.get("version"); light.set("intensity", pi / 4);
    renderer.render(world.scene, world.camera); expectPixel(renderer, {.25, .25, .25}, "light intensity mutation refreshes actual ambient uniform bytes");
    require(world.paint.get("version").sameValue(version), "light mutation does not require a material version change");
    rgb(world.paint.get("color"), {.25, .5, .75}); renderer.render(world.scene, world.camera);
    expectPixel(renderer, {.0625, .125, .1875}, "source diffuse uniform multiplies the actual ambient irradiance");
    rgb(world.paint.get("emissive"), {.1, .2, .3}); world.paint.set("emissiveIntensity", .5);
    renderer.render(world.scene, world.camera);
    expectPixel(renderer, {.1125, .225, .3375}, "source emissive refresh adds radiance after diffuse lighting");
    validate(renderer, validation);
}
void sharedLitProgram(bool validation) {
    VulkanRenderer renderer({64, 64, validation}); renderer.srgbOutput = false;
    World world(nativeMeshLambertMaterialConstructor().construct());
    call(world.scene, "remove", {world.mesh});
    const auto geometry = nativePlaneGeometryConstructor().construct({.7, 1.4});
    auto secondPaint = nativeMeshLambertMaterialConstructor().construct();
    auto left = nativeMeshConstructor().construct({geometry, world.paint}), right = nativeMeshConstructor().construct({geometry, secondPaint});
    xyz(left.get("position"), -.5, 0, 0); xyz(right.get("position"), .5, 0, 0);
    rgb(world.paint.get("color"), {.8, .4, .2}); rgb(secondPaint.get("color"), {.2, .6, .8});
    auto light = nativeAmbientLightConstructor().construct({0xffffff, pi / 2}); call(world.scene, "add", {left, right, light});
    renderer.render(world.scene, world.camera);
    expectPixel(renderer, {.4, .2, .1}, "first lit material retains its own diffuse uniform bytes", 16);
    expectPixel(renderer, {.1, .3, .4}, "second lit material uploads distinct uniforms into the shared source program", 48);
    require(renderer.info.get("programs").get("length").strictEquals(1), "two equivalent source Lambert materials share one actual linked program");
    const auto color = nativeFloat32BufferAttributeConstructor().construct({[] { auto a = JSValue::array(); for (unsigned i = 0; i < 4; ++i) for (double c : {1., .5, .25}) a.set(JSValue(a.length()), c); return a; }(), 3});
    call(geometry, "setAttribute", {"color", color});
    const auto version = secondPaint.get("version").number(); secondPaint.set("vertexColors", true); secondPaint.set("needsUpdate", true);
    require(secondPaint.get("version").number() == version + 1, "source needsUpdate increments the actual lit material version");
    renderer.render(world.scene, world.camera);
    expectPixel(renderer, {.4, .2, .1}, "shared old source program retains its first material after the second recompiles", 16);
    expectPixel(renderer, {.1, .15, .1}, "lit needsUpdate compiles the actual vertex-color shader variant", 48);
    require(renderer.info.get("programs").get("length").strictEquals(2), "source shader variant has its own actual linked program");
    validate(renderer, validation);
}
void directionalArrays(bool validation) {
    VulkanRenderer renderer({64, 64, validation}); renderer.srgbOutput = false;
    World world(nativeMeshLambertMaterialConstructor().construct()); auto light = nativeDirectionalLightConstructor().construct({0xffffff, pi / 2});
    call(world.scene, "add", {light, light.get("target")});
    renderer.render(world.scene, world.camera); expectPixel(renderer, {0, 0, 0}, "default directional +Y light is tangent to the source +Z plane");
    xyz(light.get("position"), 0, 0, 1); renderer.render(world.scene, world.camera);
    expectPixel(renderer, {.5, .5, .5}, "source directional structure uploads its front-facing direction and intensity");
    light.set("intensity", pi / 4); xyz(light.get("target").get("position"), std::sqrt(3.), 0, 0);
    renderer.render(world.scene, world.camera); expectPixel(renderer, {.125, .125, .125}, "source directional target matrix changes the cosine irradiance");
    xyz(world.camera.get("position"), 1.5, 0, 1.5 * std::sqrt(3.)); call(world.camera, "lookAt", {0, 0, 0});
    renderer.render(world.scene, world.camera); expectPixel(renderer, {.125, .125, .125}, "camera transforms both source light direction and normal into the same view space");
    xyz(world.camera.get("position"), 0, 0, 3); call(world.camera, "lookAt", {0, 0, 0}); xyz(light.get("target").get("position"), 0, 0, 0);
    rgb(light.get("color"), {1, 0, 0}); light.set("intensity", pi * .2);
    auto second = nativeDirectionalLightConstructor().construct({0x00ff00, pi * .3}), third = nativeDirectionalLightConstructor().construct({0x0000ff, pi * .1});
    xyz(second.get("position"), 0, 0, 1); xyz(third.get("position"), 0, 0, 1); call(world.scene, "add", {second, third, second.get("target"), third.get("target")});
    renderer.render(world.scene, world.camera); expectPixel(renderer, {.2, .3, .1}, "three original directional uniform structures retain separate color and direction slots");
    rgb(second.get("color"), {0, 0, 1}); second.set("intensity", pi * .4);
    renderer.render(world.scene, world.camera); expectPixel(renderer, {.2, 0, .5}, "middle structure mutation updates the correct uniform-array element");
    call(world.scene, "remove", {light, third}); renderer.render(world.scene, world.camera);
    expectPixel(renderer, {0, 0, .4}, "source light-count shader change removes stale structure-array elements");
    validate(renderer, validation);
}
void hemisphere(bool validation) {
    VulkanRenderer renderer({64, 64, validation}); renderer.srgbOutput = false;
    World world(nativeMeshLambertMaterialConstructor().construct()); auto light = nativeHemisphereLightConstructor().construct({0xff0000, 0x0000ff, pi / 2});
    call(world.scene, "add", {light}); renderer.render(world.scene, world.camera);
    // hemiDiffuseWeight = .5 * dot(normal,direction) + .5; irradiance / PI.
    expectPixel(renderer, {.25, 0, .25}, "default source hemisphere +Y direction evenly mixes sky and ground on the +Z plane");
    xyz(light.get("position"), 0, 0, 1); renderer.render(world.scene, world.camera); expectPixel(renderer, {.5, 0, 0}, "source hemisphere front direction selects the sky color");
    xyz(light.get("position"), 0, 0, -1); renderer.render(world.scene, world.camera); expectPixel(renderer, {0, 0, .5}, "source hemisphere back direction selects the ground color");
    xyz(light.get("position"), 0, 1, 0); rgb(light.get("color"), {.2, .4, .6}); rgb(light.get("groundColor"), {.8, .2, .1}); light.set("intensity", pi * .2);
    renderer.render(world.scene, world.camera); expectPixel(renderer, {.1, .06, .07}, "source hemisphere structure refreshes both colors and its strength");
    validate(renderer, validation);
}
double punctualReference(unsigned x, unsigned y, double height, double intensity, double decay, double cutoff, double spotAngle = pi, double penumbra = 0) {
    // Orthographic pixel centers on the untransformed world plane, independent
    // of framebuffer Y orientation because the centered light is symmetric.
    const auto px = (double(x) + .5) / 32 - 1, py = (double(y) + .5) / 32 - 1;
    const auto distance = std::sqrt(px * px + py * py + height * height), cosine = height / distance;
    auto attenuation = 1 / std::max(std::pow(distance, decay), .01);
    if (cutoff > 0) attenuation *= std::pow(std::max(1 - std::pow(distance / cutoff, 4), 0.), 2);
    if (spotAngle < pi) {
        const auto a = std::cos(spotAngle), b = std::cos(spotAngle * (1 - penumbra));
        const auto t = b == a ? double(cosine >= b) : std::clamp((cosine - a) / (b - a), 0., 1.);
        attenuation *= t * t * (3 - 2 * t);
    }
    return intensity / pi * cosine * attenuation;
}
void punctual(bool validation, bool spot) {
    VulkanRenderer renderer({64, 64, validation}); renderer.srgbOutput = false;
    World world(nativeMeshLambertMaterialConstructor().construct());
    auto light = (spot ? nativeSpotLightConstructor() : nativePointLightConstructor()).construct();
    xyz(light.get("position"), 0, 0, 2); call(world.scene, "add", {light}); if (spot) call(world.scene, "add", {light.get("target")});
    renderer.render(world.scene, world.camera);
    const auto defaultValue = punctualReference(32, 32, 2, 1, 2, 0);
    expectPixel(renderer, {defaultValue, defaultValue, defaultValue}, "source punctual defaults use inverse-square distance attenuation");
    light.set("intensity", 2 * pi); renderer.render(world.scene, world.camera);
    for (const auto x : {16u, 32u, 47u}) { const auto value = punctualReference(x, 32, 2, 2 * pi, 2, 0); expectPixel(renderer, {value, value, value}, "actual punctual position and decay match pixel-center distance gold", x); }
    if (spot) {
        light.set("angle", pi / 12); light.set("penumbra", .5); renderer.render(world.scene, world.camera);
        for (const auto x : {16u, 32u, 47u}) { const auto value = punctualReference(x, 32, 2, 2 * pi, 2, 0, pi / 12, .5); expectPixel(renderer, {value, value, value}, "source SpotLight angle and penumbra structures apply smoothstep attenuation", x); }
        xyz(light.get("target").get("position"), 2, 0, 0); renderer.render(world.scene, world.camera); expectPixel(renderer, {0, 0, 0}, "moving the source spot target rotates its cone away from the tested surface");
    } else {
        light.set("distance", 3); renderer.render(world.scene, world.camera);
        const auto cutoff = punctualReference(32, 32, 2, 2 * pi, 2, 3); expectPixel(renderer, {cutoff, cutoff, cutoff}, "source PointLight distance applies the fourth-power cutoff window");
        xyz(light.get("position"), 0, 0, 4); renderer.render(world.scene, world.camera); expectPixel(renderer, {0, 0, 0}, "source point light outside its cutoff contributes zero radiance");
        light.set("distance", 0); light.set("decay", 0); light.set("intensity", .3 * pi); renderer.render(world.scene, world.camera);
        const auto unattenuated = punctualReference(32, 32, 4, .3 * pi, 0, 0); expectPixel(renderer, {unattenuated, unattenuated, unattenuated}, "source zero decay removes inverse-square attenuation");
    }
    validate(renderer, validation);
}
RGB phongReference(RGB diffuse, RGB specular, double shininess, double intensity, double cosine) {
    const auto halfCosine = std::sqrt((1 + cosine) / 2);
    const auto fresnel = std::exp2((-5.55473 * halfCosine - 6.98316) * halfCosine);
    const auto specularScale = intensity * cosine * .25 / pi * (shininess * .5 + 1) * std::pow(halfCosine, shininess);
    RGB result{};
    for (unsigned c = 0; c < 3; ++c) result[c] = intensity * cosine / pi * diffuse[c] + (specular[c] * (1 - fresnel) + fresnel) * specularScale;
    return result;
}
void phongSpecular(bool validation) {
    VulkanRenderer renderer({64, 64, validation}); renderer.srgbOutput = false;
    World world(nativeMeshPhongMaterialConstructor().construct()); auto light = nativeDirectionalLightConstructor().construct({0xffffff, .2 * pi});
    xyz(light.get("position"), 0, 0, 1); call(world.scene, "add", {light, light.get("target")});
    const RGB diffuse{.1, .2, .3}, specular{.2, .1, .05}; rgb(world.paint.get("color"), diffuse); rgb(world.paint.get("specular"), specular);
    renderer.render(world.scene, world.camera);
    expectPixel(renderer, phongReference(diffuse, specular, 30, .2 * pi, 1), "original Phong default shininess executes the normalized Blinn-Phong and Schlick formulas");
    world.paint.set("shininess", 2); renderer.render(world.scene, world.camera);
    expectPixel(renderer, phongReference(diffuse, specular, 2, .2 * pi, 1), "source shininess mutation updates real specular lobe energy");
    xyz(light.get("position"), std::sqrt(3.), 0, 1); renderer.render(world.scene, world.camera);
    expectPixel(renderer, phongReference(diffuse, specular, 2, .2 * pi, .5), "source Phong uses the changed half-vector and light cosine");
    validate(renderer, validation);
}
double halfValue(unsigned bits) {
    const auto exponent = (bits >> 10) & 31, fraction = bits & 1023;
    return exponent ? std::ldexp(double(1024 + fraction), int(exponent) - 25) : std::ldexp(double(fraction), -24);
}
std::array<double, 2> dfgAtNormalIncidence(double roughness) {
    // Independent gold texels from pinned src/renderers/shaders/DFGLUTData.js,
    // final row (N.V=1), sampled with its LinearFilter/ClampToEdge parameters.
    // Production DFGLUTData and generated shader code are not used by this oracle.
    if (roughness == 1) return {halfValue(0x3586), halfValue(0x049f)};
    if (roughness == .5) return {(halfValue(0x3b79) + halfValue(0x3b1d)) / 2, (halfValue(0x047c) + halfValue(0x05d4)) / 2};
    throw std::logic_error("No independent DFG reference for this test roughness");
}
double fresnel(double f0, double f90 = 1) {
    const auto tail = std::exp2(-5.55473 - 6.98316); return f0 * (1 - tail) + f90 * tail;
}
RGB physicalReference(RGB base, double roughness, double metalness, double intensity, RGB dielectric = {.04, .04, .04}, double f90 = 1, double clearcoat = 0, double coatRoughness = 1) {
    // Normal incidence: Smith visibility=.25, D_GGX=1/(PI*roughness^4).
    // RE_Direct_Physical uses dielectric Fresnel for the diffuse layer and the
    // metallic/dielectric blend for GGX, including DFG multiscattering compensation.
    const auto dfg = dfgAtNormalIncidence(roughness); const auto energy = dfg[0] + dfg[1]; RGB result{};
    for (unsigned c = 0; c < 3; ++c) {
        const auto f0 = dielectric[c] * (1 - metalness) + base[c] * metalness;
        const auto compensation = 1 + f0 * (1 / energy - 1);
        const auto diffuse = intensity / pi * base[c] * (1 - metalness) * (1 - fresnel(dielectric[c], f90));
        const auto specular = intensity * .25 / (pi * std::pow(roughness, 4)) * fresnel(f0, f90) * compensation;
        const auto coating = intensity * .25 / (pi * std::pow(coatRoughness, 4)) * fresnel(.04);
        result[c] = (diffuse + specular) * (1 - clearcoat * fresnel(.04)) + coating * clearcoat;
    }
    return result;
}
void physicalMaterials(bool validation, bool physical) {
    VulkanRenderer renderer({64, 64, validation}); renderer.srgbOutput = false;
    World world((physical ? nativeMeshPhysicalMaterialConstructor() : nativeMeshStandardMaterialConstructor()).construct());
    auto light = nativeDirectionalLightConstructor().construct({0xffffff, .15 * pi}); xyz(light.get("position"), 0, 0, 1); call(world.scene, "add", {light, light.get("target")});
    require(world.paint.get("roughness").strictEquals(1) && world.paint.get("metalness").strictEquals(0), "source PBR defaults are roughness one and metalness zero");
    if (physical) require(world.paint.get("ior").strictEquals(1.5) && world.paint.get("clearcoat").strictEquals(0) && world.paint.get("transmission").strictEquals(0), "source Physical defaults preserve IOR and disabled coat/transmission");
    renderer.render(world.scene, world.camera);
    expectPixel(renderer, physicalReference({1, 1, 1}, 1, 0, .15 * pi), "original default PBR shader executes GGX energy compensation and Fresnel diffuse conservation");
    const RGB base{.3, .5, .7}; rgb(world.paint.get("color"), base); world.paint.set("roughness", .5);
    renderer.render(world.scene, world.camera);
    expectPixel(renderer, physicalReference(base, .5, 0, .15 * pi), "source PBR roughness updates actual GGX and the sampled half-float DFG LUT");
    world.paint.set("metalness", 1); renderer.render(world.scene, world.camera);
    expectPixel(renderer, physicalReference(base, .5, 1, .15 * pi), "source PBR metalness replaces diffuse with the tinted GGX interface");
    if (physical) {
        world.paint.set("metalness", 0); world.paint.set("ior", 2); world.paint.set("specularIntensity", .5);
        renderer.render(world.scene, world.camera);
        expectPixel(renderer, physicalReference(base, .5, 0, .15 * pi, {1. / 18, 1. / 18, 1. / 18}, .5), "source Physical IOR and specularIntensity update its dielectric Fresnel values");
        world.paint.set("ior", 1.5); world.paint.set("specularIntensity", 1); world.paint.set("clearcoat", .5); world.paint.set("clearcoatRoughness", .25); world.paint.set("needsUpdate", true);
        renderer.render(world.scene, world.camera);
        expectPixel(renderer, physicalReference(base, .5, 0, .15 * pi, {.04, .04, .04}, 1, .5, .25), "original Physical clearcoat shader attenuates the base and adds its distinct GGX lobe");
    }
    validate(renderer, validation);
}
void toon(bool validation) {
    VulkanRenderer renderer({64, 64, validation}); renderer.srgbOutput = false;
    World world(nativeMeshToonMaterialConstructor().construct()); auto light = nativeDirectionalLightConstructor().construct({0xffffff, pi / 2});
    xyz(light.get("position"), 0, 0, 1); call(world.scene, "add", {light, light.get("target")});
    require(world.paint.get("gradientMap").isNull(), "source Toon defaults to the built-in two-band gradient");
    renderer.render(world.scene, world.camera); expectPixel(renderer, {.5, .5, .5}, "original Toon front-facing band has unit gradient irradiance");
    xyz(light.get("position"), 1, 0, 0); renderer.render(world.scene, world.camera);
    expectPixel(renderer, {.35, .35, .35}, "original Toon tangent direction selects the constant 0.7 band");
    light.set("intensity", pi / 4); renderer.render(world.scene, world.camera);
    expectPixel(renderer, {.175, .175, .175}, "source Toon light strength updates the selected gradient band");
    validate(renderer, validation);
}
void depthAndDistance(bool validation, bool distanceMaterial) {
    VulkanRenderer renderer({64, 64, validation}); renderer.srgbOutput = false;
    World world((distanceMaterial ? nativeMeshDistanceMaterialConstructor() : nativeMeshDepthMaterialConstructor()).construct());
    if (distanceMaterial) {
        // WebGLShadowMap stores its point light here before rendering a source
        // distance material. Use that real public source properties boundary;
        // this checks the shader/uniform path, not a shadow-map render pass.
        auto light = nativePointLightConstructor().construct(); xyz(light.get("position"), 0, 0, 2); light.get("shadow").get("camera").set("far", 5);
        call(world.scene, "add", {light}); call(renderer.object().get("properties"), "get", {world.paint}).set("light", light);
        renderer.render(world.scene, world.camera);
        const auto radial = [](double z) { return (std::sqrt(z * z + 2. / (64 * 64)) - .5) / 4.5; };
        expectPixel(renderer, {radial(2), 0, 0}, "original distance shader uses world position and actual point-shadow camera range");
        world.mesh.get("position").set("z", 1); renderer.render(world.scene, world.camera);
        expectPixel(renderer, {radial(1), 0, 0}, "source model matrix changes actual radial distance rather than camera depth");
        light.get("position").set("z", 3); renderer.render(world.scene, world.camera);
        expectPixel(renderer, {radial(2), 0, 0}, "source distance uniform refresh follows the current light matrixWorld");
    } else {
        require(world.paint.get("depthPacking").strictEquals(BasicDepthPacking), "source Depth default uses basic depth packing");
        renderer.render(world.scene, world.camera);
        const auto first = 1 - (3 - .1) / (10 - .1); expectPixel(renderer, {first, first, first}, "original Depth shader preserves GL clip depth through native SPIR-V transformation");
        world.mesh.get("position").set("z", 1); renderer.render(world.scene, world.camera);
        const auto moved = 1 - (2 - .1) / (10 - .1); expectPixel(renderer, {moved, moved, moved}, "source depth follows the actual modelViewMatrix distance");
        world.camera.set("near", 1); world.camera.set("far", 5); call(world.camera, "updateProjectionMatrix"); renderer.render(world.scene, world.camera);
        expectPixel(renderer, {.75, .75, .75}, "source depth shader follows original camera projection near and far changes");
    }
    validate(renderer, validation);
}
}
int main(int argc, char** argv) {
    try {
        bool validation = false; std::string selected;
        for (int i = 1; i < argc; ++i) { const std::string arg = argv[i]; if (arg == "--validation") validation = true; else if (arg.starts_with("--case=")) selected = arg.substr(7); }
        if (selected.empty() || selected == "classes") lightClasses();
        if (selected.empty() || selected == "normal") normalMatrices(validation);
        if (selected.empty() || selected == "lambert") ambientDiffuse(validation, false);
        if (selected.empty() || selected == "phong") ambientDiffuse(validation, true);
        if (selected.empty() || selected == "shared") sharedLitProgram(validation);
        if (selected.empty() || selected == "directional") directionalArrays(validation);
        if (selected.empty() || selected == "hemisphere") hemisphere(validation);
        if (selected.empty() || selected == "point") punctual(validation, false);
        if (selected.empty() || selected == "spot") punctual(validation, true);
        if (selected.empty() || selected == "specular") phongSpecular(validation);
        if (selected.empty() || selected == "standard") physicalMaterials(validation, false);
        if (selected.empty() || selected == "physical") physicalMaterials(validation, true);
        if (selected.empty() || selected == "toon") toon(validation);
        if (selected.empty() || selected == "depth") depthAndDistance(validation, false);
        if (selected.empty() || selected == "distance") depthAndDistance(validation, true);
        std::cout << "managed lighting assertions=" << assertions << " images=" << images << " validation_requested=" << validationRequested << " validation_enabled=" << validationEnabled << '\n';
        return 0;
    } catch (const std::exception& error) { std::cerr << "Managed source lighting Vulkan failed: " << error.what() << '\n'; return 1; }
}
