#include "renderers/VulkanRenderer.h"
#include "core/BufferGeometry.h"
#include "core/BufferAttribute.h"
#include "core/JSGlobal.h"
#include "scenes/Scene.h"
#include "cameras/OrthographicCamera.h"
#include "materials/MeshBasicMaterial.h"
#include "materials/ShaderMaterial.h"
#include "objects/Mesh.h"
#include "geometries/PlaneGeometry.h"
#include "textures/DataTexture.h"
#include "math/Vector4.h"
#include <array>
#include <cmath>
#include <iostream>
#include <stdexcept>

namespace {
using Args = std::vector<JSValue>;
int assertions = 0;
void require(bool value, const char* message) { ++assertions; if (!value) throw std::runtime_error(message); }
JSValue call(JSValue object, const char* name, const Args& args = {}) { return object.get(name).call(object, args); }
JSValue list(const Args& values = {}) { auto result = JSValue::array(); for (const auto& value : values) result.set(JSValue(result.length()), value); return result; }
JSValue camera() { auto result = nativeOrthographicCameraConstructor().construct({-1, 1, 1, -1, .1, 10}); result.get("position").set("z", 3); return result; }
JSValue parameters(std::initializer_list<std::pair<const char*, JSValue>> fields) {
    auto result = JSValue::object(); for (const auto& [key, value] : fields) result.set(key, value); return result;
}
JSValue basic(unsigned color = 0xff0000) { return nativeMeshBasicMaterialConstructor().construct({parameters({{"color", color}})}); }
JSValue mesh(JSValue geometry, JSValue material, double x = 0) {
    auto result = nativeMeshConstructor().construct({geometry, material}); result.set("frustumCulled", false); result.get("position").set("x", x); return result;
}
JSValue scene(const Args& objects) { auto result = nativeSceneConstructor().construct(); call(result, "add", objects); return result; }
void expectPixel(const VulkanRenderer& renderer, std::array<int, 3> expected, const char* message, unsigned x = 32, unsigned y = 32) {
    const auto bytes = renderer.readPixels(); const auto offset = (y * renderer.getWidth() + x) * 4;
    require(bytes.size() == renderer.getWidth() * renderer.getHeight() * 4, "complete original-program RGBA readback");
    if (std::abs(int(bytes.at(offset)) - expected[0]) > 2 || std::abs(int(bytes.at(offset + 1)) - expected[1]) > 2 ||
        std::abs(int(bytes.at(offset + 2)) - expected[2]) > 2) {
        std::cerr << message << " actual=" << int(bytes.at(offset)) << ',' << int(bytes.at(offset + 1)) << ',' << int(bytes.at(offset + 2)) << '\n';
        require(false, message);
    }
    require(true, message);
}
void validate(const VulkanRenderer& renderer) {
    const auto messages = renderer.getValidationMessages(); for (const auto& message : messages) std::cerr << message << '\n';
    require(messages.empty(), "no collected managed program validation errors");
}
void sharedMaterialCache(bool validation) {
    VulkanRenderer renderer({64, 64, validation});
    const auto geometry = nativePlaneGeometryConstructor().construct({.7, 1.4});
    auto paint = basic(), left = mesh(geometry, paint, -.5), right = mesh(geometry, paint, .5);
    const auto view = camera(), world = scene({left, right});
    right.set("onBeforeRender", JSValue::function(+[](const JSValue&, const Args&, const Args& c) {
        call(c[0].get("color"), "setRGB", {0, 1, 0}); return JSValue();
    }, {paint}));
    renderer.render(world, view);
    expectPixel(renderer, {255, 0, 0}, "first shared material draw uploads red", 16);
    expectPixel(renderer, {255, 0, 0}, "consecutive shared material draw retains uploaded source uniforms", 48);
    require(paint.get("color").get("g").strictEquals(1), "callback changed the actual shared source material");
    renderer.render(world, view);
    expectPixel(renderer, {0, 255, 0}, "next frame refreshes actual shared material uniforms", 16);
    expectPixel(renderer, {0, 255, 0}, "next shared draw uses the refreshed program bytes", 48);
    require(renderer.info.get("render").get("calls").strictEquals(2), "actual source BufferRenderer owns draw-call statistics");
    validate(renderer);
}
void beforeCompileAndVersion(bool validation) {
    VulkanRenderer renderer({64, 64, validation});
    const auto geometry = nativePlaneGeometryConstructor().construct({1.6, 1.6});
    auto paint = basic(), state = parameters({{"key", "blue"}, {"blue", true}, {"compiles", 0}});
    paint.set("customProgramCacheKey", JSValue::function(+[](const JSValue&, const Args&, const Args& c) { return c[0].get("key"); }, {state}));
    paint.set("onBeforeCompile", JSValue::function(+[](const JSValue& receiver, const Args& args, const Args& c) {
        auto state = c[0]; require(receiver.sameValue(c[1]), "onBeforeCompile retains actual material receiver");
        state.set("compiles", state.get("compiles").number() + 1);
        auto shader = args.at(0);
        shader.set("fragmentShader", state.get("blue").toBoolean() ?
            "void main(){gl_FragColor=vec4(0.,0.,1.,1.);}" : "void main(){gl_FragColor=vec4(1.,0.,0.,1.);}");
        return JSValue();
    }, {state, paint}));
    const auto world = scene({mesh(geometry, paint)}), view = camera();
    renderer.render(world, view); expectPixel(renderer, {0, 0, 255}, "source onBeforeCompile shader executes on Vulkan");
    require(state.get("compiles").strictEquals(1), "one real shader compilation for the original key");
    state.set("blue", false); paint.set("needsUpdate", true);
    renderer.render(world, view); expectPixel(renderer, {0, 0, 255}, "same source cache key retains original shader after needsUpdate");
    require(state.get("compiles").strictEquals(1), "same key does not call onBeforeCompile again");
    state.set("key", "red"); paint.set("needsUpdate", true);
    renderer.render(world, view); expectPixel(renderer, {255, 0, 0}, "changed source key compiles and executes the changed GLSL");
    require(state.get("compiles").strictEquals(2), "different key performs actual source compilation once");
    call(paint, "dispose"); renderer.render(world, view);
    expectPixel(renderer, {255, 0, 0}, "disposed source program is recreated with the current shader");
    require(state.get("compiles").strictEquals(3), "source disposal retires its original program-cache entry");
    validate(renderer);
}
void shaderMaterialUniforms(bool validation) {
    VulkanRenderer renderer({64, 64, validation});
    auto shade = nativeVector4Constructor().construct({.25, .5, .75, 1});
    auto uniforms = parameters({{"shade", parameters({{"value", shade}})}});
    auto paint = nativeShaderMaterialConstructor().construct({parameters({{"uniforms", uniforms},
        {"vertexShader", "void main(){gl_Position=projectionMatrix*modelViewMatrix*vec4(position,1.);}"},
        {"fragmentShader", "uniform vec4 shade;void main(){gl_FragColor=shade;}"}})});
    const auto geometry = nativePlaneGeometryConstructor().construct({1.6, 1.6});
    const auto world = scene({mesh(geometry, paint)}), view = camera();
    renderer.render(world, view); expectPixel(renderer, {64, 128, 191}, "original ShaderMaterial executes custom shader and vec4 uniform");
    call(shade, "set", {0, 1, 0, 1});
    renderer.render(world, view); expectPixel(renderer, {0, 255, 0}, "original ShaderMaterial component mutation updates real uniform storage");
    const auto geometry2 = nativePlaneGeometryConstructor().construct({.7, 1.4});
    auto left = mesh(geometry2, paint, -.5), right = mesh(geometry2, paint, .5);
    auto second = scene({left, right}); call(shade, "set", {1, 0, 0, 1});
    right.set("onBeforeRender", JSValue::function(+[](const JSValue&, const Args&, const Args& c) {
        call(c[0], "set", {0, 0, 1, 1}); return JSValue();
    }, {shade}));
    renderer.render(second, view);
    expectPixel(renderer, {255, 0, 0}, "first ShaderMaterial draw uploads red", 16);
    expectPixel(renderer, {255, 0, 0}, "same ShaderMaterial cache retains red without uniformsNeedUpdate", 48);
    call(shade, "set", {1, 0, 0, 1});
    right.set("onBeforeRender", JSValue::function(+[](const JSValue&, const Args&, const Args& c) {
        call(c[0], "set", {0, 0, 1, 1}); auto material = c[1]; material.set("uniformsNeedUpdate", true); return JSValue();
    }, {shade, paint}));
    renderer.render(second, view);
    expectPixel(renderer, {255, 0, 0}, "earlier ShaderMaterial draw retains its own Vulkan uniform buffer", 16);
    expectPixel(renderer, {0, 0, 255}, "forced original uniform upload updates the later draw", 48);
    require(paint.get("uniformsNeedUpdate").strictEquals(false), "source clears uniformsNeedUpdate after upload");
    validate(renderer);
}
void sourceTextures(bool validation) {
    VulkanRenderer renderer({64, 64, validation});
    auto bytes = nativeGlobalBinding("Uint8Array").construct({list({255, 0, 0, 255, 0, 0, 255, 255})});
    auto map = nativeDataTextureConstructor().construct({bytes, 2, 1}); map.set("needsUpdate", true);
    auto paint = nativeMeshBasicMaterialConstructor().construct({parameters({{"color", 0xffffff}, {"map", map}})});
    auto geometry = nativePlaneGeometryConstructor().construct({1.6, 1.6});
    const auto world = scene({mesh(geometry, paint)}), view = camera();
    renderer.render(world, view); expectPixel(renderer, {255, 0, 0}, "source DataTexture map samples first actual GPU texel", 20);
    bytes.set("0", 0); bytes.set("1", 255);
    renderer.render(world, view); expectPixel(renderer, {255, 0, 0}, "texture CPU data mutation waits for original source needsUpdate", 20);
    call(map, "addUpdateRange", {0, 4}); const auto ranges = map.get("updateRanges"); map.set("needsUpdate", true);
    renderer.render(world, view); expectPixel(renderer, {0, 255, 0}, "source partial texture upload updates the actual GPU texel", 20);
    require(map.get("updateRanges").sameValue(ranges) && ranges.length() == 0, "original texture updateRanges array is cleared in place");
    call(map, "dispose"); map.set("needsUpdate", true); renderer.render(world, view);
    expectPixel(renderer, {0, 255, 0}, "source texture disposal recreates the actual image from original data", 20);
    validate(renderer);
}
}
int main(int argc, char** argv) {
    try {
        bool validation = false; for (int i = 1; i < argc; ++i) if (std::string(argv[i]) == "--validation") validation = true;
        sharedMaterialCache(validation); beforeCompileAndVersion(validation); shaderMaterialUniforms(validation); sourceTextures(validation);
        std::cout << "managed original-program Vulkan assertions=" << assertions << " validation_requested=" << validation << '\n';
        return 0;
    } catch (const std::exception& error) { std::cerr << "Managed original-program Vulkan failed: " << error.what() << '\n'; return 1; }
}
