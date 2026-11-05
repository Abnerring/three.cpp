#include "catch.hpp"
#include "renderers/vulkan/VulkanProgramCommands.h"
#include "renderers/webgl/WebGLUniforms.h"
#include "core/NativeArray.h"
#include "math/Color.h"
#include <algorithm>
#include <cstring>
#include <limits>

namespace {
using Args = std::vector<JSValue>;
JSValue call(JSValue gl, const char* name, const Args& args = {}) { return gl.get(name).call(gl, args); }
JSValue list(std::initializer_list<JSValue> values) {
    auto array = JSValue::array(); unsigned index = 0;
    for (const auto& value : values) array.set(JSValue(index++), value);
    return array;
}
const char* uniformFragment = R"(#version 300 es
precision highp float;precision highp int;
uniform vec3 diffuse;uniform float opacity;
uniform ivec2 pair;uniform uvec2 mask;uniform bvec2 switches;
uniform mat2 square;uniform mat2x3 rectangular;uniform mat2 transforms[2];
uniform vec3 colors[3];uniform float weights[3];uniform float single[1];
uniform sampler2D maps[3];
struct Light { vec3 color;float factors[2]; };uniform Light lights[2];
out vec4 color;
void main(){
vec3 v=diffuse+colors[0]+colors[1]+colors[2]+lights[0].color+lights[1].color;
float n=opacity+weights[0]+weights[1]+weights[2]+single[0]
+lights[0].factors[0]+lights[0].factors[1]+lights[1].factors[0]+lights[1].factors[1]
+float(pair.x+pair.y)+float(mask.x+mask.y)+(switches.x?1.:0.)+(switches.y?1.:0.)
+square[0][0]+square[1][1]+rectangular[0][0]+rectangular[1][2]+transforms[0][0][0]+transforms[1][1][1];
color=vec4(v+vec3(n)+texture(maps[0],vec2(0.)).rgb
+texture(maps[1],vec2(0.)).rgb+texture(maps[2],vec2(0.)).rgb,1.);
})";
struct ProgramFixture {
    VulkanProgramCommands commands;
    JSValue gl = commands.object(), program, vertex, fragment;
    explicit ProgramFixture(const char* fragmentSource = "#version 300 es\nprecision highp float;precision highp int;uniform vec3 diffuse;uniform float opacity;uniform int flags;uniform sampler2D map;out vec4 color;void main(){color=vec4(diffuse,opacity)+texture(map,vec2(0.))*float(flags);}") {
        vertex = call(gl, "createShader", {gl.get("VERTEX_SHADER")});
        fragment = call(gl, "createShader", {gl.get("FRAGMENT_SHADER")});
        call(gl, "shaderSource", {vertex, "#version 300 es\nprecision highp float;in vec3 position;uniform mat4 modelViewMatrix;void main(){gl_Position=modelViewMatrix*vec4(position+float(gl_VertexID)*0.001,1.);}"});
        call(gl, "shaderSource", {fragment, fragmentSource});
        call(gl, "compileShader", {vertex}); call(gl, "compileShader", {fragment});
        REQUIRE(call(gl, "getShaderParameter", {vertex, gl.get("COMPILE_STATUS")}).strictEquals(true));
        REQUIRE(call(gl, "getShaderParameter", {fragment, gl.get("COMPILE_STATUS")}).strictEquals(true));
        program = call(gl, "createProgram"); call(gl, "attachShader", {program, vertex}); call(gl, "attachShader", {program, fragment});
        call(gl, "bindAttribLocation", {program, 3, "position"}); call(gl, "linkProgram", {program});
        INFO(call(gl, "getProgramInfoLog", {program}).toString());
        REQUIRE(call(gl, "getProgramParameter", {program, gl.get("LINK_STATUS")}).strictEquals(true));
        call(gl, "useProgram", {program});
    }
    std::vector<float> floats(const char* name, unsigned count) {
        const auto linked = commands.linked(program);
        const auto mapping = std::find_if(linked->result.uniformMappings.begin(), linked->result.uniformMappings.end(),
            [&](const auto& value) { return value.sourceName == name; });
        REQUIRE(mapping != linked->result.uniformMappings.end());
        const auto bytes = linked->uniforms->bytes({mapping->set, mapping->binding});
        REQUIRE(mapping->rootLayout.offset + count * 4 <= bytes.size());
        std::vector<float> result(count); std::memcpy(result.data(), bytes.data() + mapping->rootLayout.offset, count * 4); return result;
    }
    JSValue location(const char* name) { return call(gl, "getUniformLocation", {program, name}); }
    JSValue additionalProgram() {
        const auto result = call(gl, "createProgram");
        call(gl, "attachShader", {result, vertex}); call(gl, "attachShader", {result, fragment});
        call(gl, "linkProgram", {result});
        INFO(call(gl, "getProgramInfoLog", {result}).toString());
        REQUIRE(call(gl, "getProgramParameter", {result, gl.get("LINK_STATUS")}).strictEquals(true));
        return result;
    }
    const VulkanShaderInterface::Block& layout(const char* root) {
        const auto linked = commands.linked(program);
        const auto mapping = std::find_if(linked->result.uniformMappings.begin(), linked->result.uniformMappings.end(),
            [&](const auto& value) { return value.rootName == root; });
        REQUIRE(mapping != linked->result.uniformMappings.end());
        return mapping->rootLayout;
    }
    template<class T> T at(std::size_t offset) {
        const auto bytes = commands.linked(program)->uniforms->bytes({0, 0});
        REQUIRE(offset + sizeof(T) <= bytes.size());
        T value; std::memcpy(&value, bytes.data() + offset, sizeof(T)); return value;
    }
    std::vector<std::byte> snapshot() {
        std::vector<std::byte> result;
        for (const auto& [key, block] : commands.linked(program)->uniforms->blocks()) {
            const auto bytes = block.storage.bytes(); result.insert(result.end(), bytes.begin(), bytes.end());
        }
        return result;
    }
    void rejects(const char* method, Args args, const char* code = "INVALID_OPERATION") {
        const auto bytes = snapshot();
        const auto data = commands.linked(program);
        const auto revision = data->uniformRevision;
        const auto samplers = data->uniforms->samplers();
        call(gl, method, args);
        INFO(method);
        REQUIRE(call(gl, "getError").strictEquals(gl.get(code)));
        REQUIRE(snapshot() == bytes);
        REQUIRE(data->uniformRevision == revision);
        for (const auto& [name, sampler] : samplers) REQUIRE(data->uniforms->samplers().at(name).units == sampler.units);
    }
};
}

