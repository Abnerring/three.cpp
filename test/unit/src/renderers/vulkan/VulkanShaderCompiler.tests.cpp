#include "catch.hpp"
#include "renderers/vulkan/VulkanShaderCompiler.h"
#include <future>

namespace {
const std::string vertex = R"glsl(#version 450
layout(location=0) in vec3 position;
layout(location=0) out vec2 vUv;
uniform mat4 modelViewProjection;
uniform float shift;
void main() { vUv = position.xy; gl_Position = modelViewProjection * vec4(position.x + shift, position.yz, 1.0); }
)glsl";
const std::string fragment = R"glsl(#version 450
layout(location=0) in vec2 vUv;
layout(location=0) out vec4 outputColor;
uniform vec4 diffuse;
uniform sampler2D map;
void main() { outputColor = diffuse * texture(map, vUv); }
)glsl";
VulkanShaderCompiler compiler() { return VulkanShaderCompiler(THREECPP_GLSLANG_EXECUTABLE); }
}

TEST_CASE("Native glslang links both stages and generates Vulkan SPIR-V with shared uniform layout",
          "[vulkan][shaders][compiler]") {
    const auto result = compiler().compile(vertex, fragment);
    INFO(result.diagnostics);
    REQUIRE(result.linked);
    REQUIRE(result.exitCode == 0);
    REQUIRE(result.vertex.size() > 5);
    REQUIRE(result.fragment.size() > 5);
    REQUIRE(result.vertex[0] == 0x07230203);
    REQUIRE(result.fragment[0] == 0x07230203);
    REQUIRE(result.diagnostics.find("modelViewProjection: offset 0") != std::string::npos);
    REQUIRE(result.diagnostics.find("shift: offset 64") != std::string::npos);
    REQUIRE(result.diagnostics.find("diffuse: offset 80") != std::string::npos);
    REQUIRE(result.diagnostics.find("ThreeDefaultUniforms: offset -1, type ffffffff, size 96") != std::string::npos);
    REQUIRE(result.diagnostics.find("map: offset -1, type 8b5e, size 1, index -1, binding 1") != std::string::npos);
}

TEST_CASE("Native shader syntax and interface errors keep diagnostics and produce no usable program",
          "[vulkan][shaders][compiler]") {
    const auto syntax = compiler().compile(vertex, "#version 450\nvoid main() { invalid! }\n");
    REQUIRE_FALSE(syntax.linked);
    REQUIRE(syntax.exitCode != 0);
    REQUIRE(syntax.vertex.empty());
    REQUIRE(syntax.fragment.empty());
    REQUIRE(syntax.diagnostics.find("ERROR") != std::string::npos);
    const auto interface = compiler().compile(vertex,
        "#version 450\nlayout(location=0) in vec3 vUv;\nlayout(location=0) out vec4 color;\nvoid main(){color=vec4(vUv,1.0);}\n");
    INFO(interface.diagnostics);
    REQUIRE_FALSE(interface.linked);
    REQUIRE(interface.exitCode != 0);
    REQUIRE(interface.vertex.empty());
    REQUIRE(interface.fragment.empty());
    REQUIRE(interface.diagnostics.find("ERROR") != std::string::npos);
}

TEST_CASE("Native shader compilation is independent of concurrent compilation and process working directory",
          "[vulkan][shaders][compiler]") {
    const auto directory = std::filesystem::current_path();
    auto first = std::async(std::launch::async, [] { return compiler().compile(vertex, fragment); });
    auto second = std::async(std::launch::async, [] { return compiler().compile(vertex, fragment); });
    const auto a = first.get(), b = second.get();
    REQUIRE(a.linked);
    REQUIRE(b.linked);
    REQUIRE(a.vertex == b.vertex);
    REQUIRE(a.fragment == b.fragment);
    REQUIRE(std::filesystem::current_path() == directory);
    REQUIRE_THROWS_AS(VulkanShaderCompiler(directory / "missing-glslang"), std::invalid_argument);
}
