#include "renderers/VulkanRenderer.h"
#include "core/BufferGeometry.h"
#include "core/BufferAttribute.h"
#include "core/NativeArray.h"
#include "scenes/Scene.h"
#include "cameras/OrthographicCamera.h"
#include "materials/MeshBasicMaterial.h"
#include "objects/Mesh.h"
#include "geometries/PlaneGeometry.h"
#include "textures/DataTexture.h"
#include <array>
#include <cmath>
#include <iostream>
#include <stdexcept>

namespace {
using Args = std::vector<JSValue>;
int assertions = 0, images = 0, validationRequested = 0, validationEnabled = 0, validationMessages = 0;
void require(bool value, const char* message) { ++assertions; if (!value) throw std::runtime_error(message); }
JSValue call(JSValue self, const char* name, const Args& args = {}) { return self.get(name).call(self, args); }
JSValue list(const Args& values = {}) {
    auto result = JSValue::array();
    for (const auto& value : values) result.set(JSValue(result.length()), value);
    return result;
}
void push(JSValue values, JSValue value) { values.set(JSValue(values.length()), value); }
struct Validation {
    VulkanRenderer& renderer;
    Validation(VulkanRenderer& value, bool requested) : renderer(value) {
        validationRequested += requested; validationEnabled += renderer.getInfo().validationEnabled;
        renderer.srgbOutput = false;
        auto facade = renderer.object(); facade.set("outputColorSpace", LinearSRGBColorSpace); facade.set("toneMapping", NoToneMapping);
    }
    void verify() {
        const auto messages = renderer.getValidationMessages(); validationMessages += int(messages.size());
        for (const auto& message : messages) std::cerr << "Vulkan validation: " << message << '\n';
        require(messages.empty(), "source program draws emit no collected Vulkan validation errors");
    }
};
JSValue camera(double halfWidth = 1) {
    auto value = nativeOrthographicCameraConstructor().construct({-halfWidth, halfWidth, 1, -1, .1, 10});
    value.get("position").set("z", 3); return value;
}
JSValue material(unsigned color = 0xffffff) {
    auto parameters = JSValue::object(); parameters.set("color", color);
    return nativeMeshBasicMaterialConstructor().construct({parameters});
}
JSValue triangle(double size = .25) {
    auto value = nativeBufferGeometryConstructor().construct();
    call(value, "setAttribute", {"position", nativeFloat32BufferAttributeConstructor().construct({list({-size, -size, 0, size, -size, 0, 0, size, 0}), 3})});
    return value;
}
JSValue mesh(JSValue shape, JSValue paint, double x = 0) {
    auto value = nativeMeshConstructor().construct({shape, paint});
    value.set("frustumCulled", false); value.get("position").set("x", x); return value;
}
struct World {
    JSValue scene = nativeSceneConstructor().construct(), view = camera();
    JSValue shape = nativePlaneGeometryConstructor().construct({1.6, 1.6}), paint = material(), object;
    World() { object = mesh(shape, paint); call(scene, "add", {object}); }
};
std::array<int, 4> pixel(const VulkanRenderer& renderer, unsigned x, unsigned y) {
    const auto bytes = renderer.readPixels();
    require(bytes.size() == renderer.getWidth() * renderer.getHeight() * 4, "complete actual Vulkan RGBA readback");
    const auto at = (y * renderer.getWidth() + x) * 4;
    return {bytes.at(at), bytes.at(at + 1), bytes.at(at + 2), bytes.at(at + 3)};
}
void expect(const VulkanRenderer& renderer, std::array<int, 3> expected, const char* message, unsigned x = 32, unsigned y = 32) {
    ++images; const auto actual = pixel(renderer, x, y);
    if (std::abs(actual[0] - expected[0]) > 2 || std::abs(actual[1] - expected[1]) > 2 || std::abs(actual[2] - expected[2]) > 2) {
        std::cerr << message << ": actual=" << actual[0] << ',' << actual[1] << ',' << actual[2] << '\n';
        require(false, message);
    }
    require(true, message);
}
JSValue programs(VulkanRenderer& renderer) {
    const auto values = renderer.object().get("info").get("programs");
    require(values.type()==JSValue::Type::Array, "public info.programs exposes the real source WebGLPrograms array"); return values;
}
JSValue dataTexture(std::initializer_list<int> bytes, unsigned width = 1, unsigned height = 1) {
    Uint8Array data(bytes.size()); unsigned index = 0;
    for (const auto value : bytes) data[index++] = value;
    const auto value = nativeDataTextureConstructor().construct({NativeArray_detail::wrap(data), width, height});
    require(value.get("minFilter").strictEquals(NearestFilter) && value.get("magFilter").strictEquals(NearestFilter), "canonical DataTexture nearest filtering defaults");
    require(value.get("generateMipmaps").strictEquals(false) && value.get("flipY").strictEquals(false), "canonical DataTexture upload defaults");
    auto result = value; result.set("needsUpdate", true); return result;
}
JSValue compileObserver(JSValue paint, JSValue renderer, JSValue state) {
    return JSValue::function(+[](const JSValue& self, const Args& args, const Args& captured) -> JSValue {
        require(self.sameValue(captured[0]), "onBeforeCompile receives the original source material receiver");
        require(args.size() == 2 && args[1].sameValue(captured[1]), "onBeforeCompile receives the original renderer argument");
        require(args[0].get("uniforms").isObject(), "onBeforeCompile receives the real source uniforms");
        auto state = captured[2]; state.set("calls", state.get("calls").toNumber() + 1);
        const auto rgb = state.get("rgb").toString();
        if (!rgb.empty()) {
            auto shader = args[0]; auto text = shader.get("fragmentShader").toString();
            const std::string marker = "#include <color_fragment>";
            const auto at = text.find(marker); require(at != std::string::npos, "Basic source fragment retains its color include");
            text.insert(at + marker.size(), "\ndiffuseColor.rgb = " + rgb + ";\n"); shader.set("fragmentShader", text);
        }
        return {};
    }, {paint, renderer, state});
}
JSValue compileState() { auto value = JSValue::object(); value.set("calls", 0); value.set("key", "first"); value.set("rgb", ""); return value; }
void observe(JSValue paint, JSValue renderer, JSValue state) {
    paint.set("onBeforeCompile", compileObserver(paint, renderer, state));
    paint.set("customProgramCacheKey", JSValue::function(+[](const JSValue&, const Args&, const Args& c) -> JSValue { return c[0].get("key"); }, {state}));
}

void sharedMaterialAndCamera(bool validation) {
    VulkanRenderer renderer({64, 64, validation}); Validation layer(renderer, validation);
    auto scene = nativeSceneConstructor().construct(), view = camera(), shape = triangle(), paint = material(0xff0000);
    auto left = mesh(shape, paint, -.5), right = mesh(shape, paint, .5);
    left.set("renderOrder", 0); right.set("renderOrder", 1); call(scene, "add", {left, right});
    const auto count = compileState(); observe(paint, renderer.object(), count);
    right.set("onBeforeRender", JSValue::function(+[](const JSValue&, const Args&, const Args& c) -> JSValue { call(c[0].get("color"), "setHex", {0x0000ff}); return {}; }, {paint}));
    renderer.render(scene, view);
    expect(renderer, {255, 0, 0}, "first shared-material draw captures its own model matrix", 16, 32);
    expect(renderer, {255, 0, 0}, "same material and camera retain the first source diffuse upload", 48, 32);
    require(paint.get("color").get("b").strictEquals(1), "second object callback changed the actual shared material");
    require(count.get("calls").strictEquals(1), "consecutive shared-material objects compile one source program");
    const auto values = programs(renderer), original = values.get("0"); require(values.length() == 1, "one actual cached shared Basic program");
    right.set("onBeforeRender", JSValue::function(+[](const JSValue&, const Args&, const Args&) -> JSValue { return {}; }));
    renderer.render(scene, view);
    expect(renderer, {0, 0, 255}, "next source frame refreshes the changed shared material", 16, 32);
    expect(renderer, {0, 0, 255}, "second shared object uses the same refreshed program", 48, 32);
    auto wide = camera(2); renderer.render(scene, wide);
    expect(renderer, {0, 0, 255}, "a different camera uploads its projection for the first draw", 24, 32);
    expect(renderer, {0, 0, 255}, "second draw preserves the new camera projection", 40, 32);
    expect(renderer, {0, 0, 0}, "previous camera coordinates are no longer drawn", 16, 32);
    require(programs(renderer).get("0").sameValue(original) && count.get("calls").strictEquals(1), "camera switches reuse the original native program identity");
    layer.verify();
}

void programKeysCompileAndDispose(bool validation) {
    VulkanRenderer renderer({64, 64, validation}); Validation layer(renderer, validation); World world;
    call(world.paint.get("color"), "setHex", {0xff0000}); const auto state = compileState(); observe(world.paint, renderer.object(), state);
    renderer.render(world.scene, world.view); expect(renderer, {255, 0, 0}, "canonical Basic source compiles through the production render entry");
    const auto original = programs(renderer).get("0");
    call(world.paint.get("color"), "setHex", {0x0000ff}); world.paint.set("needsUpdate", true);
    renderer.render(world.scene, world.view); expect(renderer, {0, 0, 255}, "same-key material version refreshes uniforms without rebuilding its source program");
    require(state.get("calls").strictEquals(1) && programs(renderer).get("0").sameValue(original), "same key needsUpdate hits the source material program cache");
    auto mutableState = state; mutableState.set("key", "second"); mutableState.set("rgb", "vec3(0.0,1.0,0.0)");
    renderer.render(world.scene, world.view); expect(renderer, {0, 0, 255}, "a cache key change alone waits for the source program-change trigger");
    require(state.get("calls").strictEquals(1), "customProgramCacheKey does not bypass material version gating");
    world.paint.set("needsUpdate", true); renderer.render(world.scene, world.view);
    expect(renderer, {0, 255, 0}, "onBeforeCompile changes the actual compiled fragment shader pixels");
    require(state.get("calls").strictEquals(2) && programs(renderer).length() == 2, "new source key creates one additional real program");
    const auto second = programs(renderer).get("1"); require(!second.sameValue(original), "different source program keys retain independent identities");
    mutableState.set("key", "first"); mutableState.set("rgb", "vec3(1.0,0.0,1.0)"); world.paint.set("needsUpdate", true);
    renderer.render(world.scene, world.view); expect(renderer, {0, 0, 255}, "returning to a cached key restores its original compiled shader");
    require(state.get("calls").strictEquals(2), "reusing a source key skips onBeforeCompile");
    call(world.paint, "dispose"); require(programs(renderer).length() == 0, "source material disposal releases every retained program key");
    require(original.get("program").isUndefined() && second.get("program").isUndefined(), "disposed source program wrappers release their native command handles");
    renderer.render(world.scene, world.view); expect(renderer, {255, 0, 255}, "rendering a disposed material rebuilds through its current source callback");
    require(state.get("calls").strictEquals(3) && programs(renderer).length() == 1, "disposed material programs are recreated once");
    require(!programs(renderer).get("0").sameValue(original), "recreated source program has a fresh identity"); layer.verify();
}

void mapVersionsAndRanges(bool validation) {
    VulkanRenderer renderer({64, 64, validation}); Validation layer(renderer, validation); World world;
    auto texture = dataTexture({255, 0, 0, 255, 0, 0, 255, 255}, 2); world.paint.set("map", texture);
    const auto data = texture.get("image").get("data"), source = texture.get("source"), ranges = texture.get("updateRanges");
    auto events = list(); texture.set("onUpdate", JSValue::function(+[](const JSValue& self, const Args& args, const Args& c) -> JSValue {
        require(self.sameValue(c[1]) && args.size() == 1 && args[0].sameValue(self), "real texture onUpdate preserves receiver and original argument");
        require(self.get("source").sameValue(c[2]) && self.get("image").get("data").sameValue(c[3]), "texture upload preserves original source and typed data identities");
        push(c[0], self.get("version")); return {};
    }, {events, texture, source, data}));
    renderer.render(world.scene, world.view); expect(renderer, {255, 0, 0}, "Basic map samples the first actual RGBA8 texel", 16, 32); expect(renderer, {0, 0, 255}, "Basic map samples the second actual RGBA8 texel", 48, 32);
    const auto uploads = renderer.getInfo().textureUploads;
    auto mutableData = data; mutableData.set("0", 0); mutableData.set("1", 255);
    renderer.render(world.scene, world.view); expect(renderer, {255, 0, 0}, "unversioned typed texture writes retain uploaded bytes", 16, 32);
    require(renderer.getInfo().textureUploads == uploads && events.length() == 1, "unchanged texture version skips actual GPU upload and callback");
    call(texture, "addUpdateRange", {0, 4}); texture.set("needsUpdate", true); renderer.render(world.scene, world.view);
    expect(renderer, {0, 255, 0}, "needsUpdate uploads the selected first texel", 16, 32); expect(renderer, {0, 0, 255}, "partial texture upload preserves the other GPU texel", 48, 32);
    require(texture.get("updateRanges").sameValue(ranges) && ranges.length() == 0, "source upload clears the original texture ranges array");
    require(renderer.getInfo().textureUploads == uploads + 1 && events.length() == 2, "one committed range upload produces one actual upload and callback");
    mutableData.set("1", 0); mutableData.set("2", 255); mutableData.set("4", 255); mutableData.set("5", 255); mutableData.set("6", 0);
    call(texture, "addUpdateRange", {4, 4}); texture.set("needsUpdate", true); renderer.render(world.scene, world.view);
    expect(renderer, {0, 255, 0}, "a CPU edit outside updateRanges stays absent from GPU storage", 16, 32); expect(renderer, {255, 255, 0}, "the second range reaches the actual sampled texture", 48, 32);
    require(events.length() == 3 && renderer.getInfo().textureUploads == uploads + 2, "subsequent texture version commits one further GPU update"); layer.verify();
}

void alphaTexture(bool validation) {
    VulkanRenderer renderer({64, 64, validation}); Validation layer(renderer, validation); World world;
    call(world.paint.get("color"), "setHex", {0x00ff00}); auto alpha = dataTexture({255, 0, 255, 255});
    world.paint.set("alphaMap", alpha); world.paint.set("alphaTest", .5);
    renderer.render(world.scene, world.view); expect(renderer, {0, 0, 0}, "source Basic alphaMap uses its green component for alpha testing");
    auto bytes = alpha.get("image").get("data"); bytes.set("1", 255);
    renderer.render(world.scene, world.view); expect(renderer, {0, 0, 0}, "alpha map bytes require a version update before affecting fragments");
    call(alpha, "addUpdateRange", {0, 4}); alpha.set("needsUpdate", true); renderer.render(world.scene, world.view);
    expect(renderer, {0, 255, 0}, "actual versioned alpha map permits visible Basic fragments");
    bytes.set("1", 0); alpha.set("needsUpdate", true); renderer.render(world.scene, world.view);
    expect(renderer, {0, 0, 0}, "another real alpha upload discards those fragments again"); layer.verify();
}

void sharedTextureSourceAndDisposal(bool validation) {
    VulkanRenderer renderer({64, 64, validation}); Validation layer(renderer, validation);
    auto scene = nativeSceneConstructor().construct(), view = camera(), shape = nativePlaneGeometryConstructor().construct({.6, .6});
    auto first = dataTexture({255, 0, 0, 255}), second = call(first, "clone");
    require(second.get("source").sameValue(first.get("source")), "canonical texture clone retains its original source identity");
    first.set("uuid", "source-texture-collision"); second.set("uuid", "source-texture-collision");
    auto a = material(), b = material(); a.set("map", first); b.set("map", second);
    auto left = mesh(shape, a, -.5), right = mesh(shape, b, .5); call(scene, "add", {left});
    renderer.render(scene, view); expect(renderer, {255, 0, 0}, "first source texture reaches the production GPU path", 16, 32);
    const auto memory = renderer.object().get("info").get("memory"); const auto textures = memory.get("textures").toNumber();
    const auto uploads = renderer.getInfo().textureUploads, allocations = renderer.getInfo().textureImages;
    call(scene, "add", {right}); renderer.render(scene, view);
    expect(renderer, {255, 0, 0}, "texture source sharing preserves the first material draw", 16, 32); expect(renderer, {255, 0, 0}, "a different texture wrapper samples the shared native image", 48, 32);
    require(memory.get("textures").toNumber() == textures && renderer.getInfo().textureImages == allocations, "equal source and texture cache parameters reuse one actual image");
    require(renderer.getInfo().textureUploads == uploads, "shared source version avoids a second GPU texture upload");
    left.set("visible", false); call(first, "dispose");
    require(memory.get("textures").toNumber() == textures, "disposing one wrapper retains another live source reference");
    renderer.render(scene, view); expect(renderer, {255, 0, 0}, "remaining source reference survives the first texture disposal", 48, 32);
    call(second, "dispose"); require(memory.get("textures").toNumber() == textures - 1, "last source reference releases the original texture cache entry");
    renderer.render(scene, view); expect(renderer, {255, 0, 0}, "disposed source wrapper reinitializes its actual GPU texture on reuse", 48, 32);
    require(renderer.getInfo().textureUploads > uploads && memory.get("textures").toNumber() == textures, "source texture reuse allocates and uploads again"); layer.verify();
}

void textureUploadReentry(bool validation) {
    VulkanRenderer renderer({64, 64, validation}); Validation layer(renderer, validation); World outer, inner;
    auto facade = renderer.object(); facade.set("autoClear", false); renderer.autoClear = false; renderer.clear();
    call(inner.paint.get("color"), "setHex", {0x0000ff}); auto texture = dataTexture({255, 0, 0, 255}); outer.paint.set("map", texture);
    auto events = list(); texture.set("onUpdate", JSValue::function(+[](const JSValue& self, const Args& args, const Args& c) -> JSValue {
        require(self.sameValue(c[1]) && args.size() == 1 && args[0].sameValue(self), "nested texture upload retains the original callback identity");
        push(c[0], "texture.before"); call(c[2], "render", {c[3], c[4]}); push(c[0], "texture.after"); return {};
    }, {events, texture, facade, inner.scene, inner.view}));
    renderer.render(outer.scene, outer.view);
    require(events.length() == 2 && events.get("0").strictEquals("texture.before") && events.get("1").strictEquals("texture.after"), "onUpdate completes around one real nested source render");
    expect(renderer, {0, 0, 255}, "outer draw uses the actual current native program left by upload reentry");
    // The interrupted source uniform setter caches a value even when GL rejects
    // its old program location. Change the matrix to request a valid new upload.
    outer.object.get("position").set("x", .1); renderer.clear(); renderer.render(outer.scene, outer.view);
    expect(renderer, {255, 0, 0}, "a later source frame binds the outer program and uploads its changed matrix");
    require(events.length() == 2, "unchanged source texture version does not repeat upload reentry"); layer.verify();
}

void nativeCommandConversionsAndDepth(bool validation) {
    VulkanRenderer renderer({64,64,validation});Validation layer(renderer,validation);World world;
    auto facade=renderer.object();auto gl=call(facade,"getContext");renderer.render(world.scene,world.view);
    require(call(gl,"getError").strictEquals(0),"ordinary source draw leaves no native API error");
    call(gl,"drawElements",{4,0,0x1401,-1});
    require(call(gl,"getError").strictEquals(0x0501),"negative GLintptr remains negative and produces INVALID_VALUE");
    call(gl,"drawElements",{4,0,0x1401,1.75});
    require(call(gl,"getError").strictEquals(0),"fractional byte offset truncates through native GLintptr conversion");
    call(gl,"drawElements",{4,0,0x1403,3.75});
    require(call(gl,"getError").strictEquals(0x0502),"index alignment validates the converted native byte offset");
    call(gl,"drawElements",{4,0,0x1401,-18446744073709551616.0});
    require(call(gl,"getError").strictEquals(0),"native GLintptr conversion wraps multiples of two to the sixty four");
    require(call(gl,"getParameter",{0x0d57}).strictEquals(0),"actual native framebuffer has no stencil attachment");
    facade.set("autoClear",false);call(gl,"clearDepth",{0});call(facade,"clear",{true,true,false});
    renderer.render(world.scene,world.view);expect(renderer,{0,0,0},"native clearDepth zero participates in real Vulkan depth testing");
    call(gl,"clearDepth",{1});call(facade,"clear",{true,true,false});renderer.render(world.scene,world.view);
    expect(renderer,{255,255,255},"restoring native clearDepth one permits the original source draw");layer.verify();
}
}

int main(int argc, char** argv) {
    try {
        bool validation = false; for (int i = 1; i < argc; ++i) if (std::string(argv[i]) == "--validation") validation = true;
        sharedMaterialAndCamera(validation); programKeysCompileAndDispose(validation); mapVersionsAndRanges(validation);
        alphaTexture(validation); sharedTextureSourceAndDisposal(validation); textureUploadReentry(validation);nativeCommandConversionsAndDepth(validation);
        std::cout << "source program draw assertions=" << assertions << " images=" << images
                  << " validation_requested=" << validationRequested << " validation_enabled=" << validationEnabled
                  << " validation_messages=" << validationMessages << '\n';
        if (validation && validationEnabled == 0) std::cout << "Khronos validation layer unavailable; GPU rendering checks ran without layer validation.\n";
        return 0;
    } catch (const std::exception& error) { std::cerr << "Source program Vulkan draw failed: " << error.what() << '\n'; return 1; }
}