TEST_CASE("Native program commands compile the unchanged linked ESSL and expose real active reflection", "[vulkan][program-commands]") {
    ProgramFixture fixture; auto gl = fixture.gl;
    REQUIRE(fixture.commands.current() == fixture.commands.linked(fixture.program));
    REQUIRE(fixture.commands.currentProgram().sameValue(fixture.program));
    REQUIRE(call(gl, "getProgramParameter", {fixture.program, gl.get("ACTIVE_ATTRIBUTES")}).strictEquals(1));
    REQUIRE(call(gl, "getActiveAttrib", {fixture.program, 0}).get("name").strictEquals("position"));
    REQUIRE(call(gl, "getAttribLocation", {fixture.program, "position"}).strictEquals(3));
    REQUIRE(call(gl, "getAttribLocation", {fixture.program, "gl_VertexID"}).strictEquals(-1));
    REQUIRE(call(gl, "getUniformLocation", {fixture.program, "inactive"}).isNull());
    REQUIRE(call(gl, "getError").strictEquals(0));
}

TEST_CASE("Original WebGLUniforms upload controls the program byte cache", "[vulkan][program-commands]") {
    ProgramFixture fixture; WebGLUniforms uniforms(fixture.gl, fixture.program);
    const auto color = nativeColorConstructor().construct({.25, .5, .75});
    uniforms.setValue(fixture.gl, "diffuse", color); uniforms.setValue(fixture.gl, "opacity", .5);
    const auto data = fixture.commands.current(); const auto revision = data->uniformRevision;
    REQUIRE(fixture.floats("diffuse", 3) == std::vector<float>{.25f, .5f, .75f});
    REQUIRE(fixture.floats("opacity", 1) == std::vector<float>{.5f});
    uniforms.setValue(fixture.gl, "diffuse", color); uniforms.setValue(fixture.gl, "opacity", .5);
    REQUIRE(data->uniformRevision == revision);
    auto mutableColor = color; mutableColor.set("g", .125); uniforms.setValue(fixture.gl, "diffuse", color);
    REQUIRE(data->uniformRevision == revision + 1);
    REQUIRE(fixture.floats("diffuse", 3) == std::vector<float>{.25f, .125f, .75f});
    REQUIRE(call(fixture.gl, "getError").strictEquals(0));
}

