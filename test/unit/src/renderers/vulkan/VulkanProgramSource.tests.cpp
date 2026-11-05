#include "catch.hpp"
#include "renderers/vulkan/VulkanProgramCommands.h"
#include "renderers/webgl/WebGLPrograms.h"
#include "renderers/webgl/WebGLUniforms.h"
#include "../../../utils/parity/WebGLProgramFixture.h"
#include <algorithm>
#include <cstring>

namespace {
using Args = std::vector<JSValue>;
JSValue invoke(JSValue object, const char* name, const Args& args = {}) {
    return object.get(name).call(object, args);
}
struct SourcePrograms {
    VulkanProgramCommands commands;
    JSValue gl = commands.object(), renderer = JSValue::object(), bindings = JSValue::object();
    JSValue counters = JSValue::object();
    WebGLPrograms cache;
    SourcePrograms() : cache(makeRenderer(), {}, {}, JSValue::object(), bindings, {}) {}
    JSValue makeRenderer() {
        counters.set("releases", 0);
        renderer.set("getContext", JSValue::function(
            [](const JSValue&, const Args&, const Args& c) { return c[0]; }, {gl}));
        auto debug = JSValue::object(); debug.set("checkShaderErrors", false);
        renderer.set("debug", debug);
        bindings.set("releaseStatesOfProgram", JSValue::function(
            [](const JSValue&, const Args&, const Args& c) {
                auto state = c[0]; state.set("releases", state.get("releases").number() + 1);
                return JSValue();
            }, {counters}));
        return renderer;
    }
    JSValue parameters(const char* fragment = nullptr) {
        auto p = ProgramFixture::defaults();
        p.set("shaderType", "RawShaderMaterial"); p.set("glslVersion", "300 es");
        p.set("hasPositionAttribute", true);
        p.set("vertexShader", "precision highp float;in vec3 position;uniform mat4 modelViewMatrix;void main(){gl_Position=modelViewMatrix*vec4(position,1.);}");
        p.set("fragmentShader", fragment ? fragment : "precision highp float;uniform vec3 diffuse;uniform float opacity;out vec4 result;void main(){result=vec4(diffuse,opacity);}");
        return p;
    }
    float scalar(const std::shared_ptr<VulkanProgramCommands::LinkedProgram>& program, const char* name) {
        const auto mapping = std::find_if(program->result.uniformMappings.begin(), program->result.uniformMappings.end(),
            [&](const auto& item) { return item.sourceName == name; });
        REQUIRE(mapping != program->result.uniformMappings.end());
        const auto bytes = program->uniforms->bytes({mapping->set, mapping->binding});
        REQUIRE(mapping->rootLayout.offset + sizeof(float) <= bytes.size());
        float result; std::memcpy(&result, bytes.data() + mapping->rootLayout.offset, sizeof(result));
        return result;
    }
};
}

TEST_CASE("Original WebGLPrograms caches actual natively compiled program identities and releases once", "[vulkan][program-source]") {
    SourcePrograms context;
    auto program = context.cache.acquireProgram(context.parameters(), "shared");
    const auto handle = program.get("program");
    const auto linked = context.commands.linked(handle);
    REQUIRE(linked != nullptr);
    REQUIRE_FALSE(linked->result.modules[0].words.empty());
    REQUIRE_FALSE(linked->result.modules[1].words.empty());
    REQUIRE(invoke(context.gl, "getProgramParameter", {handle, context.gl.get("LINK_STATUS")}).strictEquals(true));
    auto changed = context.parameters("precision highp float;out vec4 result;void main(){result=vec4(0.,1.,0.,1.);}");
    const auto shared = context.cache.acquireProgram(changed, "shared");
    REQUIRE(shared.sameValue(program));
    REQUIRE(program.get("usedTimes").strictEquals(2));
    REQUIRE(context.cache.object().get("programs").length() == 1);
    const auto different = context.cache.acquireProgram(changed, "different");
    REQUIRE_FALSE(different.sameValue(program));
    REQUIRE(context.commands.linked(different.get("program")) != linked);
    REQUIRE(context.cache.object().get("programs").length() == 2);
    context.cache.releaseProgram(program);
    REQUIRE(program.get("program").sameValue(handle));
    REQUIRE(context.counters.get("releases").strictEquals(0));
    context.cache.releaseProgram(shared);
    REQUIRE(program.get("program").isUndefined());
    REQUIRE(context.counters.get("releases").strictEquals(1));
    REQUIRE(context.commands.linked(handle) == nullptr);
    REQUIRE(context.cache.object().get("programs").get("0").sameValue(different));
    context.cache.releaseProgram(different);
    REQUIRE(context.cache.object().get("programs").length() == 0);
    REQUIRE(context.counters.get("releases").strictEquals(2));
    REQUIRE(invoke(context.gl, "getError").strictEquals(0));
}

TEST_CASE("Original WebGLProgram lazily reflects real source GLSL and shares the native uniform cache", "[vulkan][program-source]") {
    SourcePrograms context;
    auto program = context.cache.acquireProgram(context.parameters(), "uniforms");
    const auto handle = program.get("program");
    const auto linked = context.commands.linked(handle);
    auto uniforms = invoke(program, "getUniforms"), attributes = invoke(program, "getAttributes");
    REQUIRE(uniforms.sameValue(invoke(program, "getUniforms")));
    REQUIRE(attributes.sameValue(invoke(program, "getAttributes")));
    REQUIRE(attributes.get("position").get("location").strictEquals(0));
    REQUIRE(uniforms.get("map").hasOwn("diffuse"));
    REQUIRE(uniforms.get("map").hasOwn("opacity"));
    REQUIRE(invoke(context.gl, "getShaderParameter", {program.get("vertexShader"), context.gl.get("DELETE_STATUS")}).strictEquals(true));
    REQUIRE(invoke(context.gl, "getShaderSource", {program.get("vertexShader")}).toString().starts_with("#version 300 es\n"));
    invoke(context.gl, "useProgram", {handle});
    auto manager = WebGLUniforms::fromObject(uniforms);
    manager.setValue(context.gl, "opacity", .375);
    REQUIRE(context.scalar(linked, "opacity") == .375f);
    const auto revision = linked->uniformRevision;
    manager.setValue(context.gl, "opacity", .375);
    REQUIRE(linked->uniformRevision == revision);
    manager.setValue(context.gl, "opacity", .875);
    REQUIRE(context.scalar(linked, "opacity") == .875f);
    REQUIRE(linked->uniformRevision == revision + 1);
    context.cache.releaseProgram(program);
    REQUIRE(invoke(context.gl, "getError").strictEquals(0));
}