TEST_CASE("Program generations invalidate old uniform locations while deleted shaders stay attached", "[vulkan][program-commands]") {
    ProgramFixture fixture; auto gl = fixture.gl;
    const auto old = call(gl, "getUniformLocation", {fixture.program, "opacity"});
    call(gl, "deleteShader", {fixture.vertex}); call(gl, "deleteShader", {fixture.fragment});
    REQUIRE(call(gl, "getProgramParameter", {fixture.program, gl.get("ATTACHED_SHADERS")}).strictEquals(2));
    call(gl, "linkProgram", {fixture.program});
    REQUIRE(call(gl, "getProgramParameter", {fixture.program, gl.get("LINK_STATUS")}).strictEquals(true));
    call(gl, "uniform1f", {old, .75}); REQUIRE(call(gl, "getError").strictEquals(gl.get("INVALID_OPERATION")));
    const auto current = call(gl, "getUniformLocation", {fixture.program, "opacity"});
    call(gl, "uniform1f", {current, .25}); REQUIRE(fixture.floats("opacity", 1) == std::vector<float>{.25f});
    call(gl, "useProgram", {nullptr}); REQUIRE(fixture.commands.currentProgram().isNull());
    REQUIRE(call(gl, "getError").strictEquals(0));
}

TEST_CASE("Native shader compile failure retains diagnostics and foreign programs cannot become current", "[vulkan][program-commands]") {
    ProgramFixture fixture; auto gl = fixture.gl;
    const auto shader = call(gl, "createShader", {gl.get("VERTEX_SHADER")});
    call(gl, "shaderSource", {shader, "#version 300 es\nvoid main(){missing=1.;}"}); call(gl, "compileShader", {shader});
    REQUIRE(call(gl, "getShaderParameter", {shader, gl.get("COMPILE_STATUS")}).strictEquals(false));
    REQUIRE_FALSE(call(gl, "getShaderInfoLog", {shader}).toString().empty());
    VulkanProgramCommands other; const auto foreign = call(other.object(), "createProgram");
    call(gl, "useProgram", {foreign}); REQUIRE(call(gl, "getError").strictEquals(gl.get("INVALID_OPERATION")));
    REQUIRE(fixture.commands.currentProgram().sameValue(fixture.program));
    fixture.commands.shutdown(); REQUIRE_THROWS_AS(call(gl, "createProgram"), JSTypeError);
}

TEST_CASE("Uniform commands validate exact scalar vector and matrix types before changing storage", "[vulkan][program-commands]") {
    ProgramFixture fixture(uniformFragment);
    const auto diffuse = fixture.location("diffuse"), square = fixture.location("square");
    const auto rectangular = fixture.location("rectangular"), maps = fixture.location("maps");
    REQUIRE_FALSE(diffuse.isNull()); REQUIRE_FALSE(square.isNull()); REQUIRE_FALSE(rectangular.isNull()); REQUIRE_FALSE(maps.isNull());
    call(fixture.gl, "uniform3f", {diffuse, .25, .5, .75});
    fixture.rejects("uniform1fv", {diffuse, list({1, 2, 3})});
    fixture.rejects("uniform3iv", {diffuse, list({1, 2, 3})});
    fixture.rejects("uniform4fv", {square, list({1, 2, 3, 4})});
    fixture.rejects("uniformMatrix3x2fv", {rectangular, false, list({1, 2, 3, 4, 5, 6})});
    fixture.rejects("uniform2i", {fixture.location("mask"), 1, 2});
    fixture.rejects("uniform2ui", {fixture.location("pair"), 1, 2});
    fixture.rejects("uniform2f", {fixture.location("pair"), 1, 2});
    fixture.rejects("uniform1f", {maps, 1});
    fixture.rejects("uniform1ui", {maps, 1});
    fixture.rejects("uniform3iv", {maps, list({1, 2, 3})});
    fixture.rejects("uniformMatrix2fv", {square, true, list({1, 2, 3, 4})}, "INVALID_VALUE");
    REQUIRE(fixture.floats("diffuse", 3) == std::vector<float>{.25f, .5f, .75f});
    call(fixture.gl, "uniformMatrix2x3fv", {rectangular, false, list({1, 2, 3, 4, 5, 6})});
    REQUIRE(call(fixture.gl, "getError").strictEquals(0));
    const auto& matrix = fixture.layout("rectangular");
    for (unsigned c = 0; c < 2; ++c) for (unsigned r = 0; r < 3; ++r)
        REQUIRE(fixture.at<float>(matrix.offset + c * matrix.numeric.matrixStride + r * 4) == float(c * 3 + r + 1));
}

TEST_CASE("Boolean uniforms accept each numeric setter family with conversion before truth testing", "[vulkan][program-commands]") {
    ProgramFixture fixture(uniformFragment);
    const auto location = fixture.location("switches"); REQUIRE_FALSE(location.isNull());
    const auto offset = fixture.layout("switches").offset;
    const auto check = [&](std::uint32_t x, std::uint32_t y) {
        REQUIRE(fixture.at<std::uint32_t>(offset) == x);
        REQUIRE(fixture.at<std::uint32_t>(offset + 4) == y);
        REQUIRE(call(fixture.gl, "getError").strictEquals(0));
    };
    call(fixture.gl, "uniform2f", {location, .5, -0.}); check(1, 0);
    call(fixture.gl, "uniform2fv", {location, list({1e-50, std::numeric_limits<double>::quiet_NaN()})}); check(0, 1);
    call(fixture.gl, "uniform2i", {location, 4294967296., -2}); check(0, 1);
    call(fixture.gl, "uniform2iv", {location, list({7, 0})}); check(1, 0);
    call(fixture.gl, "uniform2ui", {location, 0, -1}); check(0, 1);
    call(fixture.gl, "uniform2uiv", {location, list({3, 0})}); check(1, 0);
    fixture.rejects("uniform1fv", {location, list({1, 0})});
    call(fixture.gl, "uniform2i", {fixture.location("pair"), -7, 11});
    call(fixture.gl, "uniform2ui", {fixture.location("mask"), 2147483648., 4294967295.});
    REQUIRE(fixture.at<std::int32_t>(fixture.layout("pair").offset) == -7);
    REQUIRE(fixture.at<std::uint32_t>(fixture.layout("mask").offset) == 0x80000000u);
    REQUIRE(call(fixture.gl, "getError").strictEquals(0));
}

TEST_CASE("Later uniform array locations preserve strides and ignore values after the active end", "[vulkan][program-commands]") {
    ProgramFixture fixture(uniformFragment);
    const auto first = fixture.location("colors"), zero = fixture.location("colors[0]"), later = fixture.location("colors[1]");
    REQUIRE_FALSE(first.isNull()); REQUIRE_FALSE(zero.isNull()); REQUIRE_FALSE(later.isNull());
    REQUIRE_FALSE(first.sameValue(zero));
    for (const auto* name : {"colors[-1]", "colors[3]", "colors[1].x", "colors[1][0]", "colors[]"}) REQUIRE(fixture.location(name).isNull());
    call(fixture.gl, "uniform3fv", {first, list({1, 2, 3, 4, 5, 6, 7, 8, 9})});
    call(fixture.gl, "uniform3fv", {later, list({10, 20, 30, 40, 50, 60, 70, 80, 90})});
    const auto& colors = fixture.layout("colors");
    for (unsigned c = 0; c < 3; ++c) {
        REQUIRE(fixture.at<float>(colors.offset + c * 4) == float(c + 1));
        REQUIRE(fixture.at<float>(colors.offset + colors.array.strides[0] + c * 4) == float((c + 1) * 10));
        REQUIRE(fixture.at<float>(colors.offset + 2 * colors.array.strides[0] + c * 4) == float((c + 4) * 10));
    }
    call(fixture.gl, "uniform1fv", {fixture.location("single[0]"), list({.25, .5})});
    REQUIRE(fixture.at<float>(fixture.layout("single").offset) == .25f);
    call(fixture.gl, "uniform1f", {fixture.location("weights[1]"), 7});
    const auto& weights = fixture.layout("weights");
    REQUIRE(fixture.at<float>(weights.offset + weights.array.strides[0]) == 7);
    call(fixture.gl, "uniformMatrix2fv", {fixture.location("transforms[1]"), false, list({1, 2, 3, 4, 5, 6, 7, 8})});
    const auto& transforms = fixture.layout("transforms");
    for (unsigned c = 0; c < 2; ++c) for (unsigned r = 0; r < 2; ++r)
        REQUIRE(fixture.at<float>(transforms.offset + transforms.array.strides[0] + c * transforms.numeric.matrixStride + r * 4) == float(c * 2 + r + 1));
    fixture.rejects("uniform1fv", {fixture.location("opacity"), list({.25, .5})});
    call(fixture.gl, "uniform1iv", {fixture.location("maps[1]"), list({2, 5, -1})});
    REQUIRE(fixture.commands.current()->uniforms->samplers().at("maps").units == std::vector<std::int32_t>{0, 2, 5});
    REQUIRE(call(fixture.gl, "getError").strictEquals(0));
}

TEST_CASE("Structured uniform array paths retain their fixed enclosing object index", "[vulkan][program-commands]") {
    ProgramFixture fixture(uniformFragment);
    const auto first = fixture.location("lights[0].color"), second = fixture.location("lights[1].color");
    const auto factor = fixture.location("lights[1].factors[1]");
    REQUIRE_FALSE(first.isNull()); REQUIRE_FALSE(second.isNull()); REQUIRE_FALSE(factor.isNull());
    REQUIRE(fixture.location("lights[2].color").isNull());
    REQUIRE(fixture.location("lights[0].color[0]").isNull());
    fixture.rejects("uniform3fv", {first, list({1, 2, 3, 4, 5, 6})});
    call(fixture.gl, "uniform3f", {first, 1, 2, 3});
    call(fixture.gl, "uniform3f", {second, 4, 5, 6});
    call(fixture.gl, "uniform1fv", {factor, list({.75, .5})});
    const auto& lights = fixture.layout("lights");
    const auto color = std::find_if(lights.members.begin(), lights.members.end(), [](const auto& m) { return m.name == "color"; });
    const auto factors = std::find_if(lights.members.begin(), lights.members.end(), [](const auto& m) { return m.name == "factors"; });
    REQUIRE(color != lights.members.end()); REQUIRE(factors != lights.members.end());
    REQUIRE(fixture.at<float>(lights.offset + color->offset) == 1);
    REQUIRE(fixture.at<float>(lights.offset + lights.array.strides[0] + color->offset) == 4);
    REQUIRE(fixture.at<float>(lights.offset + lights.array.strides[0] + factors->offset + factors->array.strides[0]) == .75f);
    REQUIRE(call(fixture.gl, "getError").strictEquals(0));
}

TEST_CASE("WebGLUniforms sees normalized basic array reflection and list range validation stays atomic", "[vulkan][program-commands]") {
    ProgramFixture fixture(uniformFragment); WebGLUniforms uniforms(fixture.gl, fixture.program);
    const auto values = list({1, 2, 3, 4, 5, 6, 7, 8, 9});
    uniforms.setValue(fixture.gl, "colors", values);
    const auto& colors = fixture.layout("colors");
    REQUIRE(fixture.at<float>(colors.offset + 2 * colors.array.strides[0] + 8) == 9);
    const auto location = fixture.location("colors[1]");
    call(fixture.gl, "uniform3fv", {location, list({-1, 10, 20, 30, 40, 50, 60, -1}), 1, 6});
    REQUIRE(fixture.at<float>(colors.offset + colors.array.strides[0]) == 10);
    REQUIRE(fixture.at<float>(colors.offset + 2 * colors.array.strides[0] + 8) == 60);
    fixture.rejects("uniform3fv", {location, list({1, 2})}, "INVALID_VALUE");
    fixture.rejects("uniform3fv", {location, JSValue::array()}, "INVALID_VALUE");
    fixture.rejects("uniform3fv", {location, values, 8, 3}, "INVALID_VALUE");
    fixture.rejects("uniform3fv", {location, values, 4294967296.}, "INVALID_VALUE");
    fixture.rejects("uniform3fv", {location, values, 1, 4}, "INVALID_VALUE");
    const auto revision = fixture.commands.current()->uniformRevision;
    auto ignored = JSValue::object();
    ignored.set("valueOf", JSValue::function([](const JSValue&, const Args&, const Args&) -> JSValue { throw JSTypeError("Unused argument"); }));
    REQUIRE_NOTHROW(call(fixture.gl, "uniform1f", {fixture.location("opacity"), .75, ignored}));
    REQUIRE(fixture.floats("opacity", 1) == std::vector<float>{.75f});
    call(fixture.gl, "uniform3fv", {nullptr, values});
    REQUIRE(fixture.commands.current()->uniformRevision == revision + 1);
    REQUIRE(call(fixture.gl, "getError").strictEquals(0));
}

TEST_CASE("Deleting a noncurrent program retires its storage and rejects its handle and locations", "[vulkan][program-commands]") {
    ProgramFixture fixture; auto gl = fixture.gl;
    const auto location = fixture.location("opacity");
    const std::weak_ptr<VulkanProgramCommands::LinkedProgram> generation = fixture.commands.current();
    call(gl, "useProgram", {nullptr});
    REQUIRE_FALSE(generation.expired());
    call(gl, "deleteProgram", {fixture.program});
    REQUIRE(generation.expired());
    REQUIRE(fixture.commands.linked(fixture.program) == nullptr);
    REQUIRE(fixture.program.getInternalSlot("shaders").isNull());
    call(gl, "useProgram", {fixture.program});
    REQUIRE(call(gl, "getError").strictEquals(gl.get("INVALID_VALUE")));
    REQUIRE(fixture.commands.currentProgram().isNull());
    REQUIRE(call(gl, "getUniformLocation", {fixture.program, "opacity"}).isNull());
    REQUIRE(call(gl, "getError").strictEquals(gl.get("INVALID_VALUE")));
    call(gl, "linkProgram", {fixture.program});
    REQUIRE(call(gl, "getError").strictEquals(gl.get("INVALID_VALUE")));
    call(gl, "uniform1f", {location, .5});
    REQUIRE(call(gl, "getError").strictEquals(gl.get("INVALID_OPERATION")));
    call(gl, "deleteProgram", {fixture.program}); call(gl, "deleteProgram", {nullptr});
    REQUIRE(call(gl, "getError").strictEquals(0));
    VulkanProgramCommands other;
    call(other.object(), "deleteProgram", {fixture.program});
    REQUIRE(call(other.object(), "getError").strictEquals(gl.get("INVALID_OPERATION")));
}

TEST_CASE("Current delete-marked programs stay usable until a successful program switch", "[vulkan][program-commands]") {
    ProgramFixture fixture; auto gl = fixture.gl;
    const auto location = fixture.location("opacity");
    const std::weak_ptr<VulkanProgramCommands::LinkedProgram> generation = fixture.commands.current();
    call(gl, "deleteProgram", {fixture.program}); call(gl, "deleteProgram", {fixture.program});
    REQUIRE(call(gl, "getProgramParameter", {fixture.program, gl.get("DELETE_STATUS")}).strictEquals(true));
    REQUIRE(call(gl, "getProgramParameter", {fixture.program, gl.get("LINK_STATUS")}).strictEquals(true));
    REQUIRE(call(gl, "getProgramInfoLog", {fixture.program}).type() == JSValue::Type::String);
    REQUIRE(fixture.commands.linked(fixture.program) == fixture.commands.current());
    REQUIRE_FALSE(fixture.location("opacity").isNull());
    call(gl, "useProgram", {fixture.program});
    call(gl, "uniform1f", {location, .75});
    REQUIRE(fixture.floats("opacity", 1) == std::vector<float>{.75f});
    REQUIRE(call(gl, "getError").strictEquals(0));
    const auto unlinked = call(gl, "createProgram");
    call(gl, "useProgram", {unlinked});
    REQUIRE(call(gl, "getError").strictEquals(gl.get("INVALID_OPERATION")));
    VulkanProgramCommands other;
    call(gl, "useProgram", {call(other.object(), "createProgram")});
    REQUIRE(call(gl, "getError").strictEquals(gl.get("INVALID_OPERATION")));
    REQUIRE(fixture.commands.currentProgram().sameValue(fixture.program));
    REQUIRE_FALSE(generation.expired());
    const auto replacement = fixture.additionalProgram();
    call(gl, "useProgram", {replacement});
    REQUIRE(generation.expired());
    REQUIRE(fixture.commands.linked(fixture.program) == nullptr);
    REQUIRE(fixture.commands.currentProgram().sameValue(replacement));
    const auto revision = fixture.commands.current()->uniformRevision;
    call(gl, "uniform1f", {location, .25});
    REQUIRE(call(gl, "getError").strictEquals(gl.get("INVALID_OPERATION")));
    REQUIRE(fixture.commands.current()->uniformRevision == revision);
    REQUIRE(call(gl, "getProgramParameter", {fixture.program, gl.get("LINK_STATUS")}).isNull());
    REQUIRE(call(gl, "getError").strictEquals(gl.get("INVALID_VALUE")));
}

TEST_CASE("Unbinding a pending program preserves independently recorded draw generations", "[vulkan][program-commands]") {
    ProgramFixture fixture; auto gl = fixture.gl;
    auto recorded = fixture.commands.current();
    const std::weak_ptr<VulkanProgramCommands::LinkedProgram> generation = recorded;
    call(gl, "uniform1f", {fixture.location("opacity"), .5});
    const auto bytes = fixture.snapshot();
    call(gl, "deleteProgram", {fixture.program}); call(gl, "useProgram", {nullptr});
    REQUIRE(fixture.commands.currentProgram().isNull());
    REQUIRE(fixture.commands.current() == nullptr);
    REQUIRE(fixture.commands.linked(fixture.program) == nullptr);
    REQUIRE_FALSE(generation.expired());
    REQUIRE_FALSE(recorded->result.modules[0].words.empty());
    const auto recordedBytes = recorded->uniforms->bytes({0, 0});
    REQUIRE(std::vector<std::byte>(recordedBytes.begin(), recordedBytes.end()) == bytes);
    call(gl, "deleteProgram", {fixture.program}); call(gl, "deleteProgram", {nullptr});
    REQUIRE(call(gl, "getError").strictEquals(0));
    recorded.reset(); REQUIRE(generation.expired());
}

TEST_CASE("Shutdown releases current and noncurrent program resources while handles remain alive", "[vulkan][program-commands]") {
    ProgramFixture fixture; auto gl = fixture.gl;
    const auto noncurrent = fixture.additionalProgram(), location = fixture.location("opacity");
    const std::weak_ptr<VulkanProgramCommands::LinkedProgram> currentGeneration = fixture.commands.current();
    const std::weak_ptr<VulkanProgramCommands::LinkedProgram> otherGeneration = fixture.commands.linked(noncurrent);
    call(gl, "deleteProgram", {fixture.program});
    REQUIRE_FALSE(currentGeneration.expired()); REQUIRE_FALSE(otherGeneration.expired());
    fixture.commands.shutdown();
    REQUIRE(currentGeneration.expired()); REQUIRE(otherGeneration.expired());
    REQUIRE(fixture.commands.currentProgram().isNull());
    REQUIRE(fixture.commands.linked(fixture.program) == nullptr);
    REQUIRE(fixture.commands.linked(noncurrent) == nullptr);
    REQUIRE(fixture.program.getInternalSlot("shaders").isNull());
    REQUIRE(noncurrent.getInternalSlot("shaders").isNull());
    REQUIRE_NOTHROW(fixture.commands.shutdown());
    REQUIRE_THROWS_AS(call(gl, "uniform1f", {location, .25}), JSTypeError);
    REQUIRE_THROWS_AS(call(gl, "useProgram", {noncurrent}), JSTypeError);
}
